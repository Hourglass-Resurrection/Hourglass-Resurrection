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



        PEHeader::PEHeader(const std::wstring& filename)
        {
            auto file = File();
            SIZE_T read;
            if (!file.OpenFile(filename, GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN))
            {
                return;
            }
            if (!file.ReadFile(reinterpret_cast<LPBYTE>(&m_dos_header), sizeof(m_dos_header), &read) && (read != sizeof(m_dos_header)))
            {
                return;
            }
            if (m_dos_header.e_lfanew > sizeof(m_dos_header))
            {
                LONGLONG length = m_dos_header.e_lfanew - sizeof(m_dos_header);
                if (file.Seek(length, FILE_CURRENT) != m_dos_header.e_lfanew)
                {
                    return;
                }
            }
            if (!file.ReadFile(reinterpret_cast<LPBYTE>(&m_pe_header), sizeof(m_pe_header), &read) && (read != sizeof(m_pe_header)))
            {
                return;
            }
        }

        DWORD PEHeader::GetImageSizeInRAM() const
        {
            return m_pe_header.OptionalHeader.SizeOfImage;
        }
    }
}