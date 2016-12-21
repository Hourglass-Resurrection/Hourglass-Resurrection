/*
 * Copyright (c) 2016- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <algorithm>

#include "application/logging.h"
#include "File.h"

Utils::File::File::File(const std::wstring& filename, DWORD access, DWORD share, DWORD create, DWORD flags) :
    m_file(INVALID_HANDLE_VALUE)
{
    m_file = CreateFileW(filename.c_str(), access, share, nullptr, create, flags, nullptr);
    if (m_file == INVALID_HANDLE_VALUE)
    {
        PrintLastError(L"CreateFileW", GetLastError());
    }
}

Utils::File::File::~File()
{
    CloseFile();
}

bool Utils::File::File::ReadFile(LPBYTE buffer, SIZE_T length, SIZE_T* read)
{
    if (!IsValid())
    {
        return false;
    }

    SIZE_T read_bytes = 0;
    SIZE_T length_remains = length;
    while (read_bytes < length)
    {
        DWORD read_length = static_cast<DWORD>(std::min<SIZE_T>(static_cast<SIZE_T>(MAXDWORD), length_remains));
        DWORD bytes = 0;
        BOOL rv = ::ReadFile(m_file, buffer, read_length, &bytes, nullptr);
        if (rv == FALSE)
        {
            return false;
        }
        if (bytes == 0)
        {
            break;
        }
        read_bytes += bytes;
        length_remains -= bytes;
    }
            
    if (read != nullptr)
    {
        *read = read_bytes;
    }
    return true;
}

bool Utils::File::File::WriteFile(const LPBYTE buffer, SIZE_T length, SIZE_T * written)
{
    // TODO
    return false;
}

bool Utils::File::File::CloseFile()
{
    if (!IsValid())
    {
        return false;
    }
    return (CloseHandle(m_file) == TRUE);
}

SIZE_T Utils::File::File::GetSize()
{
    if (!IsValid())
    {
        return 0;
    }
    LARGE_INTEGER size;
    memset(&size, 0, sizeof(size));

    if (GetFileSizeEx(m_file, &size) != TRUE)
    {
        return 0;
    }
    return size.QuadPart;
}

    LONGLONG Utils::File::File::Seek(LONGLONG distance, DWORD starting_point)
    {
    if (!IsValid())
    {
        return -1;
    }
    LARGE_INTEGER move;
    PLONG high_part = &move.HighPart;
    move.QuadPart = distance;
    if (move.HighPart == 0)
    {
        high_part = nullptr;
    }
    DWORD rv = SetFilePointer(m_file, move.LowPart, high_part, starting_point);
    if (rv == INVALID_SET_FILE_POINTER)
    {
        return -1;
    }

    move.LowPart = rv;
    return move.QuadPart;
}

bool Utils::File::File::IsValid() const
{
    return m_file != INVALID_HANDLE_VALUE;
}



Utils::File::ExecutableFileHeaders::ExecutableFileHeaders(const std::wstring& filename) :
    m_valid(false)
{
    /*
     * This is not the most optimal way of dealing with it, reading the entire file into
     * memory, but the alternative makes this code horrendously complex with seeking and
     * offset math that is unnecessary when done this way.
     * So, sacrificing efficiency for easier to understand code.
     * -- Warepire
     */
    auto file = File(filename, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN);
    SIZE_T read;
    SIZE_T file_size;

    file_size = file.GetSize();
    if (file_size == 0)
    {
        return;
    }

    m_buffer.resize(file_size / sizeof(DWORD32));

    if (!file.ReadFile(GetBufferPointerAt(0), m_buffer.size(), &read) &&
        (read != file_size))
    {
        return;
    }

    auto nt_header = GetNTHeader();

    if (memcmp(&(nt_header->Signature), "PE\0\0", 4) != 0 ||
        nt_header->FileHeader.SizeOfOptionalHeader == 0 ||
        !(nt_header->FileHeader.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE))
    {
        return;
    }

    m_valid = true;
}


            
            //IMAGE_SECTION_HEADER section_header;
            //for (UINT i = 0; i < m_pe_header.FileHeader.NumberOfSections; i++)
            //{
            //    LONGLONG section_header_pos = m_dos_header.e_lfanew +
            //                                  sizeof(m_pe_header) +
            //                                  (i * sizeof(section_header));
            //    memcpy(&section_header,
            //           file_contents.data() + section_header_pos,
            //           sizeof(section_header));
            //    debugprintf(L"Section %S found.\n", section_header.Name);
            //    debugprintf(L"    VirtualSize: 0x%X\n", section_header.Misc.VirtualSize); //?
            //    debugprintf(L"    VirtualAddress: 0x%X\n", section_header.VirtualAddress);
            //    debugprintf(L"    SizeOfRawData: 0x%X\n", section_header.SizeOfRawData);
            //    debugprintf(L"    PointerToRawData: 0x%X\n", section_header.PointerToRawData);
            //    debugprintf(L"    PointerToRelocations: 0x%X\n", section_header.PointerToRelocations);
            //    debugprintf(L"    PointerToLinenumbers: 0x%X\n", section_header.PointerToLinenumbers);
            //    debugprintf(L"    NumberOfRelocations: 0x%X\n", section_header.NumberOfRelocations);
            //    debugprintf(L"    NumberOfLinenumbers: 0x%X\n", section_header.NumberOfLinenumbers);
            //    debugprintf(L"    Characteristics: 0x%X\n", section_header.Characteristics);
            //}


DWORD Utils::File::ExecutableFileHeaders::GetImageSizeInRAM() const
{
    return GetNTHeader()->OptionalHeader.SizeOfImage;
}

std::map<DWORD64, std::wstring> Utils::File::ExecutableFileHeaders::GetExportTable(DWORD64 mod_base) const
{
    std::map<DWORD64, std::wstring> export_table;

    auto export_directory = GetExportDirectory();
    if (!export_directory)
    {
        return export_table;
    }

    DWORD dll_name_offset = RvaToOffset(export_directory->Name);
    DWORD function_table_offset = RvaToOffset(export_directory->AddressOfFunctions);
    DWORD name_table_offset = RvaToOffset(export_directory->AddressOfNames);
    DWORD ordinal_table_offset = RvaToOffset(export_directory->AddressOfNameOrdinals);
    /*
     * NumberOfNames is also the number of ordinals.
     * The number of ordinals is always equal to the number of exported functions
     * in the DLL. Not all functions have a name.
     */
    for (UINT j = 0; j < export_directory->NumberOfNames; j++)
    {
        WORD ordinal;
        memcpy(&ordinal, GetBufferPointerAt(ordinal_table_offset + (j * sizeof(WORD))), sizeof(WORD));
        DWORD name_offset;
        memcpy(&name_offset, GetBufferPointerAt(name_table_offset + (j * sizeof(DWORD))), sizeof(DWORD));
        name_offset = RvaToOffset(name_offset);
        /*
         * The index is supposed to be evaluated according to the following formula:
         *
         *   ordinal_index = NameTableLookup("FunctionName")
         *   ordinal = OrdinalTableLookup(ordinal_index)
         *   function = FunctionTableLookup(ordinal - ExportDirectory.OrdinalBase)
         *
         * However, applying the adjustment of the OrdinalBase is invalid and causes reading outside
         * of the buffer range, and when it doesn't crash, maps the wrong name to functions.
         * -- Warepire
         */
        DWORD index = ordinal * sizeof(DWORD);
        DWORD function_address;
        memcpy(&function_address, GetBufferPointerAt(function_table_offset + index), sizeof(DWORD));
        std::wstring name;
        if (name_offset == -1)
        {
            name_offset = dll_name_offset;
        }
        for (UINT k = 0; GetBufferPointerAt(name_offset)[k] != '\0'; k++)
        {
            name.push_back(GetBufferPointerAt(name_offset)[k]);
        }
        if (name_offset == dll_name_offset)
        {
            name.append(L".Ordinal").append(std::to_wstring(ordinal));
        }

        export_table.emplace(function_address + mod_base, std::move(name));
    }
    return export_table;
}

Utils::File::ExecutableFileHeaders::ImageRelocationTable Utils::File::ExecutableFileHeaders::GetRelocationTable(DWORD64 mod_base) const
{
    auto nt_header = GetNTHeader();
    Utils::File::ExecutableFileHeaders::ImageRelocationTable table = { mod_base - nt_header->OptionalHeader.ImageBase };

    if (nt_header->OptionalHeader.NumberOfRvaAndSizes >= IMAGE_DIRECTORY_ENTRY_BASERELOC)
    {
        DWORD reloc_offset = RvaToOffset(nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
        LPBYTE relocations = GetBufferPointerAt(reloc_offset);
        PIMAGE_BASE_RELOCATION this_reloc = reinterpret_cast<PIMAGE_BASE_RELOCATION>(relocations);
        for (UINT j = 0; j < nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
            j += this_reloc->SizeOfBlock, relocations += this_reloc->SizeOfBlock,
            this_reloc = reinterpret_cast<PIMAGE_BASE_RELOCATION>(relocations))
        {
            /*
             * Sometimes the .reloc table is smaller than the allocated space
             * in the executable, avoid trying to parse the padding data.
             */
            if (this_reloc->VirtualAddress == 0 || this_reloc->SizeOfBlock == 0)
            {
                break;
            }
            UINT number_of_relocs = (this_reloc->SizeOfBlock - sizeof(this_reloc)) / sizeof(WORD);
            //debugprintf(L"    VirtualAddress: 0x%X\n", this_reloc.VirtualAddress);
            //debugprintf(L"    SizeOfBlock: 0x%X\n", this_reloc.SizeOfBlock);
            //debugprintf(L"    Relocs: %d\n", number_of_relocs);
            WORD reloc;
            for (UINT r = 0; r < number_of_relocs; r++)
            {
                memcpy(&reloc, relocations + sizeof(this_reloc) + (r * sizeof(WORD)), sizeof(WORD));
                /*
                 * reloc entries that have the top 4 bites zero'd are just padding for alignment as
                 * each block must start on a 4-byte boundary.
                 */
                if (reloc & 0xF000 == 0)
                {
                    continue;
                }
                /*
                 * The top 4 bits represent the relocation type, and the bottom 12 bits
                 * the offset into the page pointed to by VirtualAddress above.
                 */
                auto& rd = table.m_relocation_data;
                rd.push_back({ static_cast<WORD>(reloc >> 12),
                               mod_base + this_reloc->VirtualAddress + static_cast<WORD>(reloc & 0x0FFF) });
            }
        }
    }
    return ImageRelocationTable();
}

bool Utils::File::ExecutableFileHeaders::IsValid() const
{
    return m_valid;
}

DWORD Utils::File::ExecutableFileHeaders::RvaToOffset(DWORD rva) const
{
    auto nt_header = GetNTHeader();
    auto sections = GetSectionHeader();
    for (DWORD i = 0; i < nt_header->FileHeader.NumberOfSections; i++)
    {
        if (sections[i].VirtualAddress + sections[i].SizeOfRawData >= rva)
        {
            return (sections[i].PointerToRawData + rva) - sections[i].VirtualAddress;
        }
    }
    return -1;
}

const PIMAGE_DOS_HEADER Utils::File::ExecutableFileHeaders::GetDOSHeader() const
{
    return reinterpret_cast<PIMAGE_DOS_HEADER>(GetBufferPointerAt(0));
}

const PIMAGE_NT_HEADERS32 Utils::File::ExecutableFileHeaders::GetNTHeader() const
{
    auto dos_header = GetDOSHeader();
    return reinterpret_cast<PIMAGE_NT_HEADERS32>(GetBufferPointerAt(dos_header->e_lfanew));
}

const PIMAGE_SECTION_HEADER Utils::File::ExecutableFileHeaders::GetSectionHeader() const
{
    auto dos_header = GetDOSHeader();
    return reinterpret_cast<PIMAGE_SECTION_HEADER>(GetBufferPointerAt(dos_header->e_lfanew + sizeof(IMAGE_NT_HEADERS32)));
}

const PIMAGE_EXPORT_DIRECTORY Utils::File::ExecutableFileHeaders::GetExportDirectory() const
{
    LPBYTE directory = nullptr;
    auto nt_header = GetNTHeader();

    if (nt_header->OptionalHeader.NumberOfRvaAndSizes >= IMAGE_DIRECTORY_ENTRY_EXPORT &&
        nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress != 0)
    {
        directory = GetBufferPointerAt(RvaToOffset(nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress));
    }
    return reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(directory);
}

const LPBYTE Utils::File::ExecutableFileHeaders::GetBufferPointerAt(SIZE_T offset) const
{
    LPBYTE buf_as_lpbyte = reinterpret_cast<LPBYTE>(const_cast<DWORD32*>(m_buffer.data()));
    return buf_as_lpbyte + offset;
}
