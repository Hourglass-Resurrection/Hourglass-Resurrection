/*
 * Copyright (c) 2016- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <map>
#include <string>
#include <vector>

namespace Utils
{
    namespace File
    {
        class File
        {
        public:
            File();
            ~File();
            bool OpenFile(const std::wstring& filename, DWORD access, DWORD share, DWORD create, DWORD flags);
            bool ReadFile(LPBYTE buffer, SIZE_T length, SIZE_T* read);
            bool WriteFile(const LPBYTE buffer, SIZE_T length, SIZE_T* written);
            bool CloseFile();
            SIZE_T GetSize();
            LONGLONG Seek(LONGLONG distance, DWORD starting_point);
            bool IsValid() const;
        private:
            HANDLE m_file;
        };

        class ExecutableFileHeaders
        {
        public:
            // TODO: Safer init interface
            ExecutableFileHeaders(const std::wstring& filename);

            DWORD GetImageSizeInRAM() const;
            std::map<DWORD, std::wstring> GetExportTable() const;
        private:
            struct RelocationData
            {
                ::WORD m_type;
                ::WORD m_offset;
            };
            struct ImageRelocations
            {
                DWORD m_page_base;
                std::vector<RelocationData> m_relocations;
            };
            IMAGE_DOS_HEADER m_dos_header;
            IMAGE_NT_HEADERS32 m_pe_header;
            std::map<DWORD, std::wstring> m_export_table;
            std::vector<ImageRelocations> m_image_relocations;
            // TODO: More image header parts?

            static constexpr const BYTE ms_export_table_name[IMAGE_SIZEOF_SHORT_NAME] =
                { '.', 'e', 'd', 'a', 't', 'a', '\0', '\0' };
            static constexpr const BYTE ms_relocation_table_name[IMAGE_SIZEOF_SHORT_NAME] =
                { '.', 'r', 'e', 'l', 'o', 'c', '\0', '\0' };
        };
    }
}
