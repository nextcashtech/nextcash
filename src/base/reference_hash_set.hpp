/**************************************************************************
 * Copyright 2018 NextCash, LLC                                           *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@nextcash.tech>                                  *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#ifndef NEXTCASH_REFERENCE_HASH_SET_HPP
#define NEXTCASH_REFERENCE_HASH_SET_HPP

#include "hash.hpp"
#include "reference_counter.hpp"
#include "reference_sorted_set.hpp"

#include <vector>

#define NEXTCASH_REF_HASH_SET_LOG_NAME "RefHashSet"


namespace NextCash
{
    // tType must have the following functions defined.
    //   bool valueEquals(tType &pRight);
    //   int compare(tType &pRight); // Calls compare on hashes.
    //   const Hash &getHash();
    template <class tType>
    class ReferenceHashSet
    {
    private:
        typedef ReferenceSortedSet<tType> Set;
    public:

        typedef ReferenceCounter<tType> Object;

        ReferenceHashSet() { mSize = 0; mLastSet = mSets + SET_COUNT - 1; }
        ~ReferenceHashSet() {}

        unsigned int size() const { return mSize; }

        void reserve(unsigned int pSize)
        {
            unsigned int sizePerSet = pSize / SET_COUNT;
            Set *set = mSets;
            for(unsigned int i = 0; i < SET_COUNT; ++i, ++set)
                set->reserve(sizePerSet);
        }

        bool contains(const Hash &pHash)
        {
            uint8_t flags = 0x00;
            binaryFindHash(*set(pHash), pHash, flags);
            return flags & SORT_WAS_FOUND;
        }

        // Returns true if the item was inserted.
        // If pAllowDuplicateSorts is false then multiple objects with the same "sort" value will
        //   not be inserted.
        // Multiple objects that match according to "valueEquals" will never be inserted.
        bool insert(Object &pObject, bool pAllowDuplicateSorts = false)
        {
            if(set(pObject->getHash())->insert(pObject, pAllowDuplicateSorts))
            {
                ++mSize;
                return true;
            }
            else
                return false;
        }

        // Removes and deletes the first item matching the hash.
        // Returns true if an item was removed.
        bool remove(const Hash &pHash)
        {
            Object object = getAndRemove(pHash);
            if(object)
                return true;
            else
                return false;
        }

        // Remove and deletes all items with a matching hash.
        // Returns the number of items removed.
        unsigned int removeAll(const Hash &pHash)
        {
            Set *theSet = set(pHash);
            uint8_t flags = 0x00;
            typename Set::Iterator item = binaryFindHash(*theSet, pHash, flags);
            unsigned int result = 0;
            while(item != end() && pHash.compare((*item)->getHash()) == 0)
            {
                ++result;
                --mSize;
                item = theSet->erase(item);
            }
            return result;
        }

        // Returns the item with the specified hash.
        // Return NULL if not found.
        Object get(const NextCash::Hash &pHash)
        {
            Set *theSet = set(pHash);
            uint8_t flags = 0x00;
            typename Set::Iterator item = binaryFindHash(*theSet, pHash, flags);
            if(item != theSet->end())
                return *item;
            else
                return Object(NULL);
        }

        // Returns item and doesn't delete it.
        Object getAndRemove(const NextCash::Hash &pHash)
        {
            Set *theSet = set(pHash);
            uint8_t flags = 0x00;
            typename Set::Iterator item = binaryFindHash(*theSet, pHash, flags);
            if(item != theSet->end())
            {
                Object result = *item;
                theSet->erase(item);
                --mSize;
                return result;
            }
            else
                return Object(NULL);
        }

        void clear()
        {
            Set *set = mSets;
            for(unsigned int i = 0; i < SET_COUNT; ++i, ++set)
                set->clear();
            mSize = 0;
        }

        void shrink()
        {
            Set *set = mSets;
            for(unsigned int i = 0; i < SET_COUNT; ++i, ++set)
                set->shrink();
        }

        class Iterator
        {
        public:
            Iterator() { mSet = NULL; mBeginSet = NULL; mLastSet = NULL; }
            Iterator(Set *pSet, Set *pBeginSet, Set *pLastSet, const typename Set::Iterator &pIter)
            {
                mSet      = pSet;
                mBeginSet = pBeginSet;
                mLastSet  = pLastSet;
                mIter     = pIter;
            }
            Iterator(const Iterator &pCopy)
            {
                mSet      = pCopy.mSet;
                mBeginSet = pCopy.mBeginSet;
                mLastSet  = pCopy.mLastSet;
                mIter     = pCopy.mIter;
            }

            Iterator &operator = (const Iterator &pRight)
            {
                mSet      = pRight.mSet;
                mBeginSet = pRight.mBeginSet;
                mLastSet  = pRight.mLastSet;
                mIter     = pRight.mIter;
                return *this;
            }

            bool operator !() const
              { return mSet == NULL || (mSet == mLastSet && mIter == mSet->end()); }
            operator bool() const
              { return mSet != NULL && (mSet != mLastSet || mIter != mSet->end()); }

            Object &operator *() { return *mIter; }
            Object *operator ->() { return (Object *)&(*mIter); }
            const Object &operator *() const { return *mIter; }
            const Object *operator ->() const { return (const Object *)&(*mIter); }

            bool operator ==(const Iterator &pRight) const
            {
                return mSet == pRight.mSet && mIter == pRight.mIter;
            }

            bool operator !=(const Iterator &pRight) const
            {
                return mSet != pRight.mSet || mIter != pRight.mIter;
            }

            // Iterator &operator +=(unsigned int pCount);
            Iterator &operator ++(); // Prefix increment
            Iterator operator ++(int); // Postfix increment
            // Iterator &operator -=(unsigned int pCount)
            Iterator &operator --(); // Prefix decrement
            Iterator operator --(int); // Postfix decrement
            // Iterator operator +(unsigned int pCount) const;
            // Iterator operator -(unsigned int pCount) const;

            // Remove item and return next item.
            Iterator erase();

            Set *set() { return mSet; }
            typename Set::Iterator &iter() { return mIter; }

            bool setIsEmpty() { return mSet->size() == 0; }
            void gotoNextBegin();
            void gotoPreviousLast();

        private:

            void increment();
            void decrement();

            Set *mSet, *mBeginSet, *mLastSet;
            typename Set::Iterator mIter;

        };

        Iterator begin();
        Iterator end();

        Object front() { return *begin(); }
        Object back() { return *--end(); }

        // Return iterator to first matching item.
        Iterator find(const NextCash::Hash &pHash);

        Iterator erase(Iterator &pIterator);

        static bool test();

    private:

        static const unsigned int SET_COUNT = 0x0100;
        unsigned int mSize;
        Set mSets[SET_COUNT];
        Set *mLastSet;

        Set *set(const Hash &pHash)
        {
            if(pHash.isEmpty())
                return mSets;
            return mSets + pHash.getByte(pHash.size() - 1);
        }

        // If item is not found, give item after where it should be.
        static const uint8_t RETURN_INSERT_POSITION = 0x01;
        // Another item with the same sort value was found.
        static const uint8_t SORT_WAS_FOUND = 0x02;
        // Another item with the same value was found.
        static const uint8_t VALUE_WAS_FOUND = 0x04;

        typename Set::Iterator moveForward(Set &pSet, typename Set::Iterator pIterator,
          const Hash &pHash, uint8_t &pFlags);
        typename Set::Iterator moveBackward(Set &pSet, typename Set::Iterator pIterator,
          const Hash &pHash, uint8_t &pFlags);
        void searchBackward(Set &pSet, typename Set::Iterator pIterator, const Hash &pHash,
          uint8_t &pFlags);
        void searchForward(Set &pSet, typename Set::Iterator pIterator, const Hash &pHash,
          uint8_t &pFlags);

        typename Set::Iterator binaryFindHash(Set &pSet, const Hash &pHash, uint8_t &pFlags);

        ReferenceHashSet(const ReferenceHashSet &pCopy);
        ReferenceHashSet &operator = (const ReferenceHashSet &pRight);

    };

    template <class tType>
    void ReferenceHashSet<tType>::Iterator::increment()
    {
        // Log::add(Log::DEBUG, NEXTCASH_REF_HASH_SET_LOG_NAME, "Increment");
        ++mIter;
        if(mIter == mSet->end())
            gotoNextBegin();
    }

    template <class tType>
    void ReferenceHashSet<tType>::Iterator::decrement()
    {
        if(mIter == mSet->begin() || (mIter == mSet->end() && mSet->size() == 0))
            gotoPreviousLast();
        else
            --mIter;
    }

    // Prefix increment
    template <class tType>
    typename ReferenceHashSet<tType>::Iterator &ReferenceHashSet<tType>::Iterator::operator ++()
    {
        increment();
        return *this;
    }

    // Postfix increment
    template <class tType>
    typename ReferenceHashSet<tType>::Iterator ReferenceHashSet<tType>::Iterator::operator ++(int)
    {
        Iterator result = *this;
        increment();
        return result;
    }

    // Prefix decrement
    template <class tType>
    typename ReferenceHashSet<tType>::Iterator &ReferenceHashSet<tType>::Iterator::operator --()
    {
        decrement();
        return *this;
    }

    // Postfix decrement
    template <class tType>
    typename ReferenceHashSet<tType>::Iterator ReferenceHashSet<tType>::Iterator::operator --(int)
    {
        Iterator result = *this;
        decrement();
        return result;
    }

    template <class tType>
    typename ReferenceHashSet<tType>::Iterator ReferenceHashSet<tType>::Iterator::erase()
    {
        typename Set::Iterator resultIter = mSet->erase(mIter);
        if(resultIter == mSet->end())
        {
            Iterator result(mSet, mBeginSet, mLastSet, resultIter);
            result.gotoNextBegin();
            return result;
        }
        else
            return Iterator(mSet, mBeginSet, mLastSet, resultIter);
    }

    template <class tType>
    void ReferenceHashSet<tType>::Iterator::gotoNextBegin()
    {
        if(mSet == mLastSet)
        {
            // Log::add(Log::DEBUG, NEXTCASH_REF_HASH_SET_LOG_NAME, "Already last set");
            mIter = mSet->end();
            return;
        }

        ++mSet;
        // Log::add(Log::DEBUG, NEXTCASH_REF_HASH_SET_LOG_NAME, "Next set");
        while(true)
        {
            if(mSet->size() > 0)
            {
                // if(mSet == mLastSet)
                    // Log::add(Log::DEBUG, NEXTCASH_REF_HASH_SET_LOG_NAME, "Non-empty end set");
                // else
                    // Log::add(Log::DEBUG, NEXTCASH_REF_HASH_SET_LOG_NAME, "Non-empty set");
                mIter = mSet->begin();
                return;
            }
            else if(mSet == mLastSet)
            {
                // Log::add(Log::DEBUG, NEXTCASH_REF_HASH_SET_LOG_NAME, "Last set");
                mIter = mSet->end();
                return;
            }
            else
            {
                ++mSet;
                // Log::add(Log::DEBUG, NEXTCASH_REF_HASH_SET_LOG_NAME, "Next set");
            }
        }
    }

    template <class tType>
    void ReferenceHashSet<tType>::Iterator::gotoPreviousLast()
    {
        if(mSet == mBeginSet)
        {
            --mIter; // This is bad
            Log::add(Log::WARNING, NEXTCASH_REF_HASH_SET_LOG_NAME, "This is bad");
            return;
        }

        --mSet;
        while(mSet->size() == 0 && mSet != mBeginSet)
            --mSet;
        mIter = --mSet->end();
    }

    template <class tType>
    typename ReferenceHashSet<tType>::Iterator ReferenceHashSet<tType>::begin()
    {
        Iterator result(mSets, mSets, mLastSet, mSets->begin());
        if(result.setIsEmpty())
            result.gotoNextBegin();
        return result;
    }

    template <class tType>
    typename ReferenceHashSet<tType>::Iterator ReferenceHashSet<tType>::end()
    {
        return Iterator(mLastSet, mSets, mLastSet, mLastSet->end());
    }

    // Return iterator to first matching item.
    template <class tType>
    typename ReferenceHashSet<tType>::Iterator ReferenceHashSet<tType>::find
      (const NextCash::Hash &pHash)
    {
        Set *theSet = set(pHash);
        uint8_t flags = 0x00;
        typename Set::Iterator item = binaryFindHash(*theSet, pHash, flags);
        if(item == theSet->end())
            return end();
        else
            return Iterator(theSet, mSets, mLastSet, item);
    }

    template <class tType>
    typename ReferenceHashSet<tType>::Iterator ReferenceHashSet<tType>::erase(Iterator &pIterator)
    {
        --mSize;
        return pIterator.erase();
    }

    template <class tType>
    typename ReferenceHashSet<tType>::Set::Iterator ReferenceHashSet<tType>::moveForward
      (Set &pSet, typename Set::Iterator pIterator, const Hash &pHash, uint8_t &pFlags)
    {
        // Check for matching transactions backward
        if(pFlags & RETURN_INSERT_POSITION) // We only care if we are attempting an insert.
            searchBackward(pSet, pIterator, pHash, pFlags);

        ++pIterator;
        while(pIterator != pSet.end() && pHash.compare((*pIterator)->getHash()) == 0)
            ++pIterator;

        if(!(pFlags & RETURN_INSERT_POSITION))
            --pIterator;

        return pIterator;
    }

    template <class tType>
    typename ReferenceHashSet<tType>::Set::Iterator ReferenceHashSet<tType>::moveBackward
      (Set &pSet, typename Set::Iterator pIterator, const Hash &pHash, uint8_t &pFlags)
    {
        // Check for matching transactions forward
        if(pFlags & RETURN_INSERT_POSITION) // We only care if we are attempting an insert.
            searchForward(pSet, pIterator, pHash, pFlags);

        if(pIterator == pSet.begin())
            return pIterator;

        --pIterator;
        while(pHash.compare((*pIterator)->getHash()) == 0)
        {
            if(pIterator == pSet.begin())
                return pIterator;
            --pIterator;
        }

        return ++pIterator;
    }

    template <class tType>
    void ReferenceHashSet<tType>::searchBackward(Set &pSet, typename Set::Iterator pIterator,
      const Hash &pHash, uint8_t &pFlags)
    {
        while(pHash.compare((*pIterator)->getHash()) == 0)
        {
            if(pIterator == pSet.begin())
                break;
            --pIterator;
        }
    }

    template <class tType>
    void ReferenceHashSet<tType>::searchForward(Set &pSet, typename Set::Iterator pIterator,
      const Hash &pHash, uint8_t &pFlags)
    {
        while(pIterator != pSet.end() && pHash.compare((*pIterator)->getHash()) == 0)
            ++pIterator;
    }

    template <class tType>
    typename ReferenceHashSet<tType>::Set::Iterator ReferenceHashSet<tType>::binaryFindHash
      (Set &pSet, const Hash &pHash, uint8_t &pFlags)
    {
#ifdef PROFILER_ON
        ProfilerReference profiler(getProfiler(PROFILER_SET, PROFILER_SORTED_SET_FIND_ID,
          PROFILER_SORTED_SET_FIND_NAME), true);
#endif

        if(pSet.size() == 0) // Item would be only item in list.
            return pSet.end();

        int compare = pHash.compare(pSet.back()->getHash());
        if(compare > 0) // Item would be after end of list.
            return pSet.end();
        else if(compare == 0)
        {
            // Item matches last item.
            pFlags |= SORT_WAS_FOUND;
            if(pFlags & RETURN_INSERT_POSITION)
            {
                searchBackward(pSet, --pSet.end(), pHash, pFlags);
                return pSet.end();
            }
            else
                return moveBackward(pSet, --pSet.end(), pHash, pFlags);
        }

        compare = pHash.compare(pSet.front()->getHash());
        if(compare < 0)
        {
            // Item would be before beginning of list.
            if(pFlags & RETURN_INSERT_POSITION)
            {
                searchForward(pSet, pSet.begin(), pHash, pFlags);
                return pSet.begin(); // Return first item to insert before.
            }
            else
                return pSet.end();
        }
        else if(compare == 0)
        {
            // Item matches first item.
            pFlags |= SORT_WAS_FOUND;
            if(pFlags & RETURN_INSERT_POSITION)
                return moveForward(pSet, pSet.begin(), pHash, pFlags);
            else
                return pSet.begin();
        }

        typename Set::Iterator bottom = pSet.begin();
        typename Set::Iterator top    = --pSet.end();
        typename Set::Iterator current;

        while(true)
        {
            // Break the set in two halves
            current = bottom + ((top - bottom) / 2);
            compare = pHash.compare((*current)->getHash());

            if(compare == 0) // Matching item found
            {
                pFlags |= SORT_WAS_FOUND;
                if(pFlags & RETURN_INSERT_POSITION)
                    return moveForward(pSet, current, pHash, pFlags);
                else
                    return moveBackward(pSet, current, pHash, pFlags);
            }

            if(current == bottom) // Matching item not found
            {
                if(pFlags & RETURN_INSERT_POSITION)
                    return top;
                else
                    return pSet.end();
            }

            // Determine which half contains the desired item
            if(compare > 0)
                bottom = current;
            else //if(compare < 0)
                top = current;
        }
    }

    bool testReferenceHashSet();
}

#endif
