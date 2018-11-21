/**************************************************************************
 * Copyright 2017-2018 NextCash, LLC                                      *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@nextcash.tech>                                  *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#ifndef NEXTCASH_HASH_HPP
#define NEXTCASH_HASH_HPP

#include "stream.hpp"
#include "log.hpp"
#include "mutex.hpp"
#include "sorted_set.hpp"

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

        Hash() { mSize = 0; mData = NULL; } // Create empty.
        // Create specific size, zeroized.
        Hash(uint8_t pSize) { mSize = 0; mData = NULL; allocate(pSize); zeroize(); }
        Hash(uint8_t pSize, int64_t pValue); // Create from integer value (arithmetic)
        Hash(const char *pHex); // Create from hex text.
        Hash(InputStream *pStream, uint8_t pSize)
          { mSize = 0; mData = NULL; allocate(pSize); read(pStream); }
        Hash(const Hash &pCopy);
        ~Hash() { if(mData != NULL) delete[] mData; }

        const Hash &operator = (const Hash &pRight);

        // Return the size in memory of a hash of a specific size.
        static constexpr stream_size memorySize(stream_size pSize)
        {
            return sizeof(uint8_t) + sizeof(uint8_t *) + (sizeof(uint8_t) * pSize);
        }

        bool isEmpty() const { return mData == NULL; }
        bool isZero() const;

        unsigned int size() const
        {
            if(isEmpty())
                return 0;
            return mSize;
        }
        const uint8_t *data() const { return mData; }

        // Set the number of bytes in the hash
        void setSize(uint8_t pSize) { allocate(pSize); }

        // Set to zero size. Makes hash "empty"
        void clear()
        {
            if(!isEmpty())
            {
                delete[] mData;
                mData = NULL;
            }
            mSize = 0;
        }

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
            if(isEmpty() || pOffset >= mSize)
                return;
            mData[pOffset] = pValue;
        }
        uint8_t getByte(unsigned int pOffset) const
        {
            if(isEmpty() || pOffset >= mSize)
                return 0;
            return mData[pOffset];
        }

        void write(OutputStream *pStream) const
        {
            if(isEmpty())
                return;
            pStream->write(mData, mSize);
        }

        bool read(InputStream *pStream)
        {
            if(isEmpty() || pStream->remaining() < mSize)
                return false;
            pStream->read(mData, mSize);
            return true;
        }

        bool read(InputStream *pStream, stream_size pSize)
        {
            allocate(pSize);
            return read(pStream);
        }

        // RawOutputStream virtual
        void write(const void *pInput, stream_size pSize)
        {
            allocate(pSize);
            std::memcpy(mData, pInput, pSize);
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
            if(isEmpty())
                return pRight.isEmpty();
            if(pRight.isEmpty())
                return false;
            if(mSize != pRight.mSize)
                return false;
            return std::memcmp(mData, pRight.mData, mSize) == 0;
        }

        bool operator != (const Hash &pRight) const
        {
            if(isEmpty())
                return !pRight.isEmpty();
            if(pRight.isEmpty())
                return true;
            if(mSize != pRight.mSize)
                return true;
            return std::memcmp(mData, pRight.mData, mSize) != 0;
        }

        uint8_t lookup8() const // Used to split into 256 (0x100) piles
        {
            if(isEmpty())
                return 0;
            else
                return mData[0];
        }

        uint16_t lookup16() const
        {
            if(isEmpty() || mSize < 2)
                return 0;
            else
                return (uint16_t)(mData[0] << 8) | (uint16_t)mData[1];
        }

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
            if(isEmpty())
                return *this;
            Hash value(mSize, pValue);
            *this += value;
            return *this;
        }
        Hash &operator -=(int64_t pValue)
        {
            if(isEmpty())
                return *this;
            Hash value(mSize, pValue);
            *this -= value;
            return *this;
        }
        Hash &operator *=(int64_t pValue)
        {
            if(isEmpty())
                return *this;
            Hash value(mSize, pValue);
            *this *= value;
            return *this;
        }
        Hash &operator /=(int64_t pValue)
        {
            if(isEmpty())
                return *this;
            Hash value(mSize, pValue);
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

        void allocate(uint8_t pSize);

        uint8_t mSize;
        uint8_t *mData;

    };

    class HashList : public std::vector<Hash>
    {
    public:

        bool contains(const Hash &pHash) const
        {
            for(const_iterator hash = begin(); hash != end(); ++hash)
                if(*hash == pHash)
                    return true;
            return false;
        }

        bool remove(const Hash &pHash)
        {
            for(const_iterator hash = begin(); hash != end(); ++hash)
                if(*hash == pHash)
                {
                    erase(hash);
                    return true;
                }
            return false;
        }

        // Insert an item into a sorted list and retain sorting
        bool insertSorted(const Hash &pHash);

        // Return true if the item exists in a sorted list
        bool containsSorted(const Hash &pHash);

        // Return true if the item was removed from the sorted list
        bool removeSorted(const Hash &pHash);

        typedef std::vector<Hash>::iterator iterator;
        typedef std::vector<Hash>::const_iterator const_iterator;

    };
}

#endif
