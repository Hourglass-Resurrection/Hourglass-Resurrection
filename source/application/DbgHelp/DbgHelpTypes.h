/*
 * Copyright (c) 2016- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#include <cassert>
#include <string>
#include <variant>

/*
 * This line triggers the C4455 warning erronously due to the following bug:
 * https://connect.microsoft.com/VisualStudio/feedback/details/3049481/c4455-on-standard-string-literal-operators
 */
#pragma warning(push)
#pragma warning(disable : 4455)
using std::string_literals::operator""s;
#pragma warning(pop)

/*
 * This struct contains all basic types which have a defined size.
 */
class DbgHelpBasicType
{
public:
    enum class BasicType
    {
        Void,
        Char,
        WideChar,
        Char16,
        Char32,
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

    DbgHelpBasicType(BasicType type) : m_type(type)
    {
    }

    std::wstring GetName() const
    {
        switch (m_type)
        {
        case BasicType::Void:
            return L"void"s;
        case BasicType::Char:
            return L"char"s;
        case BasicType::WideChar:
            return L"wchar_t"s;
        case BasicType::Char16:
            return L"char16_t"s;
        case BasicType::Char32:
            return L"char32_t"s;
        case BasicType::Int8:
            return L"s8"s;
        case BasicType::UnsignedInt8:
            return L"u8"s;
        case BasicType::Int16:
            return L"s16"s;
        case BasicType::UnsignedInt16:
            return L"u16"s;
        case BasicType::Int32:
            return L"s32"s;
        case BasicType::UnsignedInt32:
            return L"u32"s;
        case BasicType::Int64:
            return L"s64"s;
        case BasicType::UnsignedInt64:
            return L"u64"s;
        case BasicType::Float:
            return L"float"s;
        case BasicType::Double:
            return L"double"s;
        default:
            assert(false);
            return 0;
        }
    }

    size_t GetSize() const
    {
        switch (m_type)
        {
        case BasicType::Void:
            return 0;
        case BasicType::Char:
            return sizeof(char);
        case BasicType::WideChar:
            return sizeof(wchar_t);
        case BasicType::Char16:
            return sizeof(char16_t);
        case BasicType::Char32:
            return sizeof(char32_t);
        case BasicType::Int8:
            return sizeof(int8_t);
        case BasicType::UnsignedInt8:
            return sizeof(uint8_t);
        case BasicType::Int16:
            return sizeof(int16_t);
        case BasicType::UnsignedInt16:
            return sizeof(uint16_t);
        case BasicType::Int32:
            return sizeof(int32_t);
        case BasicType::UnsignedInt32:
            return sizeof(uint32_t);
        case BasicType::Int64:
            return sizeof(int64_t);
        case BasicType::UnsignedInt64:
            return sizeof(uint64_t);
        case BasicType::Float:
            return sizeof(float);
        case BasicType::Double:
            return sizeof(double);
        default:
            assert(false);
            return 0;
        }
    }
};

/*
 * An unknown type with a known size. Might have an available name.
 */
class DbgHelpUnknownType
{
public:
    const size_t m_size;
    const std::optional<std::wstring> m_name;

    DbgHelpUnknownType(size_t size, std::optional<std::wstring> name = std::nullopt)
        : m_size(size), m_name(name)
    {
    }

    std::wstring GetName() const
    {
        return m_name.value_or(L"unknown"s);
    }

    size_t GetSize() const
    {
        return m_size;
    }
};

/*
 * An unknown type with an unknown size. Might have an available name.
 */
class DbgHelpUnknownUnsizedType
{
public:
    const std::optional<std::wstring> m_name;

    DbgHelpUnknownUnsizedType(std::optional<std::wstring> name = std::nullopt) : m_name(name)
    {
    }

    std::wstring GetName() const
    {
        return m_name.value_or(L"unknown"s);
    }
};

/*
 * An enum of either a basic type or an unknown type.
 */
class DbgHelpEnumType
{
public:
    const std::variant<DbgHelpBasicType, DbgHelpUnknownType> m_underlying_type;
    const std::optional<std::wstring> m_name;

    DbgHelpEnumType(DbgHelpBasicType underlying_type, std::optional<std::wstring> name)
        : m_underlying_type(underlying_type), m_name(name)
    {
    }
    DbgHelpEnumType(DbgHelpUnknownType underlying_type, std::optional<std::wstring> name)
        : m_underlying_type(underlying_type), m_name(name)
    {
    }

    std::wstring GetName() const
    {
        return std::visit(
            [this](const auto& type) {
                std::wstring rv = L"enum";

                if (m_name.has_value())
                {
                    rv += L' ' + m_name.value();
                }

                rv += L" : " + type.GetName();
                return rv;
            },
            m_underlying_type);
    }

    size_t GetSize() const
    {
        return std::visit([](const auto& type) { return type.GetSize(); }, m_underlying_type);
    }
};

/*
 * A pointer to either a basic type, an unknown type or an enum.
 * This doesn't cleanly support multiple level deep pointers at the moment.
 */
class DbgHelpPointerType
{
public:
    // clang-format off
    const std::variant<DbgHelpBasicType,
                       DbgHelpUnknownType,
                       DbgHelpEnumType,
                       DbgHelpUnknownUnsizedType> m_underlying_type;
    // clang-format on

    /*
     * Size of the pointer type (differs between 32-bit and 64-bit targets).
     */
    const size_t m_size;

    DbgHelpPointerType(DbgHelpBasicType underlying_type, size_t size)
        : m_underlying_type(underlying_type), m_size(size)
    {
    }
    DbgHelpPointerType(DbgHelpUnknownType underlying_type, size_t size)
        : m_underlying_type(underlying_type), m_size(size)
    {
    }
    DbgHelpPointerType(DbgHelpEnumType underlying_type, size_t size)
        : m_underlying_type(underlying_type), m_size(size)
    {
    }
    DbgHelpPointerType(DbgHelpUnknownUnsizedType underlying_type, size_t size)
        : m_underlying_type(underlying_type), m_size(size)
    {
    }

    std::wstring GetName() const
    {
        return std::visit(
            [](const auto& type) {
                /*
             * L" *"s somehow becomes garbage data. Can't replicate in a test program.
             * -- YaLTeR
             */
                return type.GetName() + L" *";
            },
            m_underlying_type);
    }

    size_t GetSize() const
    {
        return m_size;
    }
};

/*
 * The type is either one of the basic types, an unknown type, a pointer or an enum.
 */
class DbgHelpType
{
public:
    const std::variant<DbgHelpBasicType, DbgHelpPointerType, DbgHelpUnknownType, DbgHelpEnumType>
        m_type;

    DbgHelpType(DbgHelpBasicType type) : m_type(type)
    {
    }
    DbgHelpType(DbgHelpPointerType type) : m_type(type)
    {
    }
    DbgHelpType(DbgHelpUnknownType type) : m_type(type)
    {
    }
    DbgHelpType(DbgHelpEnumType type) : m_type(type)
    {
    }

    std::wstring GetName() const
    {
        return std::visit([](const auto& type) { return type.GetName(); }, m_type);
    }

    size_t GetSize() const
    {
        return std::visit([](const auto& type) { return type.GetSize(); }, m_type);
    }
};
