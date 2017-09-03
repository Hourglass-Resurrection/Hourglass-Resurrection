/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <stdexcept>

#include "../Exceptions.h"

namespace
{
    static void WindowsExceptionHandler(unsigned int code, EXCEPTION_POINTERS*)
    {
        LPCSTR message;
        switch (code)
        {
        case EXCEPTION_ACCESS_VIOLATION:
            message = "The thread attempted to read from or write to a virtual address for which "
                      "it does not have access.";
            break;
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
            message = "The thread attempted to access an array element that is out of bounds.";
            break;
        case EXCEPTION_BREAKPOINT:
            message = "A breakpoint was encountered.";
            break;
        case EXCEPTION_DATATYPE_MISALIGNMENT:
            message = "The thread attempted to read or write data that is misaligned on hardware "
                      "that does not provide alignment.";
            break;
        case EXCEPTION_FLT_DENORMAL_OPERAND:
            message = "One of the operands in a floating point operation was denormal.";
            break;
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:
            message = "The thread attempted to divide a floating point value by a floating point "
                      "divisor of 0 (zero).";
            break;
        case EXCEPTION_FLT_INEXACT_RESULT:
            message = "The result of a floating point operation could not be represented exactly "
                      "as a decimal fraction.";
            break;
        case EXCEPTION_FLT_INVALID_OPERATION:
            message = "Invalid floating point operation.";
            break;
        case EXCEPTION_FLT_OVERFLOW:
            message = "The exponent of a floating point operation was greater than the magnitude "
                      "allowed by the corresponding type.";
            break;
        case EXCEPTION_FLT_STACK_CHECK:
            message = "The stack overflowed or underflowed, because of a floating point operation.";
            break;
        case EXCEPTION_FLT_UNDERFLOW:
            message = "The exponent of a floating point operation was less than the magnitude "
                      "allowed by the corresponding type.";
            break;
        case EXCEPTION_GUARD_PAGE:
            message = "The thread accessed memory allocated with the PAGE_GUARD modifier.";
            break;
        case EXCEPTION_ILLEGAL_INSTRUCTION:
            message = "The thread tried to execute an invalid instruction.";
            break;
        case EXCEPTION_IN_PAGE_ERROR:
            message = "The thread tried to access a page that is not present, and the system was "
                      "unable to load the page.";
            break;
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
            message = "The thread attempted to divide an integer value by an integer divisor of 0 "
                      "(zero).";
            break;
        case EXCEPTION_INT_OVERFLOW:
            message = "The result of an integer operation created a value that was too large to be "
                      "held by the destination register.";
            break;
        case EXCEPTION_INVALID_DISPOSITION:
            message =
                "An exception handler returned an invalid disposition to the exception dispatcher.";
            break;
        case EXCEPTION_INVALID_HANDLE:
            message = "The thread used a handle to a kernel object that was invalid. Probably "
                      "because it has been closed.";
            break;
        case EXCEPTION_NONCONTINUABLE_EXCEPTION:
            message = "The thread attempted to continue execution after a non-continuable "
                      "exception occured.";
            break;
        case EXCEPTION_PRIV_INSTRUCTION:
            message = "The thread attempted to execute an instruction with an operation that is "
                      "not allowed in the current computer mode.";
            break;
        case EXCEPTION_SINGLE_STEP:
            message = "Trace trap or other single instruction mechanism signal.";
            break;
        case EXCEPTION_STACK_OVERFLOW:
            message = "The thread used up its stack.";
            break;
        case STATUS_UNWIND_CONSOLIDATE:
            message = "A frame consolidation was executed.";
            break;
        default:
            message = "Unknown exception.";
            break;
        }
        throw Utils::Exceptions::WindowsException(code, message);
    }
}

Utils::Exceptions::WindowsException::WindowsException(DWORD code, LPCSTR message)
    : std::runtime_error(message), m_code(code)
{
}

DWORD Utils::Exceptions::WindowsException::code() const
{
    return m_code;
}

void Utils::Exceptions::InitWindowsExceptionsHandler()
{
    _set_se_translator(WindowsExceptionHandler);
}
