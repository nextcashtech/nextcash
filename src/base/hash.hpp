/**************************************************************************
 * Copyright 2017 NextCash, LLC                                            *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@nextcash.com>                                    *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#ifndef NEXTCASH_HASH_HPP
#define NEXTCASH_HASH_HPP

#include "stream.hpp"
#include "log.hpp"
#include "mutex.hpp"

#ifdef PROFILER_ON
#include "profiler.hpp"
#endif

#include <vector>

#define NEXTCASH_HASH_LOG_NAME "Hash"


namespace NextCash
{
    class Hash : public RawOutputStream // So Digest can write results to it
    {
    public:

        Hash() { mData = NULL; } // Create empty
        Hash(unsigned int pSize) { mData = NULL; allocate(pSize); } // Create specific size, zeroized
        Hash(unsigned int pSize, int64_t pValue); // Create from integer value (arithmetic)
        Hash(const char *pHex); // Create from hex text
        Hash(InputStream *pStream, unsigned int pSize)
          { mData = NULL; allocate(pSize); read(pStream); }
        Hash(const Hash &pCopy);
        ~Hash() { deallocate(); }

        const Hash &operator = (const Hash &pRight);

        bool isEmpty() const { return mData == NULL; }
        bool isZero() const;

        unsigned int size() const
        {
            if(mData == NULL)
                return 0;
            return mData->size;
        }
        const uint8_t *data() const
        {
            if(mData == NULL)
                return NULL;
            return mData->data;
        }

        // Set the number of bytes in the hash
        void setSize(unsigned int pSize)
        {
            if(pSize == 0)
            {
                deallocate();
                return;
            }

            if(mData == NULL || mData->size != pSize)
                allocate(pSize);
        }

        // Set to zero size. Makes hash "empty"
        void clear() { setSize(0); }

        void zeroize(); // Set all bytes to zero
        void setMax(); // Set all bytes to 0xff
        void randomize(); // Set all bytes to random values

        // Big endian (most significant bytes first, i.e. leading zeroes for block hashes)
        String hex() const;
        // Little endian (least significant bytes first)
        String littleHex() const;
        // Big endian (most significant bytes first, i.e. leading zeroes for block hashes)
        void setHex(const char *pHex);
        // Little endian (least significant bytes first)
        void setLittleHex(const char *pHex);

        void setByte(unsigned int pOffset, uint8_t pValue)
        {
            if(mData == NULL || pOffset >= mData->size)
                return;
            makeExclusive();
            mData->data[pOffset] = pValue;
        }
        uint8_t getByte(unsigned int pOffset) const
        {
            if(mData == NULL || pOffset >= mData->size)
                return 0;
            return mData->data[pOffset];
        }

        void write(OutputStream *pStream) const
        {
            if(mData == NULL)
                return;
            pStream->write(mData->data, mData->size);
        }

        bool read(InputStream *pStream)
        {
            if(mData == NULL || pStream->remaining() < mData->size)
                return false;
            makeExclusive();
            pStream->read(mData->data, mData->size);
            return true;
        }

        bool read(InputStream *pStream, stream_size pSize)
        {
            setSize(pSize);
            return read(pStream);
        }

        // RawOutputStream virtual
        void write(const void *pInput, stream_size pSize)
        {
            setSize(pSize);
            makeExclusive();
            std::memcpy(mData->data, pInput, pSize);
        }

        // Consecutive zero bits at most significant endian
        unsigned int leadingZeroBits() const;

        // Consecutive zero bytes at most significant endian
        unsigned int leadingZeroBytes() const;
        uint64_t shiftBytesDown(unsigned int pByteShift) const;

        int compare(const Hash &pRight) const;

        bool operator < (const Hash &pRight) const { return compare(pRight) < 0; }
        bool operator > (const Hash &pRight) const { return compare(pRight) > 0; }
        bool operator <= (const Hash &pRight) const { return compare(pRight) <= 0; }

        bool operator == (const Hash &pRight) const
        {
            if(mData == NULL)
                return pRight.mData == NULL;
            if(pRight.mData == NULL)
                return false;
            if(mData->size != pRight.mData->size)
                return false;
            return std::memcmp(mData->data, pRight.mData->data, mData->size) == 0;
        }

        bool operator != (const Hash &pRight) const
        {
            if(mData == NULL)
                return pRight.mData != NULL;
            if(pRight.mData == NULL)
                return true;
            if(mData->size != pRight.mData->size)
                return true;
            return std::memcmp(mData->data, pRight.mData->data, mData->size) != 0;
        }

        uint8_t lookup8() const // Used to split into 256 piles
        {
            if(mData == NULL)
                return 0;
            else
                return mData->data[0];
        }

        uint16_t lookup16() const
        {
            if(mData == NULL || mData->size < 2)
                return 0;
            else
                return (mData->data[0] << 8) + mData->data[1];
        }

        // Calculate the SipHash-2-4 "Short ID" and put it in pHash
        bool getShortID(Hash &pHash, const Hash &pHeaderHash);

        void setDifficulty(uint32_t pTargetBits);
        void getDifficulty(uint32_t &pTargetBits, uint32_t pMax = 0x1d00ffff) const;
        void getWork(Hash &pWork) const;

        // Arithmetic
        const Hash &operator = (int64_t pValue);

        Hash operator -() const;
        Hash operator ~() const;
        Hash operator +(const Hash &pValue) const
        {
            Hash result(*this);
            result += pValue;
            return result;
        }
        Hash operator -(const Hash &pValue) const
        {
            Hash result(*this);
            result -= pValue;
            return result;
        }
        Hash operator *(const Hash &pValue) const
        {
            Hash result(*this);
            result *= pValue;
            return result;
        }
        Hash operator /(const Hash &pValue) const
        {
            Hash result(*this);
            result /= pValue;
            return result;
        }

        Hash &operator ++();
        Hash &operator +=(const Hash &pValue);
        Hash &operator --();
        Hash &operator -=(const Hash &pValue) { *this += -pValue; return *this; }
        Hash &operator *=(const Hash &pValue);
        Hash &operator /=(const Hash &pValue);
        Hash &operator <<=(unsigned int pShiftBits);
        Hash &operator >>=(unsigned int pShiftBits);

        Hash &operator +=(int64_t pValue)
        {
            if(mData == NULL)
                return *this;
            Hash value(mData->size, pValue);
            *this += value;
            return *this;
        }
        Hash &operator -=(int64_t pValue)
        {
            if(mData == NULL)
                return *this;
            Hash value(mData->size, pValue);
            *this -= value;
            return *this;
        }
        Hash &operator *=(int64_t pValue)
        {
            if(mData == NULL)
                return *this;
            Hash value(mData->size, pValue);
            *this *= value;
            return *this;
        }
        Hash &operator /=(int64_t pValue)
        {
            if(mData == NULL)
                return *this;
            Hash value(mData->size, pValue);
            *this /= value;
            return *this;
        }
        Hash operator +(int64_t pValue) const
        {
            Hash result(size(), pValue);
            result += *this;
            return result;
        }
        Hash operator -(int64_t pValue) const
        {
            Hash result(size(), pValue);
            result -= *this;
            return result;
        }
        Hash operator *(int64_t pValue) const
        {
            Hash result(size(), pValue);
            result *= *this;
            return result;
        }
        Hash operator /(int64_t pValue) const
        {
            Hash result(size(), pValue);
            result /= *this;
            return result;
        }

        static bool test();

    private:

        class Data
        {
        public:

            Data(unsigned int pSize) : mutex("Hash")
            {
                size = pSize;
                data = new uint8_t[pSize];
                references = 1;
                std::memset(data, 0, pSize); // Zeroize
            }
            ~Data() { delete[] data; }

            unsigned int size;
            uint8_t *data;
            int references;
            Mutex mutex;

        private:
            Data(const Data &pCopy);
            Data &operator = (const Data &pRight);
        };

        void makeExclusive();
        void allocate(unsigned int pSize);
        void deallocate();

        Data *mData;
        static unsigned int mCount;

    };

    class HashList : public std::vector<Hash>
    {
    public:

        bool contains(const Hash &pHash) const
        {
            for(const_iterator hash=begin();hash!=end();++hash)
                if(*hash == pHash)
                    return true;
            return false;
        }

        // Insert an item into a sorted list and retain sorting
        void insertSorted(const Hash &pHash);

        // Return true if the item exists in a sorted list
        bool containsSorted(const Hash &pHash);

    private:
    };

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
        };

        std::vector<Data *> mList;

        typedef typename std::vector<Data *>::iterator SubIterator;

        SubIterator backup(const SubIterator &pIterator)
        {
            SubIterator result = pIterator;
            if(result == mList.end())
                return result;

            Hash &hash = (*result)->hash;
            SubIterator start = mList.begin();
            while(result != start && (*result)->hash == hash)
                --result;

            if((*result)->hash != hash)
                ++result;

            return result;
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
        // Returns false if an item with a matching value and hash was found and no insert was done.
        // pValuesMatch must be a function that takes two tType parameters and returns true if the values match.
        // Inserts after all items with matching hashes.
        bool insertIfNotMatching(const Hash &pHash, tType &pData,
          bool (*pValuesMatch)(tType &pLeft, tType &pRight));

        void clear()
        {
            for(SubIterator item=mList.begin();item!=mList.end();++item)
                delete *item;
            mList.clear();
        }
        void clearNoDelete() { mList.clear(); }

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
        for(SubIterator item=mList.begin();item!=mList.end();++item)
            delete *item;
    }

    template <class tType>
    typename HashContainerList<tType>::SubIterator HashContainerList<tType>::findInsertBefore(const Hash &pHash, bool pFirst)
    {
#ifdef PROFILER_ON
        Profiler profiler("Hash Container Find Insert Before");
#endif
        if(mList.size() == 0)
            return mList.end(); // Insert at the end (as only item)

        int compare = mList.back()->hash.compare(pHash);

        if(compare < 0)
            return mList.end(); // Insert at the end

        Data **first = mList.data();
        Data **afterLast = mList.data() + mList.size();
        Data **bottom = first;
        Data **top = afterLast - 1;
        Data **current = afterLast;

        if(compare == 0)
            current = top;
        else if(mList.size() > 1)
        {
            compare = mList.front()->hash.compare(pHash);
            if(compare >= 0)
                return mList.begin(); // Insert at the beginning
        }
        else
            return mList.begin(); // Only one item in list and it is after the specified hash, so insert before it

        if(current == afterLast) // Check if already found
        {
            bool done = false;
            while(!done)
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
        }

        if(current == afterLast)
            return mList.end(); // Insert at the end

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
        Profiler profiler("Hash Container Insert");
#endif
        SubIterator insertBefore = findInsertBefore(pHash, false);

#ifdef PROFILER_ON
        Profiler profilerCheck("Hash Container Insert Push");
#endif
        if(insertBefore == mList.end())
            mList.push_back(new Data(pHash, pData)); // Insert at the end
        else
            mList.insert(insertBefore, new Data(pHash, pData));
    }

    template <class tType>
    bool HashContainerList<tType>::remove(const Hash &pHash)
    {
        bool result = false;
        for(Iterator item=get(pHash);item!=end()&&item.hash() == pHash;)
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
        Profiler profiler("Hash Container Insert Not Matching");
#endif
        SubIterator item = findInsertBefore(pHash, true);

#ifdef PROFILER_ON
        Profiler profilerCheck("Hash Container Insert Check Match");
#endif
        // Iterate through any matching hashes to check for a matching value
        bool wasIncremented = false;
        while(item != mList.end() && (*item)->hash == pHash)
        {
            if(pValuesMatch((*item)->data, pData))
                return false;
            ++item;
            wasIncremented = true;
        }
#ifdef PROFILER_ON
        profilerCheck.stop();
#endif

#ifdef PROFILER_ON
        Profiler profilerPush("Hash Container Insert Push");
#endif
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
}

#endif
