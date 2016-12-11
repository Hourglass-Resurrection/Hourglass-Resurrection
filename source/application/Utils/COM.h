/*
 * Copyright (c) 2016- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <memory>

namespace Utils
{
    namespace COM
    {
        /*
         * Custom unique_ptr deleter for COM objects.
         */
        template<class T>
        class COMObjectDeleter
        {
        public:
            void operator()(T* ptr)
            {
                ptr->Release();
            }
        };

        template<class T>
        using UniqueCOMPtr = std::unique_ptr<T, COMObjectDeleter<T>>;

        template<class T>
        UniqueCOMPtr<T> MakeUniqueCOMPtr(REFIID class_id)
        {
            T* result = nullptr;
            if (CoCreateInstance(class_id, nullptr, CLSCTX_INPROC_SERVER, __uuidof(T),
                                 reinterpret_cast<LPVOID*>(&result)) != S_OK)
            {
                throw std::bad_alloc();
            }
            return std::move(UniqueCOMPtr<T>(result));
        }
    }
}
