/*
 * Copyright (c) 2016- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <algorithm>

#include "../File.h"
#include "application/logging.h"

Utils::File::ExecutableFileHeaders::ExecutableFileHeaders(const std::wstring& filename)
    : m_file(INVALID_HANDLE_VALUE)
    , m_mapped_file(INVALID_HANDLE_VALUE)
    , m_buffer(nullptr)
    , m_valid(false)
{
    HANDLE m_file = CreateFileW(filename.c_str(),
                                GENERIC_READ,
                                FILE_SHARE_READ,
                                nullptr,
                                OPEN_EXISTING,
                                FILE_FLAG_SEQUENTIAL_SCAN,
                                nullptr);
    if (m_file == INVALID_HANDLE_VALUE)
    {
        return;
    }

    LARGE_INTEGER size = {0, 0};
    if (GetFileSizeEx(m_file, &size) == FALSE)
    {
        return;
    }

    HANDLE m_mapped_file = CreateFileMappingW(m_file,
                                              nullptr,
                                              PAGE_READONLY,
                                              size.u.HighPart,
                                              size.u.LowPart,
                                              nullptr);
    if (m_mapped_file == INVALID_HANDLE_VALUE)
    {
        return;
    }

    m_buffer = reinterpret_cast<BYTE*>(MapViewOfFile(m_mapped_file, FILE_MAP_READ, 0, 0, 0));
    if (m_buffer == nullptr)
    {
        return;
    }

    auto nt_header = GetNTHeader();

    if (memcmp(&(nt_header->Signature), "PE\0\0", 4) != 0
        || nt_header->FileHeader.SizeOfOptionalHeader == 0
        || !(nt_header->FileHeader.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE))
    {
        return;
    }

    m_valid = true;
}

Utils::File::ExecutableFileHeaders::~ExecutableFileHeaders()
{
    if (m_mapped_file != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_mapped_file);
    }

    if (m_file != INVALID_HANDLE_VALUE)
    {
        CloseHandle(m_file);
    }
}

DWORD Utils::File::ExecutableFileHeaders::GetImageSizeInRAM() const
{
    return GetNTHeader()->OptionalHeader.SizeOfImage;
}

std::map<DWORD64, std::wstring> Utils::File::ExecutableFileHeaders::GetExportTable(
    DWORD64 mod_base) const
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
        memcpy(&ordinal, m_buffer + ordinal_table_offset + (j * sizeof(WORD)), sizeof(WORD));
        DWORD name_offset;
        memcpy(&name_offset, m_buffer + name_table_offset + (j * sizeof(DWORD)), sizeof(DWORD));
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
        memcpy(&function_address, m_buffer + function_table_offset + index, sizeof(DWORD));
        std::wstring name;
        if (name_offset == -1)
        {
            name_offset = dll_name_offset;
        }
        for (UINT k = 0; (m_buffer + name_offset)[k] != '\0'; k++)
        {
            name.push_back((m_buffer + name_offset)[k]);
        }
        if (name_offset == dll_name_offset)
        {
            name.append(L".Ordinal").append(std::to_wstring(ordinal));
        }

        export_table.emplace(function_address + mod_base, std::move(name));
    }
    return export_table;
}

Utils::File::ExecutableFileHeaders::ImageRelocationTable Utils::File::ExecutableFileHeaders::
    GetRelocationTable(DWORD64 mod_base) const
{
    auto nt_header = GetNTHeader();
    Utils::File::ExecutableFileHeaders::ImageRelocationTable table = {
        mod_base - nt_header->OptionalHeader.ImageBase};

    if (nt_header->OptionalHeader.NumberOfRvaAndSizes >= IMAGE_DIRECTORY_ENTRY_BASERELOC)
    {
        DWORD reloc_offset =
            RvaToOffset(nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC]
                            .VirtualAddress);
        LPBYTE relocations = const_cast<LPBYTE>(m_buffer) + reloc_offset;
        PIMAGE_BASE_RELOCATION this_reloc = reinterpret_cast<PIMAGE_BASE_RELOCATION>(relocations);
        for (UINT j = 0;
             j < nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
             j += this_reloc->SizeOfBlock,
                  relocations += this_reloc->SizeOfBlock,
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
            WORD reloc;
            for (UINT r = 0; r < number_of_relocs; r++)
            {
                memcpy(&reloc, relocations + sizeof(this_reloc) + (r * sizeof(WORD)), sizeof(WORD));
                /*
                 * reloc entries that have the top 4 bits zero'd are just padding for alignment as
                 * each block must start on a 4-byte boundary.
                 */
                if ((reloc & 0xF000) == 0)
                {
                    continue;
                }
                /*
                 * The top 4 bits represent the relocation type, and the bottom 12 bits
                 * the offset into the page pointed to by VirtualAddress above.
                 */
                auto& rd = table.m_relocation_data;
                rd.push_back(
                    {static_cast<WORD>(reloc >> 12),
                     mod_base + this_reloc->VirtualAddress + static_cast<WORD>(reloc & 0x0FFF)});
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
        if (sections[i].VirtualAddress <= rva
            && sections[i].VirtualAddress + sections[i].SizeOfRawData > rva)
        {
            return (sections[i].PointerToRawData + rva) - sections[i].VirtualAddress;
        }
    }
    return -1;
}

const PIMAGE_DOS_HEADER Utils::File::ExecutableFileHeaders::GetDOSHeader() const
{
    return reinterpret_cast<PIMAGE_DOS_HEADER>(const_cast<LPBYTE>(m_buffer));
}

const PIMAGE_NT_HEADERS32 Utils::File::ExecutableFileHeaders::GetNTHeader() const
{
    auto dos_header = GetDOSHeader();
    return reinterpret_cast<PIMAGE_NT_HEADERS32>(const_cast<LPBYTE>(m_buffer)
                                                 + dos_header->e_lfanew);
}

const PIMAGE_SECTION_HEADER Utils::File::ExecutableFileHeaders::GetSectionHeader() const
{
    auto dos_header = GetDOSHeader();
    return reinterpret_cast<PIMAGE_SECTION_HEADER>(
        const_cast<LPBYTE>(m_buffer) + dos_header->e_lfanew + sizeof(IMAGE_NT_HEADERS32));
}

const PIMAGE_EXPORT_DIRECTORY Utils::File::ExecutableFileHeaders::GetExportDirectory() const
{
    LPBYTE directory = nullptr;
    auto nt_header = GetNTHeader();

    if (nt_header->OptionalHeader.NumberOfRvaAndSizes >= IMAGE_DIRECTORY_ENTRY_EXPORT
        && nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress
               != 0)
    {
        directory =
            const_cast<LPBYTE>(m_buffer)
            + RvaToOffset(nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT]
                              .VirtualAddress);
    }
    return reinterpret_cast<PIMAGE_EXPORT_DIRECTORY>(directory);
}
