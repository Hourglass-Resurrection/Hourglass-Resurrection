/*
 * Copyright (c) 2016- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <sstream>
#include <string>

#include <atlbase.h>
#include <atlcom.h>

#include "application/logging.h"
#include "DbgHelpPrivate.h"
#include "DbgHelpStackWalkCallback.h"

namespace
{
    // In these functions length MUST match the "expected" size of the basic types,
    // otherwise they will be read incorrectly.
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
        // Treat wchar_t, char16_t and char32_t as integers of the given size as they don't have a defined size.
        case btWChar:
        case btChar16:
        case btChar32:
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

        // Everything we don't know about / don't handle yet.
        return std::nullopt;
    }

    DbgHelpType GetDbgHelpType(ULONGLONG length, DWORD type_tag, CComPtr<IDiaSymbol> type_info)
    {
        DbgHelpUnknownType default_return_value = DbgHelpUnknownType(length);

        DWORD type;
        switch (type_tag)
        {
        case SymTagBaseType:
            if (type_info->get_baseType(&type) == S_OK)
            {
                // Convert the type.
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
                            // This complicated std::visit() expression converts DbgHelpType
                            // to DbgHelpBasicType or DbgHelpUnknownType.

#pragma message(__FILE__ ": TODO: change to constexpr-if lambdas when they are supported (VS2017 Preview 3)")
                            // This helper type is required until VS2017.3 with constexpr ifs.
                            class visitor
                            {
                                size_t m_length;

                            public:
                                visitor(size_t length) : m_length(length) {}

                                DbgHelpPointerType operator()(DbgHelpBasicType t)
                                {
                                    return DbgHelpPointerType(t, m_length);
                                }

                                DbgHelpPointerType operator()(DbgHelpPointerType t)
                                {
                                    // TODO: this is here until multi-level pointers are supported.
                                    // Treat the underlying pointer as an unsigned integer.
                                    return DbgHelpPointerType(GetDbgHelpBasicType(t.GetSize(), btUInt).value(), m_length);
                                }

                                DbgHelpPointerType operator()(DbgHelpUnknownType t)
                                {
                                    return DbgHelpPointerType(t, m_length);
                                }

                                DbgHelpPointerType operator()(DbgHelpEnumType t)
                                {
                                    return DbgHelpPointerType(t, m_length);
                                }
                            };

                            return std::visit(visitor(length),
                                GetDbgHelpType(underlying_length, underlying_type_tag, underlying_type).m_type);
                        }
                        else
                        {
                            return DbgHelpPointerType(DbgHelpUnknownType(underlying_length), length);
                        }
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
                    // Can only be DbgHelpUnknownType in this case.
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

                return DbgHelpUnknownType(length, name);
            }
            break;
        }

        // Everything we don't know about / don't handle yet.
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
}

// TODO: Split module name and PC
std::wstring DbgHelpStackWalkCallback::GetModuleName()
{
    std::wstringstream name;
    if (m_mod_info == nullptr)
    {
        name << L"?";
    }
    else
    {
        name << m_mod_info->m_module_name;
    }
    name << L"!0x" << std::hex << GetProgramCounter();
    return name.str();
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
    return symbol;
}

ULONGLONG DbgHelpStackWalkCallback::GetProgramCounter()
{
    ULONGLONG pc;
    m_frame->get_registerValue(CV_REG_EIP, &pc);
    return pc;
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
        IDiaSymbol* sym_info;
        ULONG num_fetched;

        /*
         * This will be incremented for each found symbol, with it's size.
         */
        DWORD offset = 0;
        for (DWORD count = 0; (enum_symbols->Next(1, &sym_info, &num_fetched) == S_OK && num_fetched == 1); count++)
        {
            CComPtr<IDiaSymbol> type_info;
            if (sym_info->get_type(&type_info) != S_OK)
            {
                /*
                 * This will filter out most non-argument children in the function
                 */
                sym_info->Release();
                continue;
            }
            DWORD kind;
            if (sym_info->get_dataKind(&kind) != S_OK || kind != DataIsParam)
            {
                sym_info->Release();
                continue;
            }
            DWORD symtag;
            if (sym_info->get_symTag(&symtag) != S_OK || symtag == SymTagCallSite)
            {
                sym_info->Release();
                continue;
            }
            DWORD typetag;
            if (type_info->get_symTag(&typetag) != S_OK)
            {
                sym_info->Release();
                continue;
            }
            ULONGLONG length = 0;
            if (type_info->get_length(&length) != S_OK)
            {
                sym_info->Release();
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
            // Temp debug stuff
            //debugprintf(L"Found argument %s with type tag %u\n", name, typetag);

            if (name != nullptr)
            {
                SysFreeString(name);
            }
            sym_info->Release();
        }
    }
    /*ULONGLONG stackframe_base = 0;
    m_frame->get_base(&stackframe_base);
    for (DWORD i = 0; i < 30; i++)
    {
        ULONGLONG address = stackframe_base + (i * 4);
        DWORD value;
        SIZE_T read_bytes;
        if (ReadProcessMemory(m_process, reinterpret_cast<LPVOID>(address), &value, sizeof(value), &read_bytes) == TRUE)
        {
            debugprintf(L"%#llX: %#X\n", address, value);
        }
    }*/
    m_params_enumerated = true;
}

std::optional<IDbgHelpStackWalkCallback::ParameterValue>
DbgHelpStackWalkCallback::GetParameterValue(const ParamInfo& param_info)
{
    ULONGLONG stackframe_base = 0;
    if (m_frame->get_base(&stackframe_base) != S_OK)
    {
        return std::nullopt;
    }

#pragma message(__FILE__ ": TODO: change to constexpr-if lambdas when they are supported (VS2017 Preview 3)")
    // First, value initialize the variant we're going to read bytes into.
    class visitor
    {
    public:
        std::optional<IDbgHelpStackWalkCallback::ParameterValue> operator()(DbgHelpBasicType t)
        {
            switch (t.m_type)
            {
            case DbgHelpBasicType::BasicType::Void:
                return std::nullopt;
            case DbgHelpBasicType::BasicType::Char:
                return char{};
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
            }
        }

        std::optional<IDbgHelpStackWalkCallback::ParameterValue> operator()(DbgHelpPointerType t)
        {
            return static_cast<void*>(nullptr);
        }

        std::optional<IDbgHelpStackWalkCallback::ParameterValue> operator()(DbgHelpEnumType t)
        {
            return std::visit(*this, t.m_underlying_type);
        }

        std::optional<IDbgHelpStackWalkCallback::ParameterValue> operator()(DbgHelpUnknownType t)
        {
            // We don't get the values of unknown types.
            return std::nullopt;
        }
    };

    std::optional<IDbgHelpStackWalkCallback::ParameterValue> value = std::visit(visitor(), param_info.m_type.m_type);

    // Now, read the bytes.
    // I'm really missing all the support functions for these types like map(), filter(), etc...
    if (value.has_value())
    {
        bool success = std::visit([this, &stackframe_base, &param_info](auto& t) {
            ULONGLONG address = stackframe_base + param_info.m_offset;
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
