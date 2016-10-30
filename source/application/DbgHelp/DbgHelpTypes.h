/*
 * Copyright (c) 2016- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

enum class DbgHelpArgType
{
    Unknown,
    Char,
    WideChar,
    Char16,
    Char32,
    Short,
    UnsignedShort,
    Int,
    UnsignedInt,
    Long,
    UnsignedLong,
    LongLong,
    UnsignedLongLong,
    Float,
    Double,
    LongDouble,
    /*
     * These 3 are special, in the sense that they have "sub types".
     */
    Pointer,
    Struct,
    Union,
};
