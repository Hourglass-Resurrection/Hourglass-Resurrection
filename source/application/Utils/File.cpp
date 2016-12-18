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

namespace Utils
{
    namespace File
    {
        File::File() :
            m_file(INVALID_HANDLE_VALUE)
        {
        }

        File::~File()
        {
            CloseFile();
        }

        bool File::OpenFile(const std::wstring& filename, DWORD access, DWORD share, DWORD create, DWORD flags)
        {
            if (IsValid())
            {
                return false;
            }
            m_file = CreateFileW(filename.c_str(), access, share, nullptr, create, flags, nullptr);
            if (m_file == INVALID_HANDLE_VALUE)
            {
                PrintLastError(L"CreateFileW", GetLastError());
                return false;
            }
            return true;
        }

        bool File::ReadFile(LPBYTE buffer, SIZE_T length, SIZE_T* read)
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

        bool File::WriteFile(const LPBYTE buffer, SIZE_T length, SIZE_T * written)
        {
            // TODO
            return false;
        }

        bool File::CloseFile()
        {
            if (!IsValid())
            {
                return false;
            }
            return (CloseHandle(m_file) == TRUE);
        }

        SIZE_T File::GetSize()
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

        LONGLONG File::Seek(LONGLONG distance, DWORD starting_point)
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

        bool File::IsValid() const
        {
            return m_file != INVALID_HANDLE_VALUE;
        }



        ExecutableFileHeaders::ExecutableFileHeaders(const std::wstring& filename)
        {
            /*
             * This is not the most optimal way of dealing with it, reading the entire file into
             * memory, but the alternative makes this code horrendously complex with seeking and
             * offset math that is unnecessary when done this way.
             * So, sacrificing efficiency for easier to understand code.
             * -- Warepire
             */
            auto file = File();
            SIZE_T read;
            SIZE_T file_size;
            if (!file.OpenFile(filename, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING,
                FILE_FLAG_SEQUENTIAL_SCAN))
            {
                return;
            }
            file_size = file.GetSize();
            if (file_size == 0)
            {
                return;
            }
            std::vector<BYTE> file_contents(file_size);
            if (!file.ReadFile(file_contents.data(), file_contents.size(), &read) &&
                (read != file_size))
            {
                return;
            }

            memcpy(&m_dos_header, file_contents.data(), sizeof(m_dos_header));
            memcpy(&m_pe_header, file_contents.data() + m_dos_header.e_lfanew, sizeof(m_pe_header));

            if (memcmp(&(m_pe_header.Signature), "PE\0\0", 4) != 0 ||
                m_pe_header.FileHeader.SizeOfOptionalHeader == 0 ||
                !(m_pe_header.FileHeader.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE))
            {
                return;
            }

            PIMAGE_SECTION_HEADER sections =
                reinterpret_cast<PIMAGE_SECTION_HEADER>(file_contents.data() + 
                                                        m_dos_header.e_lfanew +
                                                        sizeof(m_pe_header));
            auto RvaToOffset = [sections, num_headers=m_pe_header.FileHeader.NumberOfSections](DWORD rva) -> DWORD
            {
                for (DWORD i = 0; i < num_headers; i++)
                {
                    if (sections[i].VirtualAddress + sections[i].SizeOfRawData >= rva)
                    {
                        return (sections[i].PointerToRawData + rva) - sections[i].VirtualAddress;
                    }
                }
                return -1;
            };

            if (m_pe_header.OptionalHeader.NumberOfRvaAndSizes >= IMAGE_DIRECTORY_ENTRY_EXPORT &&
                m_pe_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress > 0)
            {
                DWORD export_offset = RvaToOffset(m_pe_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
                IMAGE_EXPORT_DIRECTORY directory;
                memcpy(&directory, file_contents.data() + export_offset, sizeof(directory));
                DWORD dll_name_offset = RvaToOffset(directory.Name);
                DWORD function_table_offset = RvaToOffset(directory.AddressOfFunctions);
                DWORD name_table_offset = RvaToOffset(directory.AddressOfNames);
                DWORD ordinal_table_offset = RvaToOffset(directory.AddressOfNameOrdinals);
                /*
                 * NumberOfNames is also the number of ordinals.
                 * The number of ordinals is always equal to the number of exported functions
                 * in the DLL. Not all functions have a name.
                 */
                for (UINT j = 0; j < directory.NumberOfNames; j++)
                {
                    WORD ordinal;
                    memcpy(&ordinal, file_contents.data() + ordinal_table_offset + (j * sizeof(WORD)), sizeof(WORD));
                    DWORD index = (ordinal - 1/* - directory.Base*/) * sizeof(DWORD);
                    DWORD function_address;
                    memcpy(&function_address, file_contents.data() + function_table_offset + index, sizeof(DWORD));
                    DWORD name_offset;
                    memcpy(&name_offset, file_contents.data() + name_table_offset + index, sizeof(DWORD));
                    name_offset = RvaToOffset(name_offset);
                    std::wstring name;
                    if (name_offset == -1)
                    {
                        name_offset = dll_name_offset;
                    }
                    for (UINT k = 0; (file_contents.data() + name_offset)[k] != '\0'; k++)
                    {
                        name.push_back((file_contents.data() + name_offset)[k]);
                    }
                    if (name_offset == dll_name_offset)
                    {
                        name.append(L".Ordinal").append(std::to_wstring(ordinal));
                    }
                    //debugprintf(L"Found ordinal %d, function RVA 0x%X, name %s\n", ordinal, function_address, name.c_str());
                    m_export_table.emplace(function_address, std::move(name));
                }
            }
            if (m_pe_header.OptionalHeader.NumberOfRvaAndSizes >= IMAGE_DIRECTORY_ENTRY_BASERELOC)
            {
                DWORD reloc_offset = RvaToOffset(m_pe_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress);
                LPBYTE relocations = file_contents.data() + reloc_offset;
                IMAGE_BASE_RELOCATION this_reloc;
                for (UINT j = 0; j < m_pe_header.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size;
                    j += this_reloc.SizeOfBlock, relocations += this_reloc.SizeOfBlock)
                {
                    memcpy(&this_reloc, relocations, sizeof(this_reloc));
                    /*
                    * Sometimes the .reloc table is smaller than the allocated space
                    * in the executable, avoid trying to parse the padding data.
                    */
                    if (this_reloc.VirtualAddress == 0 || this_reloc.SizeOfBlock == 0)
                    {
                        break;
                    }
                    m_image_relocations.push_back({ this_reloc.VirtualAddress });
                    UINT number_of_relocs = (this_reloc.SizeOfBlock - sizeof(this_reloc)) / sizeof(WORD);
                    /*debugprintf(L"    VirtualAddress: 0x%X\n", this_reloc.VirtualAddress);
                    debugprintf(L"    SizeOfBlock: 0x%X\n", this_reloc.SizeOfBlock);
                    debugprintf(L"    Relocs: %d\n", number_of_relocs);*/
                    WORD reloc;
                    for (UINT r = 0; r < number_of_relocs; r++)
                    {
                        memcpy(&reloc, relocations + sizeof(this_reloc) + (r * sizeof(WORD)), sizeof(WORD));
                        /*
                        * reloc entries that are 0 are just padding for alignment as each block
                        * must start on a 4-byte boundary.
                        */
                        if (reloc == 0)
                        {
                            continue;
                        }
                        /*
                        * The top 4 bits represent the relocation type, and the bottom 12 bits
                        * the offset into the page pointed to by VirtualAddress above.
                        */
                        auto& image_reloc = m_image_relocations.back().m_relocations;
                        image_reloc.push_back({ static_cast<WORD>(reloc >> 12), static_cast<WORD>(reloc & 0x0FFF) });
                    }
                }
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
        }

        DWORD ExecutableFileHeaders::GetImageSizeInRAM() const
        {
            return m_pe_header.OptionalHeader.SizeOfImage;
        }

        std::map<DWORD, std::wstring> ExecutableFileHeaders::GetExportTable() const
        {
            return m_export_table;
        }
    }
}