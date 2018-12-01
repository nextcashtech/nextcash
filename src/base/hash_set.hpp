/**************************************************************************
 * Copyright 2018 NextCash, LLC                                           *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@nextcash.tech>                                  *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#ifndef NEXTCASH_HASH_SET_HPP
#define NEXTCASH_HASH_SET_HPP

#include "hash.hpp"
#include "sorted_set.hpp"

#include <vector>


namespace NextCash
{
    class HashObject : public SortedObject
    {
    public:

        virtual ~HashObject() {}

        virtual const Hash &getHash() = 0;

        int compare(SortedObject *pRight)
        {
            try
            {
                return getHash().compare(dynamic_cast<HashObject *>(pRight)->getHash());
            }
            catch(...)
            {
                return -1;
            }
        }

    };

    class HashSet
    {
    public:

        HashSet() { mSize = 0; mLastSet = mSets + SET_COUNT - 1; }
        ~HashSet() {}

        unsigned int size() const { return mSize; }

        void reserve(unsigned int pSize)
        {
            unsigned int sizePerSet = pSize / SET_COUNT;
            SortedSet *set = mSets;
            for(unsigned int i = 0; i < SET_COUNT; ++i, ++set)
                set->reserve(sizePerSet);
        }

        bool contains(const Hash &pHash)
        {
            HashLookupObject lookup(pHash);
            return set(pHash)->contains(lookup);
        }

        // Returns true if the item was inserted.
        // If pAllowDuplicateSorts is false then multiple objects with the same "sort" value will
        //   not be inserted.
        // Multiple objects that match according to "valueEquals" will never be inserted.
        bool insert(HashObject *pObject, bool pAllowDuplicateSorts = false)
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
            HashLookupObject lookup(pHash);
            if(set(pHash)->remove(lookup))
            {
                --mSize;
                return true;
            }
            else
                return false;
        }

        // Remove and deletes all items with a matching hash.
        // Returns the number of items removed.
        unsigned int removeAll(const Hash &pHash)
        {
            HashLookupObject lookup(pHash);
            return set(pHash)->removeAll(lookup);
        }

        // Returns the item with the specified hash.
        // Return NULL if not found.
        HashObject *get(const NextCash::Hash &pHash)
        {
            HashLookupObject lookup(pHash);
            return (HashObject *)set(pHash)->get(lookup);
        }

        // Returns item and doesn't delete it.
        HashObject *getAndRemove(const NextCash::Hash &pHash)
        {
            HashLookupObject lookup(pHash);
            HashObject *result = (HashObject *)set(pHash)->getAndRemove(lookup);
            if(result != NULL)
                --mSize;
            return result;
        }

        void clear()
        {
            SortedSet *set = mSets;
            for(unsigned int i = 0; i < SET_COUNT; ++i, ++set)
                set->clear();
            mSize = 0;
        }
        void clearNoDelete() // Doesn't delete items.
        {
            SortedSet *set = mSets;
            for(unsigned int i = 0; i < SET_COUNT; ++i, ++set)
                set->clearNoDelete();
            mSize = 0;
        }

        void shrink()
        {
            SortedSet *set = mSets;
            for(unsigned int i = 0; i < SET_COUNT; ++i, ++set)
                set->shrink();
        }

        class Iterator
        {
        public:
            Iterator() { mSet = NULL; mBeginSet = NULL; mLastSet = NULL; }
            Iterator(SortedSet *pSet, SortedSet *pBeginSet, SortedSet *pLastSet,
              const SortedSet::Iterator &pIter)
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

            HashObject *operator *() { return (HashObject *)*mIter; }
            HashObject *operator ->() { return (HashObject *)&(*mIter); }
            const HashObject *operator *() const { return (const HashObject *)*mIter; }
            const HashObject *operator ->() const { return (const HashObject *)&(*mIter); }

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
            Iterator eraseNoDelete();

            SortedSet *set() { return mSet; }
            SortedSet::Iterator &iter() { return mIter; }

            bool setIsEmpty() { return mSet->size() == 0; }
            void gotoNextBegin();
            void gotoPreviousLast();

        private:

            void increment();
            void decrement();

            SortedSet *mSet, *mBeginSet, *mLastSet;
            SortedSet::Iterator mIter;

        };

        Iterator begin();
        Iterator end();

        // Return iterator to first matching item.
        Iterator find(const NextCash::Hash &pHash);

        Iterator eraseDelete(Iterator &pIterator);
        Iterator eraseNoDelete(Iterator &pIterator);

        static bool test();

    private:

        // Used for lookups in SortedSets containing HashObjects
        class HashLookupObject : public HashObject
        {
        public:

            HashLookupObject(const Hash &pHash) : mHash(pHash) {}
            ~HashLookupObject() {}

            const Hash &getHash() { return mHash; }

        private:

            const Hash &mHash;

        };

        static const unsigned int SET_COUNT = 0x0100;
        unsigned int mSize;
        SortedSet mSets[SET_COUNT];
        SortedSet *mLastSet;

        SortedSet *set(const Hash &pHash)
        {
            if(pHash.isEmpty())
                return mSets;
            return mSets + pHash.getByte(pHash.size() - 1);
        }

        HashSet(const HashSet &pCopy);
        HashSet &operator = (const HashSet &pRight);

    };
}

#endif
