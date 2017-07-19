/*
 * Copyright (c) 2016- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#include <string>
#include <variant>

using std::string_literals::operator""s;

// This struct contains all basic types which have a defined size.
// This doesn't include types like wchar_t without a defined size,
// or types like long double for which no basic type exists in MSVC.
class DbgHelpBasicType
{
public:
    enum class BasicType {
        Char,
        Int8,
        UnsignedInt8,
        Int16,
        UnsignedInt16,
        Int32,
        UnsignedInt32,
        Int64,
        UnsignedInt64,
        Float,
        Double,
    };

    const BasicType m_type;

    DbgHelpBasicType(BasicType type) : m_type(type) {}

    std::wstring GetName() const
    {
        switch (m_type)
        {
        case BasicType::Char:
            return L"char"s;
        case BasicType::Int8:
            return L"int8_t"s;
        case BasicType::UnsignedInt8:
            return L"uint8_t"s;
        case BasicType::Int16:
            return L"int16_t"s;
        case BasicType::UnsignedInt16:
            return L"uint16_t"s;
        case BasicType::Int32:
            return L"int32_t"s;
        case BasicType::UnsignedInt32:
            return L"uint32_t"s;
        case BasicType::Int64:
            return L"int64_t"s;
        case BasicType::UnsignedInt64:
            return L"uint64_t"s;
        case BasicType::Float:
            return L"float"s;
        case BasicType::Double:
            return L"double"s;
        }
    }

    size_t GetSize() const
    {
        switch (m_type)
        {
        case BasicType::Char:
            return 1;
        case BasicType::Int8:
            return 1;
        case BasicType::UnsignedInt8:
            return 1;
        case BasicType::Int16:
            return 2;
        case BasicType::UnsignedInt16:
            return 2;
        case BasicType::Int32:
            return 4;
        case BasicType::UnsignedInt32:
            return 4;
        case BasicType::Int64:
            return 8;
        case BasicType::UnsignedInt64:
            return 8;
        case BasicType::Float:
            return 4;
        case BasicType::Double:
            return 8;
        }
    }
};

// An unknown type with a known size.
class DbgHelpUnknownType
{
public:
    const size_t m_size;

    DbgHelpUnknownType(size_t size) : m_size(size) {}

    std::wstring GetName() const
    {
        // TODO: test when this happens, maybe return "void" makes more sense.
        return L"unknown"s;
    }

    size_t GetSize() const
    {
        return m_size;
    }
};

// An enum of either a basic type or an unknown type.
class DbgHelpEnumType
{
public:
    const std::variant<DbgHelpBasicType, DbgHelpUnknownType> m_underlying_type;
    const std::optional<std::wstring> m_name;

    DbgHelpEnumType(DbgHelpBasicType underlying_type, std::optional<std::wstring> name)
        : m_underlying_type(underlying_type), m_name(name) {}
    DbgHelpEnumType(DbgHelpUnknownType underlying_type, std::optional<std::wstring> name)
        : m_underlying_type(underlying_type), m_name(name) {}

    std::wstring GetName() const
    {
        return std::visit([this](const auto& type) {
            std::wstring rv = L"enum";

            if (m_name.has_value())
            {
                rv += L' ' + m_name.value();
            }

            rv += L" : " + type.GetName();
            return rv;
        }, m_underlying_type);
    }

    size_t GetSize() const
    {
        return std::visit([](const auto& type) {
            return type.GetSize();
        }, m_underlying_type);
    }
};

// A pointer to either a basic type, an unknown type or an enum.
// This doesn't cleanly support multiple level deep pointers at the moment.
class DbgHelpPointerType
{
public:
    const std::variant<DbgHelpBasicType, DbgHelpUnknownType, DbgHelpEnumType> m_underlying_type;

    // Size of the pointer type (differs between 32-bit and 64-bit targets).
    const size_t m_size;

    DbgHelpPointerType(DbgHelpBasicType underlying_type, size_t size)
        : m_underlying_type(underlying_type), m_size(size) {}
    DbgHelpPointerType(DbgHelpUnknownType underlying_type, size_t size)
        : m_underlying_type(underlying_type), m_size(size) {}
    DbgHelpPointerType(DbgHelpEnumType underlying_type, size_t size)
        : m_underlying_type(underlying_type), m_size(size) {}

    std::wstring GetName() const
    {
        return std::visit([](const auto& type) {
            return type.GetName() + L" *"; // L" *"s somehow becomes garbage data. Can't replicate in a test program.
        }, m_underlying_type);
    }

    size_t GetSize() const
    {
        return m_size;
    }
};

// The type is either one of the basic types, an unknown type, a pointer or an enum.
class DbgHelpType
{
public:
    const std::variant<DbgHelpBasicType, DbgHelpPointerType, DbgHelpUnknownType, DbgHelpEnumType> m_type;

    DbgHelpType(DbgHelpBasicType type) : m_type(type) {}
    DbgHelpType(DbgHelpPointerType type) : m_type(type) {}
    DbgHelpType(DbgHelpUnknownType type) : m_type(type) {}
    DbgHelpType(DbgHelpEnumType type) : m_type(type) {}

    std::wstring GetName() const
    {
        return std::visit([](const auto& type) {
            return type.GetName();
        }, m_type);
    }

    size_t GetSize() const
    {
        return std::visit([](const auto& type) {
            return type.GetSize();
        }, m_type);
    }
};
