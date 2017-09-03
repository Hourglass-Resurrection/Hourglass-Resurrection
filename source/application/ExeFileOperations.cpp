#include <Windows.h>
#include <stdio.h>

// Gets the default stack size of a Win32 exe file using it's PE header.
// By default the stack size is 1 megabyte, and most programs use that size,
// but some linkers (incl. GNUs and MSVCs) allow this value to be customized using a flag,
// so it would be naive to assume 1 megabyte for all programs, specially very complex ones.
// An example being Cheat Engine that has chosen to make the default stack size 16(!) megabytes.

// Returns the stack size in bytes on success, and 0 if it failed.
// TODO: Perhaps a bit overkill on the error checking?
unsigned int GetWin32ExeDefaultStackSize(LPCWSTR exefile)
{
    // Open exe
    FILE* exe = _wfopen(exefile, L"rb");
    if (exe == NULL)
    {
        return 0;
    }

    // Seek to position of the bytes that contain the PE header offset. (Note that this is only valid on Win32 exes!)
    if (fseek(exe, 0x3C, SEEK_SET))
    {
        // Failed to seek in the file
        fclose(exe);
        return 0;
    }

    // As far as I can tell the offset is an unsigned int, but it might be an unsigned short.
    // If some exes produces weird results, open them in a hex editor and look for this byte sequence "50 45 00 00" (the first 4 bytes of the PE header, "PE\0\0"),
    // and then compare the location of the first byte of the PE header to the location given by the 4 bytes from the range 0x3C to 0x3F. (Note: Little endian byte order!)
    // If the last 2 bytes add something that creates a bad offset, then it's an unsigned short instead, and the blow lines should be fixed to solve the issue like this:
    // unsigned int peHeaderOffset = 0; --> unsigned short peHeaderOffset = 0;
    // fread(&peHeaderOffset, 4, 1, exe); --> fread(&peHeaderOffset, 2, 1, exe);
    // And also please update this (and other comments) to the accurate information.
    // -- Warepire
    unsigned int peHeaderOffset = 0;

    // Read the bytes
    fread(&peHeaderOffset, 4, 1, exe);
    if (feof(exe))
    {
        // Hit EOF before the 4 bytes were read, something's wrong.
        fclose(exe);
        return 0;
    }

    // Calculate the offset of the default stack size value.
    unsigned int defaultStackSizeOffset =
        peHeaderOffset
        + 0x60; // 0x60 is the offset from the PE-header start to where the default stack size is stored by the linker in the PE header

    // Seek to the location in the file.
    if (fseek(exe, defaultStackSizeOffset, SEEK_SET))
    {
        // Failed to seek in the file
        fclose(exe);
        return 0;
    }

    unsigned int defaultStackSize = 0;

    // Read the default stack size.
    fread(&defaultStackSize, 4, 1, exe);
    if (feof(exe))
    {
        // Hit EOF before the 4 bytes were read, something's wrong.
        fclose(exe);
        return 0;
    }

    fclose(exe);

    return defaultStackSize; // Yay, success!
}
