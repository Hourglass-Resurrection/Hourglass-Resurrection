/*
 * Copyright (c) 2016- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <atlbase.h>
#include <atlcom.h>

namespace Utils
{
    namespace COM
    {
        using DllGetClassObject_t = HRESULT(__stdcall*)(REFCLSID rclsid, REFIID riid, LPVOID* ppv);

        template<class T>
        CComPtr<T> CreateCOMPtr(REFIID class_id)
        {
            CComPtr<T> result;
            if (CoCreateInstance(class_id,
                                 nullptr,
                                 CLSCTX_INPROC_SERVER,
                                 __uuidof(T),
                                 reinterpret_cast<LPVOID*>(&result))
                != S_OK)
            {
                throw std::bad_alloc();
            }
            return result;
        }

        class COMInstance
        {
        public:
            static void Init();

        private:
            COMInstance();
            ~COMInstance();
            COMInstance(const COMInstance&) = delete;
            void operator=(const COMInstance&) = delete;

            static bool ms_initialized;
        };

        class COMLibrary
        {
        public:
            COMLibrary();
            ~COMLibrary();

            void Load(LPCWSTR name);

            template<class T>
            inline HRESULT DllGetClassObject(REFCLSID rclsid, REFIID riid, T* ppv) const
            {
                return m_dll_get_class_object(rclsid, riid, reinterpret_cast<LPVOID*>(ppv));
            }

            template<class T>
            CComPtr<T> CreateCOMPtr(REFIID class_id) const
            {
                CComPtr<IClassFactory> factory;
                if (DllGetClassObject(class_id, IID_IClassFactory, &factory) != S_OK)
                {
                    throw std::exception("The requested class is not present in this library!");
                }

                CComPtr<T> result;
                if (factory->CreateInstance(nullptr,
                                            __uuidof(T),
                                            reinterpret_cast<LPVOID*>(&result))
                    != S_OK)
                {
                    throw std::exception("Error creating a class instance!");
                }

                return result;
            }

        private:
            HMODULE m_handle;
            DllGetClassObject_t m_dll_get_class_object;
        };
    }
}
