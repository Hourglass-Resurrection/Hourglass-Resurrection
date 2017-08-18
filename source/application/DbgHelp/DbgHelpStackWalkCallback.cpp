/*
 * Copyright (c) 2016- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <cassert>
#include <string>

#include <atlbase.h>
#include <atlcom.h>

#include "application/logging.h"
#include "DbgHelpPrivate.h"
#include "DbgHelpStackWalkCallback.h"
#include "DiaEnumIterator.h"

namespace
{
    /*
     * In these functions length MUST match the "expected" size of the basic types (see DbgHelpBasicType::GetSize()),
     * otherwise they will be read incorrectly.
     * -- YaLTeR
     */
    std::optional<DbgHelpBasicType::BasicType> GetDbgHelpIntType(ULONGLONG length, bool is_signed)
    {
        switch (length)
        {
        case 1:
            return is_signed ? DbgHelpBasicType::BasicType::Int8 : DbgHelpBasicType::BasicType::UnsignedInt8;
        case 2:
            return is_signed ? DbgHelpBasicType::BasicType::Int16 : DbgHelpBasicType::BasicType::UnsignedInt16;
        case 4:
            return is_signed ? DbgHelpBasicType::BasicType::Int32 : DbgHelpBasicType::BasicType::UnsignedInt32;
        case 8:
            return is_signed ? DbgHelpBasicType::BasicType::Int64 : DbgHelpBasicType::BasicType::UnsignedInt64;
        default:
            return std::nullopt;
        }
    }

    std::optional<DbgHelpBasicType::BasicType> GetDbgHelpFloatType(ULONGLONG length)
    {
        switch (length)
        {
        case 4:
            return DbgHelpBasicType::BasicType::Float;
        case 8:
            return DbgHelpBasicType::BasicType::Double;
        default:
            return std::nullopt;
        }
    }

    std::optional<DbgHelpBasicType> GetDbgHelpBasicType(ULONGLONG length, DWORD basic_type)
    {
        switch (basic_type)
        {
        case btVoid:
            return DbgHelpBasicType(DbgHelpBasicType::BasicType::Void);
        case btChar:
            return DbgHelpBasicType(DbgHelpBasicType::BasicType::Char);
        case btInt:
        case btLong:
            {
                auto int_type = GetDbgHelpIntType(length, true);
                if (int_type.has_value())
                {
                    return DbgHelpBasicType(int_type.value());
                }
            }
            break;

        /*
         * This code assumes that lengths of the extra char types are the same as sizeof's of the respective types.
         * -- YaLTeR
         */
        case btWChar:
            static_assert(sizeof(wchar_t) == 2, "time to implement handling of different wchar_t sizes");
            return DbgHelpBasicType(DbgHelpBasicType::BasicType::WideChar);
        case btChar16:
            static_assert(sizeof(char16_t) == 2, "time to implement handling of different char16_t sizes");
            return DbgHelpBasicType(DbgHelpBasicType::BasicType::Char16);
        case btChar32:
            static_assert(sizeof(char32_t) == 4, "time to implement handling of different char32_t sizes");
            return DbgHelpBasicType(DbgHelpBasicType::BasicType::Char32);

        case btUInt:
        case btULong:
            {
                auto int_type = GetDbgHelpIntType(length, false);
                if (int_type.has_value())
                {
                    return DbgHelpBasicType(int_type.value());
                }
            }
            break;
        case btFloat:
            {
                auto float_type = GetDbgHelpFloatType(length);
                if (float_type.has_value())
                {
                    return DbgHelpBasicType(float_type.value());
                }
            }
            break;
        }

        /*
         * Everything we don't know about / don't handle yet.
         */
        return std::nullopt;
    }

    DbgHelpType GetDbgHelpType(ULONGLONG length, DWORD type_tag, CComPtr<IDiaSymbol> type_info)
    {
        DbgHelpUnknownType default_return_value = DbgHelpUnknownType(static_cast<size_t>(length));

        DWORD type;
        switch (type_tag)
        {
        case SymTagBaseType:
            if (type_info->get_baseType(&type) == S_OK)
            {
                /*
                 * Convert the type.
                 */
                std::optional<DbgHelpType> rv = GetDbgHelpBasicType(length, type);
                return rv.value_or(default_return_value);
            }
            break;

        case SymTagPointerType:
            {
                CComPtr<IDiaSymbol> underlying_type;
                if (type_info->get_type(&underlying_type) == S_OK)
                {
                    ULONGLONG underlying_length;
                    if (underlying_type->get_length(&underlying_length) == S_OK)
                    {
                        DWORD underlying_type_tag;
                        if (underlying_type->get_symTag(&underlying_type_tag) == S_OK)
                        {
                            auto type = GetDbgHelpType(underlying_length, underlying_type_tag, underlying_type).m_type;

                            return std::visit([&length](auto&& type) {
                                using T = std::decay_t<decltype(type)>;

                                if constexpr (std::is_same_v<T, DbgHelpPointerType>)
                                {
                                    /*
                                     * TODO: this is here until multi-level pointers are supported.
                                     * Treat the underlying pointer as an unsigned integer.
                                     * -- YaLTeR
                                     */

                                    auto basic_type = GetDbgHelpBasicType(type.GetSize(), btUInt).value();
                                    return DbgHelpPointerType(basic_type, static_cast<size_t>(length));
                                }
                                else
                                {
                                    return DbgHelpPointerType(type, static_cast<size_t>(length));
                                }
                            }, type);
                        }
                        else
                        {
                            return DbgHelpPointerType(DbgHelpUnknownType(static_cast<size_t>(underlying_length)),
                                                      static_cast<size_t>(length));
                        }
                    }
                    else
                    {
                        /*
                         * The underlying type has unknown length. Perhaps we can at least retrieve the name.
                         *
                         * TODO: The only case of this I stumbled upon so far is SymTagFunctionType.
                         *       It doesn't have a name, but it can be printed. This requires more work though.
                         * -- YaLTeR
                         */
                        std::optional<std::wstring> name;

                        DWORD underlying_type_tag;
                        if (underlying_type->get_symTag(&underlying_type_tag) == S_OK)
                        {
                            BSTR name_bstr;
                            if (type_info->get_name(&name_bstr) == S_OK)
                            {
                                name = name_bstr;
                                SysFreeString(name_bstr);
                            }
                        }

                        return DbgHelpPointerType(DbgHelpUnknownUnsizedType(name), static_cast<size_t>(length));
                    }
                }
            }
            break;

        case SymTagEnum:
            {
                std::optional<std::wstring> name;

                BSTR name_bstr;
                if (type_info->get_name(&name_bstr) == S_OK)
                {
                    name = name_bstr;
                    SysFreeString(name_bstr);
                }

                DbgHelpType underlying_type = GetDbgHelpType(length, SymTagBaseType, type_info);

                if (std::holds_alternative<DbgHelpBasicType>(underlying_type.m_type))
                {
                    return DbgHelpEnumType(std::get<DbgHelpBasicType>(underlying_type.m_type), name);
                }
                else
                {
                    /*
                     * Can only be DbgHelpUnknownType in this case.
                     */
                    return DbgHelpEnumType(std::get<DbgHelpUnknownType>(underlying_type.m_type), name);
                }
            }
            break;

        case SymTagUDT:
            {
                std::optional<std::wstring> name;

                BSTR name_bstr;
                if (type_info->get_name(&name_bstr) == S_OK)
                {
                    name = name_bstr;
                    SysFreeString(name_bstr);
                }

                return DbgHelpUnknownType(static_cast<size_t>(length), name);
            }
            break;
        }

        /*
         * Everything we don't know about / don't handle yet.
         */
        return default_return_value;
    }
}

DbgHelpStackWalkCallback::DbgHelpStackWalkCallback(HANDLE process, IDiaStackFrame* frame, const DbgHelpPrivate::ModuleData* mod_info) :
    m_process(process),
    m_frame(frame),
    m_mod_info(mod_info),
    m_unsure(0),
    m_params_enumerated(false)
{
    if (m_frame->get_base(&m_stack_frame_base) != S_OK)
        m_stack_frame_base = 0;
}

ULONGLONG DbgHelpStackWalkCallback::GetProgramCounter()
{
    ULONGLONG pc;
    m_frame->get_registerValue(CV_REG_EIP, &pc);
    return pc;
}

std::wstring DbgHelpStackWalkCallback::GetModuleName()
{
    if (m_mod_info == nullptr)
    {
        return L"?";
    }
    else
    {
        return m_mod_info->m_module_name;
    }
}

std::wstring DbgHelpStackWalkCallback::GetFunctionName()
{
    if (m_mod_info == nullptr)
    {
        return L"?";
    }
    std::wstring function_name = L"?";
    if (m_mod_info->m_module_symbol_session == nullptr)
    {
        auto it = m_mod_info->m_module_exports_table.lower_bound(GetProgramCounter());
        if (it != m_mod_info->m_module_exports_table.begin())
        {
            it--;
            function_name = it->second;
        }
        return function_name;
    }
    auto symbol = GetFunctionSymbol();
    
    BSTR name = nullptr;
    if (symbol != nullptr && symbol->get_name(&name) == S_OK)
    {
        function_name = name;
        SysFreeString(name);
    }
    return function_name;
}

DWORD DbgHelpStackWalkCallback::GetUnsureStatus()
{
    return m_unsure;
}

std::vector<DbgHelpStackWalkCallback::Parameter> DbgHelpStackWalkCallback::GetParameters()
{
    if (m_mod_info == nullptr)
    {
        return std::vector<Parameter>();
    }

    if (!m_params_enumerated)
    {
        EnumerateParameters();
    }

    std::vector<Parameter> parameters;
    parameters.reserve(m_param_info.size());

    size_t arg_index = 1;
    for (const ParamInfo& param_info : m_param_info)
    {
        parameters.emplace_back(param_info.m_type,
            param_info.m_name.value_or(L"arg"s + std::to_wstring(arg_index)),
            GetParameterValue(param_info));

        ++arg_index;
    }

    return parameters;
}

CComPtr<IDiaSymbol> DbgHelpStackWalkCallback::GetFunctionSymbol()
{
    CComPtr<IDiaSymbol> symbol;
    HRESULT rv = m_mod_info->m_module_symbol_session->findSymbolByVA(GetProgramCounter(), SymTagFunction, &symbol);

    if (rv != S_OK)
    {
        rv = m_mod_info->m_module_symbol_session->findSymbolByVA(GetProgramCounter(), SymTagPublicSymbol, &symbol);
        if (rv == S_OK)
        {
            BOOL is_function = false;
            rv = symbol->get_function(&is_function);
            if (!is_function)
            {
                rv = S_FALSE;
            }
        }
    }

    assert(rv == S_OK);

    return symbol;
}


void DbgHelpStackWalkCallback::EnumerateParameters()
{
    if (m_mod_info->m_module_symbol_session == nullptr)
    {
        /*
         * TODO: Look at callee asm to guess number of parameters. The get_lengthParams method 
         *       always returns 0 when no debug symbols are known...
         * Temp workaround: Assume 4 parameters (this is what the old implementation did).
         * -- Warepire
         */
        m_unsure++;
        for (DWORD i = 0; i < 4; i++)
        {
            m_param_info.emplace_back(ParamInfo{ DbgHelpBasicType(DbgHelpBasicType::BasicType::Int32),
                                                 std::nullopt, nullptr, nullptr, i * 4 });
        }
    }
    else
    {
        auto symbol = GetFunctionSymbol();
        CComPtr<IDiaEnumSymbols> enum_symbols;
        HRESULT rv = symbol->findChildren(SymTagNull, nullptr, nsNone, &enum_symbols);
        if (enum_symbols == nullptr)
        {
            return;
        }

        /*
         * This will be incremented for each found symbol, with it's size.
         * The parameters start at offset = 4.
         * -- YaLTeR
         */
        DWORD offset = 4;

        for (const auto& sym_info : enum_symbols)
        {
            CComPtr<IDiaSymbol> type_info;
            if (sym_info->get_type(&type_info) != S_OK)
            {
                continue;
            }
            DWORD kind;
            if (sym_info->get_dataKind(&kind) != S_OK || kind != DataIsParam)
            {
                continue;
            }
            DWORD symtag;
            if (sym_info->get_symTag(&symtag) != S_OK || symtag == SymTagCallSite)
            {
                continue;
            }
            DWORD typetag;
            if (type_info->get_symTag(&typetag) != S_OK)
            {
                continue;
            }
            ULONGLONG length = 0;
            if (type_info->get_length(&length) != S_OK)
            {
                continue;
            }

            DbgHelpType arg_type = GetDbgHelpType(length, typetag, type_info);

            BSTR name;
            if (sym_info->get_name(&name) != S_OK)
            {
                name = nullptr;
            }

            /*
             * The cast should be safe, if a parameter on the stack needs more than 4 GB of memory
             * something else has probably gone really awry.
             * -- Warepire
             */
            m_param_info.emplace_back(ParamInfo{ arg_type,
                                                 name != nullptr
                                                     ? std::make_optional(std::wstring(name)) : std::nullopt,
                                                 sym_info, type_info, offset });
            offset += static_cast<DWORD>(length);

            if (name != nullptr)
            {
                SysFreeString(name);
            }
        }
    }
    m_params_enumerated = true;
}

std::optional<IDbgHelpStackWalkCallback::ParameterValue>
DbgHelpStackWalkCallback::GetParameterValue(const ParamInfo& param_info)
{
    if (m_stack_frame_base == 0)
    {
        return std::nullopt;
    }

    /*
     * First, value initialize the variant we're going to read bytes into.
     */
    auto visitor = [](auto&& type) {
        using T = std::decay_t<decltype(type)>;

        auto inner = [](auto&& type) -> std::optional<IDbgHelpStackWalkCallback::ParameterValue> {
            using T = std::decay_t<decltype(type)>;

            if constexpr (std::is_same_v<T, DbgHelpPointerType>)
            {
                return static_cast<void*>(nullptr);
            }
            else if constexpr (std::is_same_v<T, DbgHelpUnknownType>)
            {
                /*
                 * We don't get the values of unknown types.
                 */
                return std::nullopt;
            }
            else if constexpr (std::is_same_v<T, DbgHelpBasicType>)
            {
                switch (type.m_type)
                {
                case DbgHelpBasicType::BasicType::Void:
                    return std::nullopt;
                case DbgHelpBasicType::BasicType::Char:
                    return char{};
                case DbgHelpBasicType::BasicType::WideChar:
                    return wchar_t{};
                case DbgHelpBasicType::BasicType::Char16:
                    return char16_t{};
                case DbgHelpBasicType::BasicType::Char32:
                    return char32_t{};
                case DbgHelpBasicType::BasicType::Int8:
                    return int8_t{};
                case DbgHelpBasicType::BasicType::UnsignedInt8:
                    return uint8_t{};
                case DbgHelpBasicType::BasicType::Int16:
                    return int16_t{};
                case DbgHelpBasicType::BasicType::UnsignedInt16:
                    return uint16_t{};
                case DbgHelpBasicType::BasicType::Int32:
                    return int32_t{};
                case DbgHelpBasicType::BasicType::UnsignedInt32:
                    return uint32_t{};
                case DbgHelpBasicType::BasicType::Int64:
                    return int64_t{};
                case DbgHelpBasicType::BasicType::UnsignedInt64:
                    return uint64_t{};
                case DbgHelpBasicType::BasicType::Float:
                    return float{};
                case DbgHelpBasicType::BasicType::Double:
                    return double{};
                default:
                    assert(false);
                    return std::nullopt;
                }
            }
            else
            {
                /*
                 * This case is handled below.
                 */
                assert(false);
                return std::nullopt;
            }
        };

        if constexpr (std::is_same_v<T, DbgHelpEnumType>)
        {
            return std::visit(inner, type.m_underlying_type);
        }
        else
        {
            return inner(type);
        }
    };

    std::optional<IDbgHelpStackWalkCallback::ParameterValue> value = std::visit(visitor, param_info.m_type.m_type);

    /*
     * Now, read the bytes.
     */
    if (value.has_value())
    {
        bool success = std::visit([this, &param_info](auto& t) {
            ULONGLONG address = m_stack_frame_base + param_info.m_offset;
            SIZE_T read_bytes;

            return ReadProcessMemory(m_process,
                                     reinterpret_cast<LPVOID>(address),
                                     &t,
                                     sizeof(t),
                                     &read_bytes) == TRUE
                && read_bytes == sizeof(t);
        }, *value);

        if (success)
        {
            return value;
        }
        else
        {
            return std::nullopt;
        }
    }

    return std::nullopt;
}
