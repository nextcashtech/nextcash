/**************************************************************************
 * Copyright 2018 NextCash, LLC                                           *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@nextcash.tech>                                  *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#ifndef NEXTCASH_HASH_CONTAINER_LIST_HPP
#define NEXTCASH_HASH_CONTAINER_LIST_HPP

#include "hash.hpp"

#ifdef PROFILER_ON
#include "profiler.hpp"
#endif

#include <vector>


namespace NextCash
{
    template <class tType>
    class HashContainerList
    {
    private:

        class Data
        {
        public:
            Data(const Hash &pHash, tType &pData) : hash(pHash) { data = pData; }

            Hash hash;
            tType data;

        private:
            Data(const Data &pCopy);
            const Data &operator = (const Data &pRight);
        };

        std::vector<Data *> mList;

        typedef typename std::vector<Data *>::iterator SubIterator;

        // Back up to first matching hash
        SubIterator backup(const SubIterator &pIterator)
        {
            SubIterator result = pIterator;
            if(result == mList.end())
                return result;

            Hash &hash = (*result)->hash;
            SubIterator start = mList.begin();
            while((*result)->hash == hash)
            {
                if(result == start)
                    return result;
                --result;
            }

            return ++result;
        }

        // Returns iterator to insert new item before.
        // Returns end if the new item should be appended to the end.
        // Set pFirst to true to ensure the returned iterator is the first item matching the
        //   specified hash.
        SubIterator findInsertBefore(const Hash &pHash, bool pFirst);

    public:

        ~HashContainerList();

        unsigned int size() const { return mList.size(); }

        void insert(const Hash &pHash, tType &pData);
        bool remove(const Hash &pHash);

        // Returns true if the new item was inserted
        // Returns false if an item with a matching value and hash was found and no insert was
        //   done.
        // pValuesMatch must be a function that takes two tType parameters and returns true if the
        //   values match.
        // Inserts after all items with matching hashes.
        bool insertIfNotMatching(const Hash &pHash, tType &pData,
          bool (*pValuesMatch)(tType &pLeft, tType &pRight));

        void clear()
        {
            for(SubIterator item = mList.begin(); item != mList.end(); ++item)
                delete *item;
            mList.clear();
        }

        class Iterator
        {
        public:

            Iterator() {}
            Iterator(const SubIterator &pIterator) { mIterator = pIterator; }

            tType &operator *() { return (*mIterator)->data; }
            tType *operator ->() { return &(*mIterator)->data; }

            const Hash &hash() const { return (*mIterator)->hash; }

            bool operator ==(const Iterator &pRight) const { return mIterator == pRight.mIterator; }
            bool operator !=(const Iterator &pRight) const { return mIterator != pRight.mIterator; }

            Iterator &operator =(const Iterator &pRight)
            {
                mIterator = pRight.mIterator;
                return *this;
            }

            void increment() { ++mIterator; }
            Iterator operator +(unsigned int pCount) { return mIterator + pCount; }

            Iterator &operator +=(unsigned int pCount)
            {
                mIterator += pCount;
                return *this;
            }

            // Prefix increment
            Iterator &operator ++()
            {
                increment();
                return *this;
            }

            // Postfix increment
            Iterator operator ++(int)
            {
                Iterator result = *this;
                increment();
                return result;
            }

            void decrement() { --mIterator; }
            Iterator operator -(unsigned int pCount) { return mIterator - pCount; }

            Iterator &operator -=(unsigned int pCount)
            {
                mIterator -= pCount;
                return *this;
            }

            // Prefix decrement
            Iterator &operator --()
            {
                decrement();
                return *this;
            }

            // Postfix decrement
            Iterator operator --(int)
            {
                Iterator result = *this;
                decrement();
                return result;
            }

            SubIterator subIterator() const { return mIterator; }

        private:
            SubIterator mIterator;
        };

        Iterator begin() { return Iterator(mList.begin()); }
        Iterator end() { return Iterator(mList.end()); }

        tType &front() { return mList.front()->data; }
        tType &back() { return mList.back()->data; }
        tType &operator [](unsigned int pOffset) { return mList[pOffset]->data; }

        // Return the iterator to the first item with the specified hash
        Iterator get(const Hash &pHash);
        Iterator erase(const Iterator &pToErase)
        {
            if(pToErase.subIterator() == mList.end())
                return end();
            delete *pToErase.subIterator();
            return Iterator(mList.erase(pToErase.subIterator()));
        }
    };

    template <class tType>
    HashContainerList<tType>::~HashContainerList()
    {
        for(SubIterator item = mList.begin(); item != mList.end(); ++item)
            delete *item;
    }

    template <class tType>
    typename HashContainerList<tType>::SubIterator HashContainerList<tType>::findInsertBefore(
      const Hash &pHash, bool pFirst)
    {
#ifdef PROFILER_ON
        ProfilerReference profiler(getProfiler(PROFILER_SET, PROFILER_HASH_CONT_FIND_ID,
          PROFILER_HASH_CONT_FIND_NAME), true);
#endif
        if(mList.size() == 0)
            return mList.end(); // Insert at the end (as only item)

        int compare = mList.back()->hash.compare(pHash);
        if(compare <= 0)
            return mList.end(); // Insert at the end

        Data **first = mList.data();
        Data **afterLast = mList.data() + mList.size();
        Data **bottom = first;
        Data **top = afterLast - 1;
        Data **current = afterLast;

        if(mList.size() > 1)
        {
            compare = mList.front()->hash.compare(pHash);
            if(compare >= 0)
                return mList.begin(); // Insert at the beginning
        }
        else // Only one item in list and it is after the specified hash, soinsert before it.
            return mList.begin();

        while(true)
        {
            // Break the set in two halves
            current = bottom + ((top - bottom) / 2);

            // Bottom and top are next to each other and have both been checked
            if(current == bottom)
            {
                // Top is the item to insert before, since bottom is below the specified hash
                current = top;
                break;
            }

            compare = pHash.compare((*current)->hash);

            // Determine which half the desired item is in
            if(compare > 0)
                bottom = current;
            else if(compare < 0)
                top = current;
            else
                break; // Matching item found
        }

        if(pFirst && current < afterLast)
        {
            // Back up to first matching item
            bool matchFound = false;
            while(first <= current && (*current)->hash == pHash)
            {
                matchFound = true;
                --current;
            }

            if(first > current)
                current = first;
            else if(matchFound)
                ++current;
        }

        return mList.begin() + (current - mList.data());
    }

    template <class tType>
    void HashContainerList<tType>::insert(const Hash &pHash, tType &pData)
    {
#ifdef PROFILER_ON
        ProfilerReference profiler(getProfiler(PROFILER_SET, PROFILER_HASH_CONT_INSERT_ID,
          PROFILER_HASH_CONT_INSERT_NAME), true);
#endif
        SubIterator insertBefore = findInsertBefore(pHash, false);

        if(insertBefore == mList.end())
            mList.push_back(new Data(pHash, pData)); // Insert at the end
        else
            mList.insert(insertBefore, new Data(pHash, pData));
    }

    template <class tType>
    bool HashContainerList<tType>::remove(const Hash &pHash)
    {
        bool result = false;
        for(Iterator item = get(pHash); item != end() && item.hash() == pHash;)
        {
            delete *item;
            item = erase(item);
            result = true;
        }
        return result;
    }

    template <class tType>
    bool HashContainerList<tType>::insertIfNotMatching(const Hash &pHash, tType &pData,
      bool (*pValuesMatch)(tType &pLeft, tType &pRight))
    {
#ifdef PROFILER_ON
        ProfilerReference profiler(getProfiler(PROFILER_SET, PROFILER_HASH_CONT_INSERT_NM_ID,
          PROFILER_HASH_CONT_INSERT_NM_NAME), true);
#endif
        SubIterator item = findInsertBefore(pHash, true);

        // Iterate through any matching hashes to check for a matching value
        bool wasIncremented = false;
        while(item != mList.end() && (*item)->hash == pHash)
        {
            if(pValuesMatch((*item)->data, pData))
                return false;
            ++item;
            wasIncremented = true;
        }

        // No items with matching hash and value, so insert
        // This will insert after all values with matching hashes
        if(item == mList.end())
            mList.push_back(new Data(pHash, pData)); // Insert at the end
        else
        {
            if(wasIncremented)
                --item;
            mList.insert(item, new Data(pHash, pData));
        }
        return true;
    }

    template <class tType>
    typename HashContainerList<tType>::Iterator HashContainerList<tType>::get(const Hash &pHash)
    {
        if(mList.size() == 0)
            return mList.end();

        int compare;

        compare = mList.back()->hash.compare(pHash);
        if(compare == 0)
            return backup(--mList.end());
        else if(compare < 0)
            return mList.end();

        compare = mList.front()->hash.compare(pHash);
        if(compare == 0)
            return mList.begin();
        else if(compare > 0)
            return mList.end();

        Data **bottom = mList.data();
        Data **top = mList.data() + mList.size() - 1;
        Data **current;

        while(true)
        {
            // Break the set in two halves
            current = bottom + ((top - bottom) / 2);

            if(current == bottom)
                return mList.end();

            compare = pHash.compare((*current)->hash);

            // Determine which half the desired item is in
            if(compare > 0)
                bottom = current;
            else if(compare < 0)
                top = current;
            else
                return backup(mList.begin() + (current - mList.data()));
        }

        return mList.end();
    }

    bool testHashContainerList();
}

#endif
