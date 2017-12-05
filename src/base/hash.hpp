/**************************************************************************
 * Copyright 2017 ArcMist, LLC                                            *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@arcmist.com>                                    *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#ifndef ARCMIST_HASH_HPP
#define ARCMIST_HASH_HPP

#include "arcmist/io/stream.hpp"

#include <vector>


namespace ArcMist
{
    class Hash : public RawOutputStream // So ArcMist::Digest can write results to it
    {
    public:

        Hash() { mSize = 0; mData = NULL; }
        Hash(unsigned int pSize) { mSize = pSize; mData = new uint8_t[mSize]; zeroize(); }
        Hash(unsigned int pSize, int64_t pValue)
        {
            mSize = pSize;
            mData = new uint8_t[mSize];
            zeroize();
            for(unsigned int i=0;i<8&&i<mSize;++i)
                mData[i] = pValue >> (i * 8);

            if(pValue < 0)
                for(unsigned int i=8;i<mSize;++i)
                    mData[i] = 0xff;
        }
        Hash(const char *pHex);
        Hash(const Hash &pCopy) { mData = NULL; *this = pCopy; }
        ~Hash() { if(mData != NULL) delete[] mData; }

        unsigned int size() const { return mSize; }
        const uint8_t *value() const { return mData; }

        // Big endian (most significant bytes first, i.e. leading zeroes for block hashes)
        ArcMist::String hex() const;

        // Little endian (least significant bytes first)
        ArcMist::String littleHex() const;

        void setSize(unsigned int pSize)
        {
            if(mSize == pSize)
                return;
            if(mData != NULL)
                delete[] mData;
            mSize = pSize;
            if(mSize == 0)
                mData = NULL;
            else
            {
                mData = new uint8_t[mSize];
                zeroize();
            }
        }

        // Big endian (most significant bytes first, i.e. leading zeroes for block hashes)
        void setHex(const char *pHex);

        // Little endian (least significant bytes first)
        void setLittleHex(const char *pHex);

        void setByte(unsigned int pOffset, uint8_t pValue)
        {
            if(pOffset >= mSize)
                return;
            mData[pOffset] = pValue;
        }
        uint8_t getByte(unsigned int pOffset) const
        {
            if(pOffset >= mSize)
                return 0;
            return mData[pOffset];
        }

        bool operator <= (const Hash &pRight) const;

        int compare(const Hash &pRight) const
        {
            if(mSize < pRight.mSize)
                return -1;
            if(mSize > pRight.mSize)
                return 1;
            uint8_t *left = mData + mSize - 1;
            uint8_t *right = pRight.mData + mSize - 1;
            for(unsigned int i=0;i<mSize;++i,--left,--right)
            {
                if(*left < *right)
                    return -1;
                else if(*left > *right)
                    return 1;
            }
            return 0;
        }
        bool operator < (const Hash &pRight) const { return compare(pRight) < 0; }
        bool operator > (const Hash &pRight) const { return compare(pRight) > 0; }

        // Set to zero size. Makes hash "empty"
        void clear() { setSize(0); }

        bool isEmpty() const { return mSize == 0; }

        bool isZero() const
        {
            if(mSize == 0)
                return false; // Empty is not zero
            for(unsigned int i=0;i<mSize;i++)
                if(mData[i] != 0)
                    return false;
            return true;
        }

        // Consecutive zero bits at most significant endian
        unsigned int leadingZeroBits() const;

        // Consecutive zero bytes at most significant endian
        unsigned int leadingZeroBytes() const;
        uint64_t shiftBytesDown(unsigned int pByteShift) const;

        uint16_t lookup16() const
        {
            if(mSize < 2)
                return 0;
            else
                return (mData[0] << 8) + mData[1];
        }

        uint8_t lookup8() const // Used to split into 256 piles
        {
            if(mSize < 1)
                return 0;
            else
                return mData[0];
        }

        // Calculate the SipHash-2-4 "Short ID" and put it in pHash
        bool getShortID(Hash &pHash, const Hash &pHeaderHash);

        void zeroize() { if(mSize > 0) std::memset(mData, 0, mSize); }
        void setMax() { if(mSize > 0) std::memset(mData, 0xff, mSize); }

        void randomize()
        {
            uint32_t random;
            for(unsigned int i=0;i<mSize;i+=4)
            {
                random = ArcMist::Math::randomInt();
                std::memcpy(mData + i, &random, 4);
            }
        }

        bool operator == (const Hash &pRight) const
        {
            if(mSize != pRight.mSize)
                return false;

            if(mSize == 0)
                return true;

            return std::memcmp(mData, pRight.mData, mSize) == 0;
        }

        bool operator != (const Hash &pRight) const
        {
            if(mSize != pRight.mSize)
                return true;

            if(mSize == 0)
                return false;

            return std::memcmp(mData, pRight.mData, mSize) != 0;
        }

        const Hash &operator = (const Hash &pRight)
        {
            if(mData != NULL)
                delete[] mData;
            mSize = pRight.mSize;
            if(mSize > 0)
            {
                mData = new uint8_t[mSize];
                std::memcpy(mData, pRight.mData, mSize);
            }
            else
                mData = NULL;

            return *this;
        }

        const Hash &operator = (int64_t pValue)
        {
            zeroize();
            for(unsigned int i=0;i<8&&i<mSize;++i)
                mData[i] = pValue >> (i * 8);

            if(pValue < 0)
                for(unsigned int i=8;i<mSize;++i)
                    mData[i] = 0xff;

            return *this;
        }

        void write(ArcMist::OutputStream *pStream) const
        {
            if(mSize == 0)
                return;

            pStream->write(mData, mSize);
        }

        bool read(ArcMist::InputStream *pStream)
        {
            if(mSize == 0)
                return true;

            if(pStream->remaining() < mSize)
                return false;

            pStream->read(mData, mSize);
            return true;
        }

        bool read(ArcMist::InputStream *pStream, ArcMist::stream_size pSize)
        {
            setSize(pSize);
            return read(pStream);
        }

        // ArcMist::RawOutputStream virtual
        void write(const void *pInput, ArcMist::stream_size pSize)
        {
            setSize(pSize);
            std::memcpy(mData, pInput, pSize);
        }

        void setDifficulty(uint32_t pTargetBits);
        void getDifficulty(uint32_t &pTargetBits, uint32_t pMax = 0x1d00ffff);
        void getWork(Hash &pWork) const;

        // Arithmetic
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
        Hash &operator -=(const Hash &pValue)
        {
            *this += -pValue;
            return *this;
        }
        Hash &operator *=(const Hash &pValue);
        Hash &operator /=(const Hash &pValue);
        Hash &operator <<=(unsigned int pShiftBits);
        Hash &operator >>=(unsigned int pShiftBits);

        Hash &operator +=(int64_t pValue)
        {
            Hash value(mSize, pValue);
            *this += value;
            return *this;
        }
        Hash &operator -=(int64_t pValue)
        {
            Hash value(mSize, pValue);
            *this -= value;
            return *this;
        }
        Hash &operator *=(int64_t pValue)
        {
            Hash value(mSize, pValue);
            *this *= value;
            return *this;
        }
        Hash &operator /=(int64_t pValue)
        {
            Hash value(mSize, pValue);
            *this /= value;
            return *this;
        }
        Hash operator +(int64_t pValue) const
        {
            Hash result(mSize, pValue);
            result += *this;
            return result;
        }
        Hash operator -(int64_t pValue) const
        {
            Hash result(mSize, pValue);
            result -= *this;
            return result;
        }
        Hash operator *(int64_t pValue) const
        {
            Hash result(mSize, pValue);
            result *= *this;
            return result;
        }
        Hash operator /(int64_t pValue) const
        {
            Hash result(mSize, pValue);
            result /= *this;
            return result;
        }

        static bool test();

    private:

        unsigned int mSize;
        uint8_t *mData;

    };

    class HashList : public std::vector<Hash *>
    {
    public:
        HashList() {}
        ~HashList()
        {
            for(iterator hash=begin();hash!=end();++hash)
                delete *hash;
        }

        void clear()
        {
            for(iterator hash=begin();hash!=end();++hash)
                delete *hash;
            std::vector<Hash *>::clear();
        }

        // Clear without deleting hashes (if they were reused by something else)
        void clearNoDelete() { std::vector<Hash *>::clear(); }

        bool contains(const Hash &pHash) const
        {
            for(const_iterator hash=begin();hash!=end();++hash)
                if(**hash == pHash)
                    return true;
            return false;
        }

        // Insert an item into a sorted list and retain sorting
        void insertSorted(const Hash &pHash);

        // Return true if the item exists in a sorted list
        bool containsSorted(const Hash &pHash);

        void copyNoAllocate(const HashList &pOther)
        {
            reserve(pOther.size());
            for(HashList::const_iterator other=pOther.begin();other!=pOther.end();++other)
                push_back(*other);
        }

    private:
        HashList(HashList &pCopy);
        HashList &operator = (HashList &pRight);
    };
}

#endif
