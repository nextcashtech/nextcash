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

        virtual const Hash &getHash() const = 0;

        virtual bool operator == (const HashObject *pRight) const
        {
            return getHash() == pRight->getHash();
        }

        virtual int compare(const SortedObject *pRight) const
        {
            return getHash().compare(((HashObject *)pRight)->getHash());
        }

    };

    // TODO Upgrade to have subsets so large sets can insert/remove more efficiently.
    class HashSet
    {
    public:

        HashSet() { mEndSet = mSets + SET_COUNT - 1; }
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
            return set(pHash)->contains(HashLookupObject(pHash));
        }

        // Returns true if the item was inserted.
        bool insert(HashObject *pObject, bool pAllowDuplicates = false)
        {
            return set(pObject->getHash())->insert(pObject, pAllowDuplicates);
        }

        // Returns true if the item was removed. Deletes item.
        bool remove(const Hash &pHash)
        {
            return set(pHash)->remove(HashLookupObject(pHash));
        }

        // Returns the item with the specified hash.
        // Return NULL if not found.
        HashObject *get(const NextCash::Hash &pHash)
        {
            return (HashObject *)set(pHash)->get(HashLookupObject(pHash));
        }

        // Returns item and doesn't delete it.
        HashObject *getAndRemove(const NextCash::Hash &pHash)
        {
            return (HashObject *)set(pHash)->getAndRemove(HashLookupObject(pHash));
        }

        void clear()
        {
            SortedSet *set = mSets;
            for(unsigned int i = 0; i < SET_COUNT; ++i, ++set)
                set->clear();
        }
        void clearNoDelete() // Doesn't delete items.
        {
            SortedSet *set = mSets;
            for(unsigned int i = 0; i < SET_COUNT; ++i, ++set)
                set->clearNoDelete();
        }

        class Iterator
        {
        public:
            Iterator() { mSet = NULL; mBeginSet = NULL; mEndSet = NULL; }
            Iterator(SortedSet *pSet, SortedSet *pBeginSet, SortedSet *pEndSet,
              const SortedSet::Iterator &pIter)
            {
                mSet      = pSet;
                mBeginSet = pBeginSet;
                mEndSet   = pEndSet;
                mIter     = pIter;
            }
            Iterator(const Iterator &pCopy)
            {
                mSet      = pCopy.mSet;
                mBeginSet = pCopy.mBeginSet;
                mEndSet   = pCopy.mEndSet;
                mIter     = pCopy.mIter;
            }

            bool operator !() const
              { return mSet == NULL || (mSet == mEndSet && mIter == mSet->end()); }
            operator bool() const
              { return mSet != NULL && (mSet != mEndSet || mIter != mSet->end()); }

            HashObject *operator *() { return (HashObject *)*mIter; }
            HashObject *operator ->() { return (HashObject *)&(*mIter); }

            bool operator ==(const Iterator &pRight) const
            {
                return mSet == pRight.mSet && mIter == pRight.mIter;
            }

            bool operator !=(const Iterator &pRight) const
            {
                return mSet != pRight.mSet && mIter != pRight.mIter;
            }

            Iterator &operator =(const Iterator &pRight);
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

            SortedSet *set() { return mSet; }
            SortedSet::Iterator &iter() { return mIter; }

            bool setIsEmpty() { return mSet->size() == 0; }
            void gotoNextBegin();
            void gotoPreviousLast();

        private:

            void increment();
            void decrement();

            SortedSet *mSet, *mBeginSet, *mEndSet;
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

            const Hash &getHash() const { return mHash; }

        private:

            const Hash &mHash;

        };

        static const unsigned int SET_COUNT = 0x0100;
        unsigned int mSize;
        SortedSet mSets[SET_COUNT];
        SortedSet *mEndSet;

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
