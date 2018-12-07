/**************************************************************************
 * Copyright 2018 NextCash, LLC                                           *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@nextcash.tech>                                  *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#ifndef NEXTCASH_REFERENCE_SORTED_SET_HPP
#define NEXTCASH_REFERENCE_SORTED_SET_HPP

#include "log.hpp"
#include "reference_counter.hpp"

#ifdef PROFILER_ON
#include "profiler.hpp"
#endif

#include <cstdint>
#include <vector>

#define NEXTCASH_SORTED_SET_LOG_NAME "RefSortedSet"


namespace NextCash
{
    // tType must have the following functions defined.
    //   int compare(tType &pRight);
    //   bool valueEquals(tType &pRight);
    template <class tType>
    class ReferenceSortedSet
    {
    public:

        typedef ReferenceCounter<tType> Object;

        ReferenceSortedSet() {}

        unsigned int size() const { return mItems.size(); }

        void reserve(unsigned int pSize) { mItems.reserve(pSize); }

        bool contains(Object &pMatching);

        // Returns true if the item was inserted.
        // If pAllowDuplicateSorts is false then multiple objects with the same "sort" value will
        //   not be inserted.
        // Multiple objects that match according to "valueEquals" will never be inserted.
        bool insert(Object &pObject, bool pAllowDuplicateSorts = false);

        // Removes the first item with mathcing "sort" value, and deletes it.
        // Returns true if the item was removed.
        bool remove(Object &pMatching);

        // Deletes all items with matching "sort" value, and deletes them.
        // Returns number of items removed.
        unsigned int removeAll(Object &pMatching);

        // Returns the item with the specified hash.
        // Returns empty reference if not found.
        Object get(Object &pMatching);

        // Returns item and doesn't delete it.
        Object getAndRemove(Object &pMatching);

        void clear() { mItems.clear(); }

        void shrink() { mItems.shrink_to_fit(); }

        typedef typename std::vector<Object>::iterator Iterator;

        Iterator begin() { return mItems.begin(); }
        Iterator end() { return mItems.end(); }

        Object front() { return *mItems.begin(); }
        Object back() { return *--mItems.end(); }

        Iterator find(Object &pMatching);// Return iterator to first matching item.
        Iterator erase(Iterator &pIterator) { return mItems.erase(pIterator); }

        static bool test();

    protected:

        std::vector<Object> mItems;

        // If item is not found, give item after where it should be.
        static const uint8_t RETURN_INSERT_POSITION = 0x01;
        // Another item with the same sort value was found.
        static const uint8_t SORT_WAS_FOUND = 0x02;
        // Another item with the same value was found.
        static const uint8_t VALUE_WAS_FOUND = 0x04;

        // if RETURN_INSERT_POSITION:
        //   return the iterator for the item after the last matching item.
        //   or the item in the position the item specified belongs.
        // else:
        //   return the iterator for the first item matching.
        //   or "end" if no item matching.
        //
        // Sets SORT_WAS_FOUND in pFlags if a matching item was found.
        Iterator binaryFind(Object &pMatching, uint8_t &pFlags);

        // if RETURN_INSERT_POSITION:
        //   increment iterator forward to the item after the last matching item.
        // else:
        //   increment iterator forward to the last matching item.
        Iterator moveForward(Iterator pIterator, Object &pMatching, uint8_t &pFlags);

        // Decrement iterator backward to the first matching item.
        Iterator moveBackward(Iterator pIterator, Object &pMatching, uint8_t &pFlags);

        // Search for matching value from specified iterator and set VALUE_WAS_FOUND if found.
        void searchBackward(Iterator pIterator, Object &pMatching, uint8_t &pFlags);
        void searchForward(Iterator pIterator, Object &pMatching, uint8_t &pFlags);

        ReferenceSortedSet(const ReferenceSortedSet &pCopy);
        ReferenceSortedSet &operator = (const ReferenceSortedSet &pRight);

    };

    template <class tType>
    bool ReferenceSortedSet<tType>::contains(Object &pMatching)
    {
        uint8_t flags = 0x00;
        binaryFind(pMatching, flags);
        return flags & SORT_WAS_FOUND;
    }

    template <class tType>
    bool ReferenceSortedSet<tType>::insert(Object &pObject, bool pAllowDuplicateSorts)
    {
        uint8_t flags = RETURN_INSERT_POSITION;
        Iterator item = binaryFind(pObject, flags);

        if(flags & VALUE_WAS_FOUND)
            return false; // Matching value already in list.

        if(flags & SORT_WAS_FOUND && !pAllowDuplicateSorts)
            return false; // Matching sort already in list.

        mItems.insert(item, pObject); // Insert into list.
        return true;
    }

    template <class tType>
    bool ReferenceSortedSet<tType>::remove(Object &pMatching)
    {
        Object object = getAndRemove(pMatching);
        if(object)
            return true;
        else
            return false;
    }

    template <class tType>
    unsigned int ReferenceSortedSet<tType>::removeAll(Object &pMatching)
    {
        unsigned int result = 0;
        Iterator matching = find(pMatching);
        while(matching != end() && pMatching->compare(*matching) == 0)
        {
            ++result;
            matching = mItems.erase(matching);
        }
        return result;
    }

    template <class tType>
    typename ReferenceSortedSet<tType>::Object ReferenceSortedSet<tType>::get(Object &pMatching)
    {
        uint8_t flags = 0x00;
        Iterator item = binaryFind(pMatching, flags);
        if(item != mItems.end())
            return *item;
        else
            return Object(NULL);
    }

    template <class tType>
    typename ReferenceSortedSet<tType>::Object ReferenceSortedSet<tType>::getAndRemove
      (Object &pMatching)
    {
        uint8_t flags = 0x00;
        Iterator item = binaryFind(pMatching, flags);
        if(item != mItems.end())
        {
            Object result = *item;
            mItems.erase(item);
            return result;
        }
        else
            return Object(NULL);
    }

    template <class tType>
    typename ReferenceSortedSet<tType>::Iterator ReferenceSortedSet<tType>::find(Object &pMatching)
    {
        uint8_t flags = 0x00;
        return binaryFind(pMatching, flags);
    }

    template <class tType>
    typename ReferenceSortedSet<tType>::Iterator ReferenceSortedSet<tType>::moveForward
      (Iterator pIterator, Object &pMatching, uint8_t &pFlags)
    {
        // Check for matching transactions backward
        if(pFlags & RETURN_INSERT_POSITION) // We only care if we are attempting an insert.
            searchBackward(pIterator, pMatching, pFlags);

        ++pIterator;
        while(pIterator != end() && pMatching->compare(**pIterator) == 0)
        {
            if(pMatching->valueEquals(**pIterator))
            {
                pFlags |= VALUE_WAS_FOUND;
                break;
            }
            ++pIterator;
        }

        if(!(pFlags & RETURN_INSERT_POSITION))
            --pIterator;

        return pIterator;
    }

    template <class tType>
    typename ReferenceSortedSet<tType>::Iterator ReferenceSortedSet<tType>::moveBackward
      (Iterator pIterator, Object &pMatching, uint8_t &pFlags)
    {
        // Check for matching transactions forward
        if(pFlags & RETURN_INSERT_POSITION) // We only care if we are attempting an insert.
            searchForward(pIterator, pMatching, pFlags);

        if(pIterator == begin())
            return pIterator;

        --pIterator;
        while(pMatching->compare(**pIterator) == 0)
        {
            if(pMatching->valueEquals(**pIterator))
            {
                pFlags |= VALUE_WAS_FOUND;
                break;
            }
            if(pIterator == begin())
                return pIterator;
            --pIterator;
        }

        return ++pIterator;
    }

    template <class tType>
    void ReferenceSortedSet<tType>::searchBackward(Iterator pIterator, Object &pMatching,
      uint8_t &pFlags)
    {
        while(pMatching->compare(**pIterator) == 0)
        {
            if(pMatching->valueEquals(**pIterator))
            {
                pFlags |= VALUE_WAS_FOUND;
                break;
            }
            if(pIterator == begin())
                break;
            --pIterator;
        }
    }

    template <class tType>
    void ReferenceSortedSet<tType>::searchForward(Iterator pIterator, Object &pMatching,
      uint8_t &pFlags)
    {
        while(pIterator != end() && pMatching->compare(**pIterator) == 0)
        {
            if(pMatching->valueEquals(**pIterator))
            {
                pFlags |= VALUE_WAS_FOUND;
                break;
            }
            ++pIterator;
        }
    }

    template <class tType>
    typename ReferenceSortedSet<tType>::Iterator ReferenceSortedSet<tType>::binaryFind
      (Object &pMatching, uint8_t &pFlags)
    {
#ifdef PROFILER_ON
        ProfilerReference profiler(getProfiler(PROFILER_SET, PROFILER_SORTED_SET_FIND_ID,
          PROFILER_SORTED_SET_FIND_NAME), true);
#endif

        if(mItems.size() == 0) // Item would be only item in list.
            return mItems.end();

        int compare = pMatching->compare(*mItems.back());
        if(compare > 0) // Item would be after end of list.
            return mItems.end();
        else if(compare == 0)
        {
            // Item matches last item.
            pFlags |= SORT_WAS_FOUND;
            if(pFlags & RETURN_INSERT_POSITION)
            {
                searchBackward(--mItems.end(), pMatching, pFlags);
                return mItems.end();
            }
            else
                return moveBackward(--mItems.end(), pMatching, pFlags);
        }

        compare = pMatching->compare(*mItems.front());
        if(compare < 0)
        {
            // Item would be before beginning of list.
            if(pFlags & RETURN_INSERT_POSITION)
            {
                searchForward(mItems.begin(), pMatching, pFlags);
                return mItems.begin(); // Return first item to insert before.
            }
            else
                return mItems.end();
        }
        else if(compare == 0)
        {
            // Item matches first item.
            pFlags |= SORT_WAS_FOUND;
            if(pFlags & RETURN_INSERT_POSITION)
                return moveForward(mItems.begin(), pMatching, pFlags);
            else
                return mItems.begin();
        }

        Iterator bottom = mItems.begin();
        Iterator top    = --mItems.end();
        Iterator current;

        while(true)
        {
            // Break the set in two halves
            current = bottom + ((top - bottom) / 2);
            compare = pMatching->compare(**current);

            if(compare == 0) // Matching item found
            {
                pFlags |= SORT_WAS_FOUND;
                if(pFlags & RETURN_INSERT_POSITION)
                    return moveForward(current, pMatching, pFlags);
                else
                    return moveBackward(current, pMatching, pFlags);
            }

            if(current == bottom) // Matching item not found
            {
                if(pFlags & RETURN_INSERT_POSITION)
                    return top;
                else
                    return mItems.end();
            }

            // Determine which half contains the desired item
            if(compare > 0)
                bottom = current;
            else //if(compare < 0)
                top = current;
        }
    }

    bool testReferenceSortedSet();
}

#endif
