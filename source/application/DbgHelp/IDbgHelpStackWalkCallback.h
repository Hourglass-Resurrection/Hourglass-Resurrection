/*
 * Copyright (c) 2016- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <optional>
#include <string>
#include <variant>
#include <vector>

#include "DbgHelpTypes.h"

class IDbgHelpStackWalkCallback
{
public:
    enum class Action
    {
        CONTINUE,
        STOP,
    };

    using ParameterValue = std::variant<char,
                                        wchar_t,
                                        char16_t,
                                        char32_t,
                                        int8_t,
                                        uint8_t,
                                        int16_t,
                                        uint16_t,
                                        int32_t,
                                        uint32_t,
                                        int64_t,
                                        uint64_t,
                                        float,
                                        double,
                                        void*>;

    /*
     * TODO: Is there any good way to connect m_type to the value type?
     */
    class Parameter
    {
    public:
        DbgHelpType m_type;
        std::wstring m_name;
        std::optional<ParameterValue> m_value;

        Parameter(DbgHelpType type, std::wstring name, std::optional<ParameterValue> value)
            : m_type(type), m_name(name), m_value(value) {}
    };

    virtual ~IDbgHelpStackWalkCallback() = default;

    virtual std::wstring GetModuleName() = 0;
    virtual std::wstring GetFunctionName() = 0;
    virtual std::vector<Parameter> GetParameters() = 0;
    virtual DWORD GetUnsureStatus() = 0;
};
