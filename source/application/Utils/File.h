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
            /*
             * The access, share, create and flags variables map against the counterparts
             * in the CreateFile() API.
             */
            File(const std::wstring& filename, DWORD access, DWORD share, DWORD create, DWORD flags);
            ~File();
            /*
             * TODO: Read/WriteFile to return SIZE_T instead?
             */
            bool ReadFile(LPBYTE buffer, SIZE_T length, SIZE_T* read);
            bool WriteFile(const LPBYTE buffer, SIZE_T length, SIZE_T* written);
            bool CloseFile();
            /*
             * Returns the file size in bytes.
             * Returns 0 on failure.
             */
            SIZE_T GetSize();
            /*
             * Returns the new position of the file pointer in the file.
             * Returns -1 on failure.
             */
            LONGLONG Seek(LONGLONG distance, DWORD starting_point);

            bool IsValid() const;
        private:
            HANDLE m_file;
        };

        /*
         * ExecutableFileHeaders provides access to decoded contents of the PE / COFF headers
         * in an executable file.
         *
         * NOTE: Don't keep this object around for too long as it may use several megabytes of memory.
         *       This is an unfortunate side-effect of having any form of performance on the parsing
         *       as parsing the PE / COFF headers requires jumping around in the file.
         */
        class ExecutableFileHeaders
        {
        public:
            struct RelocationData
            {
                /*
                 * One of the IMAGE_REL_BASED_* values
                 */
                WORD m_relocation_type;
                /*
                 * The location in memory to apply the relocation data.
                 */
                DWORD64 m_address;
            };
            struct ImageRelocationTable
            {
                /*
                 * This is the value that should be applied at all locations pointed to by
                 * the RelocationData structs in accordance to the relocation type.
                 */
                INT m_diff;
                std::vector<RelocationData> m_relocation_data;
            };

            ExecutableFileHeaders(const std::wstring& filename);

            /*
             * Returns the size (in bytes) of the DLL as it would be when loaded using i.e.
             * LoadLibrary().
             */
            DWORD GetImageSizeInRAM() const;
            /*
             * Returns a map of the methods in the DLL containing the address of the method
             * in RAM and it's name. The format [dllname].Ordinal[ordinal-ID] is used when
             * there is no name for the method.
             */
            std::map<DWORD64, std::wstring> GetExportTable(DWORD64 mod_base) const;
            /*
             * Returns a struct contaitning the information necessary to manually relocate
             * a DLL in memory.
             */
            ImageRelocationTable GetRelocationTable(DWORD64 mod_base) const;

            bool IsValid() const;
        private:
            DWORD RvaToOffset(DWORD rva) const;

            /*
             * Add more image header sections as they are discovered to be necessary.
             */
            const PIMAGE_DOS_HEADER GetDOSHeader() const;
            const PIMAGE_NT_HEADERS32 GetNTHeader() const;
            const PIMAGE_SECTION_HEADER GetSectionHeader() const;
            const PIMAGE_EXPORT_DIRECTORY GetExportDirectory() const;

            /*
             * Use a data-type for the vector that is 32-bit aligned.
             */
            std::vector<DWORD32> m_buffer;
            /*
             * Provide an accessor to provide a byte-pointer offset into the buffer
             */
            const LPBYTE GetBufferPointerAt(SIZE_T offset) const;

            bool m_valid;
        };
    }
}
