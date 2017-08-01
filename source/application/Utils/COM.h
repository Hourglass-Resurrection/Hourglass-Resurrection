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
        template<class T>
        CComPtr<T> CreateCOMPtr(REFIID class_id)
        {
            CComPtr<T> result;
            if (CoCreateInstance(class_id, nullptr, CLSCTX_INPROC_SERVER, __uuidof(T),
                                 reinterpret_cast<LPVOID*>(&result)) != S_OK)
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
            COMInstance() throw();
            ~COMInstance();
            COMInstance(const COMInstance&) = delete;
            void operator=(const COMInstance&) = delete;

            static bool ms_initialized;
        };
    }
}
