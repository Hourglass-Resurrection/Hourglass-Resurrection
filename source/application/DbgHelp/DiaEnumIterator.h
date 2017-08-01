/*
 * Copyright (c) 2017- Hourglass Resurrection Team
 * Hourglass Resurrection is licensed under GPL v2.
 * Refer to the file COPYING.txt in the project root.
 */

/*
 * Wrapper around enumerators (like IDiaEnumSymbols) that provides a safe C++ iterator interface and allows them to be
 * used in range-based for loops.
 */
#pragma once
#include <type_traits>

#include <DIA SDK/include/dia2.h>

namespace DiaEnumIteratorImpl
{
    /*
     * This little bit of template magic figures out the item type for a given DIA enumerator.
     * -- YaLTeR
     */
    template<class Enumerator, class Item, class IndexType>
    Item* ItemTypeHelper(HRESULT(__stdcall Enumerator::*)(IndexType, Item**));

    template<class Enumerator>
    using item_type = std::remove_pointer_t<decltype(ItemTypeHelper(&Enumerator::Item))>;

    static_assert(std::is_same_v<item_type<IDiaEnumSymbols>, IDiaSymbol>);

    /*
     * A helper function that returns the count of elements in a DIA enumerator, or zero on failure.
     */
    template<class Enumerator>
    inline LONG GetCount(Enumerator* enumerator)
    {
        LONG count;
        if (enumerator->get_Count(&count) == S_OK)
        {
            return count;
        }
        else
        {
            return 0;
        }
    }

    /*
     * Helper methods to deal with the fact that some enumerators accept a VARIANT as index, and some a DWORD.
     */
    template<class Enumerator, class Item>
    inline HRESULT CallItem(Enumerator* enumerator,
        HRESULT(__stdcall Enumerator::*method)(DWORD, Item**),
        DWORD index,
        Item** item)
    {
        return (enumerator->*method)(index, item);
    }

    template<class Enumerator, class Item>
    inline HRESULT CallItem(Enumerator* enumerator,
        HRESULT(__stdcall Enumerator::*method)(VARIANT, Item**),
        DWORD index,
        Item** item)
    {
        VARIANT v;
        v.vt = VT_INT;
        v.intVal = index;

        return (enumerator->*method)(v, item);
    }

    template<class Enumerator, class Item>
    inline HRESULT ItemByIndex(Enumerator* enumerator, DWORD index, Item** item)
    {
        return CallItem(enumerator, &Enumerator::Item, index, item);
    }

    /*
     * An enum for representing the iterator's starting state.
     */
    enum class StartingState
    {
        Begin,
        End
    };

    /*
     * The iterator itself, implementing prefix ++, * and != as needed for the range-based for.
     */
    template<class Enumerator>
    class DiaEnumIterator
    {
    public:
        using item_type = item_type<Enumerator>;

        DiaEnumIterator(CComPtr<Enumerator> enumerator, StartingState starting_state)
            : m_enumerator(enumerator)
            , m_count(GetCount<Enumerator>(enumerator))
            , m_next_index(starting_state == StartingState::Begin ? 0 : m_count)
            , m_last_item(nullptr)
        {
            GetNextItem();
        }

        inline const CComPtr<item_type>& operator++()
        {
            GetNextItem();
            return m_last_item;
        }

        inline const CComPtr<item_type>& operator*()
        {
            return m_last_item;
        }

        inline bool operator!=(const DiaEnumIterator<Enumerator>& rhs)
        {
            return m_last_item != rhs.m_last_item;
        }

    private:
        const CComPtr<Enumerator> m_enumerator;
        const ULONG m_count;
        DWORD m_next_index;
        CComPtr<item_type> m_last_item;

        void GetNextItem()
        {
            m_last_item = nullptr;

            if (m_next_index == m_count)
            {
                return;
            }

            item_type* item;
            if (ItemByIndex<Enumerator, item_type>(m_enumerator, m_next_index++, &item) == S_OK)
            {
                m_last_item = item;
                item->Release();
            }
        }
    };
}

/*
 * The begin() and end() functions that the range-based for loop uses.
 */
template<class Enumerator>
DiaEnumIteratorImpl::DiaEnumIterator<Enumerator> begin(Enumerator* enumerator)
{
    return DiaEnumIteratorImpl::DiaEnumIterator<Enumerator>(enumerator, DiaEnumIteratorImpl::StartingState::Begin);
}

template<class Enumerator>
DiaEnumIteratorImpl::DiaEnumIterator<Enumerator> end(Enumerator* enumerator)
{
    return DiaEnumIteratorImpl::DiaEnumIterator<Enumerator>(enumerator, DiaEnumIteratorImpl::StartingState::End);
}

template<class Enumerator>
DiaEnumIteratorImpl::DiaEnumIterator<Enumerator> begin(CComPtr<Enumerator> enumerator)
{
    return DiaEnumIteratorImpl::DiaEnumIterator<Enumerator>(std::move(enumerator),
                                                            DiaEnumIteratorImpl::StartingState::Begin);
}

template<class Enumerator>
DiaEnumIteratorImpl::DiaEnumIterator<Enumerator> end(CComPtr<Enumerator> enumerator)
{
    return DiaEnumIteratorImpl::DiaEnumIterator<Enumerator>(std::move(enumerator),
                                                            DiaEnumIteratorImpl::StartingState::End);
}
