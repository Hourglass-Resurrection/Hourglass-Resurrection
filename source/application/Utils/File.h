/*
 * Copyright (c) 2016- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

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

        class PEHeader
        {
        public:
            // TODO: Safer init interface
            PEHeader(const std::wstring& filename);

            DWORD GetImageSizeInRAM() const;
        private:
            IMAGE_DOS_HEADER m_dos_header;
            IMAGE_NT_HEADERS32 m_pe_header;
            // TODO: More image header parts
        };
    }
}
