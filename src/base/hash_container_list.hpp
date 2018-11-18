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
            if(pIterator == mList.end())
            {
                // Log::add(Log::ERROR, "HashContainerList", "Backup from end");
                return pIterator;
            }

            Hash hash = (*pIterator)->hash;
            // Log::addFormatted(Log::VERBOSE, "HashContainerList", "Backup hash : %s",
              // hash.hex().text());
            SubIterator result = pIterator;
            while((*result)->hash == hash)
            {
                if(result == mList.begin())
                {
                    // Log::add(Log::VERBOSE, "HashContainerList", "Backup at begining");
                    return result;
                }
                --result;
            }

            ++result;
            // Log::addFormatted(Log::VERBOSE, "HashContainerList", "Backup returning : %s",
              // (*result)->hash.hex().text());
            return result;
        }

        // Returns iterator to insert new item before.
        // Returns end if the new item should be appended to the end.
        // Set pFirst to true to ensure the returned iterator is the first item matching the
        //   specified hash.
        SubIterator findInsertBefore(const Hash &pHash, bool &pMatchFound);

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
      const Hash &pHash, bool &pMatchFound)
    {
#ifdef PROFILER_ON
        ProfilerReference profiler(getProfiler(PROFILER_SET, PROFILER_HASH_CONT_FIND_ID,
          PROFILER_HASH_CONT_FIND_NAME), true);
#endif
        // Log::addFormatted(Log::VERBOSE, "HashContainerList", "Insert hash : %s",
          // pHash.hex().text());
        if(mList.size() == 0)
        {
            // Log::add(Log::VERBOSE, "HashContainerList", "Insert as only item");
            return mList.end(); // Insert at the end (as only item)
        }

        Data **begin = mList.data();
        Data **end = mList.data() + mList.size();
        Data **bottom = begin;
        Data **top = end - 1;
        Data **current;

        // Log::addFormatted(Log::VERBOSE, "HashContainerList", "First hash : %s",
          // mList.front()->hash.hex().text());
        int compare = mList.front()->hash.compare(pHash);
        if(compare > 0)
        {
            // Log::add(Log::VERBOSE, "HashContainerList", "Insert at beginning");
            return mList.begin();
        }
        else if(compare == 0)
        {
            // Log::add(Log::VERBOSE, "HashContainerList", "Insert after matching beginning");
            pMatchFound = true;
            current = begin;
            while(current != end && (*current)->hash == pHash)
                ++current;
            return mList.begin() + (current - mList.data());
        }
        else if(mList.size() == 1)
        {
            // Log::add(Log::VERBOSE, "HashContainerList", "Insert after only item");
            return mList.end();
        }

        // Log::addFormatted(Log::VERBOSE, "HashContainerList", "Last hash : %s",
          // mList.back()->hash.hex().text());
        compare = mList.back()->hash.compare(pHash);
        if(compare < 0)
        {
            // Log::add(Log::VERBOSE, "HashContainerList", "Insert at end");
            return mList.end(); // Insert at the end
        }
        else if(compare == 0)
        {
            // Log::add(Log::VERBOSE, "HashContainerList", "Insert at end matching");
            pMatchFound = true;
            return mList.end();
        }

        while(true)
        {
            // Break the set in two halves
            current = bottom + ((top - bottom) / 2);
            if(current == bottom)
            {
                // Log::add(Log::VERBOSE, "HashContainerList", "Current is bottom");
                ++current;
                break;
            }

            // Determine which half the desired item is in
            compare = pHash.compare((*current)->hash);
            if(compare > 0)
            {
                // Log::addFormatted(Log::VERBOSE, "HashContainerList", "Bottom to current : %s",
                  // (*current)->hash.hex().text());
                bottom = current;
            }
            else if(compare < 0)
            {
                // Log::addFormatted(Log::VERBOSE, "HashContainerList", "Top to current : %s",
                  // (*current)->hash.hex().text());
                top = current;
            }
            else
            {
                // Log::addFormatted(Log::VERBOSE, "HashContainerList", "Match found : %s",
                  // (*current)->hash.hex().text());
                pMatchFound = true;
                break; // Matching item found
            }
        }

        // Go to item after last matching
        while(current != end && (*current)->hash == pHash)
        {
            ++current;
            // Log::addFormatted(Log::VERBOSE, "HashContainerList", "Finding item after matching : %s",
              // (*current)->hash.hex().text());
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
        bool matchFound = false;
        SubIterator insertBefore = findInsertBefore(pHash, matchFound);
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
        bool matchFound = false;
        SubIterator item = findInsertBefore(pHash, matchFound);
        if(matchFound)
        {
            // Iterate through any matching hashes to check for a matching value
            SubIterator match = item - 1;
            while((*match)->hash == pHash)
            {
                if(pValuesMatch((*match)->data, pData))
                    return false;
                if(match == mList.begin())
                    break;
                --match;
            }
        }

        // No items with matching hash and value, so insert
        // This will insert after all values with matching hashes
        if(item == mList.end())
            mList.push_back(new Data(pHash, pData)); // Insert at the end
        else
            mList.insert(item, new Data(pHash, pData));
        return true;
    }

    template <class tType>
    typename HashContainerList<tType>::Iterator HashContainerList<tType>::get(const Hash &pHash)
    {
        // Log::addFormatted(Log::VERBOSE, "HashContainerList", "Get hash: %s",
          // pHash.hex().text());
        if(mList.size() == 0)
            return mList.end();

        // Log::addFormatted(Log::VERBOSE, "HashContainerList", "Last hash: %s",
          // mList.back()->hash.hex().text());
        int compare = mList.back()->hash.compare(pHash);
        if(compare == 0) // Last item matches hash.
            return backup(mList.begin() + (mList.size() - 1));
        else if(compare < 0)
            return mList.end(); // Hash is after end.
        else if(mList.size() == 1)
            return mList.end(); // Only item in list doesn't match.

        // Log::addFormatted(Log::VERBOSE, "HashContainerList", "First hash: %s",
          // mList.front()->hash.hex().text());
        compare = mList.front()->hash.compare(pHash);
        if(compare == 0)
            return mList.begin(); // First item matches hash.
        else if(compare > 0)
            return mList.end(); // Hash is before beginning.

        Data **bottom = mList.data();
        Data **top = mList.data() + mList.size() - 1;
        Data **current;

        while(true)
        {
            // Break the set in two halves.
            current = bottom + ((top - bottom) / 2);
            if(current == bottom)
                return mList.end();

            // Determine which half the desired item is in.
            // Log::addFormatted(Log::VERBOSE, "HashContainerList", "Current hash: %s",
              // (*current)->hash.hex().text());
            compare = pHash.compare((*current)->hash);
            if(compare > 0)
                bottom = current;
            else if(compare < 0)
                top = current;
            else
                return backup(mList.begin() + (current - mList.data()));
        }
    }

    bool testHashContainerList();
}

#endif
