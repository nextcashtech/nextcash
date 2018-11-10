/**************************************************************************
 * Copyright 2017-2018 NextCash, LLC                                      *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@nextcash.tech>                                  *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#include "hash.hpp"

#include "endian.hpp"
#include "log.hpp"
#include "digest.hpp"

#define NEXTCASH_HASH_LOG_NAME "Hash"


namespace NextCash
{
    Hash::Hash(const char *pHex)
    {
        mSize = 0;
        mData = NULL;
        setHex(pHex);
    }

    Hash::Hash(const Hash &pCopy)
    {
        mSize = pCopy.mSize;
        if(mSize == 0)
            mData = NULL;
        else
        {
            mData = new uint8_t[mSize];
            std::memcpy(mData, pCopy.mData, mSize);
        }
    }

    Hash::Hash(uint8_t pSize, int64_t pValue)
    {
        mSize = 0;
        mData = NULL;
        allocate(pSize);

        // Assign value
        zeroize();
        for(uint8_t i = 0; i < 8 && i < mSize; ++i)
            mData[i] = pValue >> (i * 8);

        if(pValue < 0)
            for(uint8_t i = 8; i < mSize; ++i)
                mData[i] = 0xff;
    }

    void Hash::allocate(uint8_t pSize)
    {
        if(pSize == 0)
        {
            clear();
            return;
        }

        if(mData != NULL)
        {
            if(mSize == pSize)
                return;

            delete[] mData;
        }

        mSize = pSize;
        mData = new uint8_t[mSize];
    }

    const Hash &Hash::operator = (const Hash &pRight)
    {
        if(pRight.mData == NULL)
        {
            clear();
            return *this;
        }

        if(mData != NULL)
        {
            if(mSize == pRight.mSize)
            {
                std::memcpy(mData, pRight.mData, mSize);
                return *this;
            }

            delete[] mData;
        }

        mSize = pRight.mSize;
        mData = new uint8_t[mSize];
        std::memcpy(mData, pRight.mData, mSize);
        return *this;
    }

    bool Hash::isZero() const
    {
        if(mData == NULL)
            return false; // Empty is not zero
        uint8_t *byte = mData;
        for(uint8_t i = 0; i < mSize; ++i, ++byte)
            if(*byte != 0)
                return false;
        return true;
    }

    void Hash::zeroize()
    {
        if(mData != NULL)
            std::memset(mData, 0, mSize);
    }

    void Hash::setMax()
    {
        if(mData != NULL)
            std::memset(mData, 0xff, mSize);
    }

    void Hash::randomize()
    {
        if(mData == NULL)
            return;
        uint32_t random;
        for(uint8_t i = 0; i < mSize; ++i)
        {
            random = Math::randomInt();
            mData[i] = random & 0xff;
        }
    }

    int Hash::compare(const Hash &pRight) const
    {
        if(mData == NULL)
        {
            if(pRight.mData == NULL)
                return 0;
            else
                return -1;
        }

        if(pRight.mData == NULL)
        {
            if(mData == NULL)
                return 0;
            else
                return 1;
        }

        if(mSize < pRight.mSize)
            return -1;
        if(mSize > pRight.mSize)
            return 1;

        const uint8_t *left = mData + mSize - 1;
        const uint8_t *right = pRight.mData + mSize - 1;
        for(uint8_t i = 0; i < mSize; ++i, --left, --right)
        {
            if(*left < *right)
                return -1;
            else if(*left > *right)
                return 1;
        }
        return 0;
    }

    bool Hash::getShortID(Hash &pHash, const Hash &pHeaderHash)
    {
        if(mData == NULL || mSize != 32 || pHeaderHash.size() != 32)
        {
            pHash.clear();
            return false;
        }

        // Use first two little endian 64 bit integers from header hash as keys
        uint64_t key0 = 0;
        uint64_t key1 = 0;
        uint8_t i;
        const uint8_t *byte = pHeaderHash.mData;
        for(i = 0; i < 8; ++i)
            key0 |= (uint64_t)*byte++ << (i * 8);
        for(i = 0; i < 8; ++i)
            key1 |= (uint64_t)*byte++ << (i * 8);

        uint64_t sipHash24Value = Digest::sipHash24(mData, 32, key0, key1);

        // Put 6 least significant bytes of sipHash24Value into result
        pHash.setSize(6);
        for(i = 0; i < 6; ++i)
            pHash.mData[i] = (sipHash24Value >> (i * 8)) & 0xff;

        return true;
    }

    unsigned int Hash::leadingZeroBits() const
    {
        if(mData == NULL)
            return 0;

        unsigned int result = 0;
        const uint8_t *byte = mData + mSize - 1;
        for(uint8_t i = 0; i < mSize; ++i, --byte)
        {
            if(*byte == 0)
                result += 8;
            else
            {
                for(int j=7;j>=0;--j)
                {
                    if(*byte >> j != 0)
                        break;
                    else
                        ++result;
                }
                break;
            }
        }
        return result;
    }

    unsigned int leadingZeroBits(uint8_t pByte)
    {
        unsigned int result = 0;
        for(int j = 7; j >= 0; --j)
        {
            if(pByte >> j != 0)
                return result;
            else
                ++result;
        }
        return result;
    }

    unsigned int Hash::leadingZeroBytes() const
    {
        if(mData == NULL)
            return 0;

        unsigned int result = 0;
        const uint8_t *byte = mData + mSize - 1;
        for(uint8_t i = 0; i < mSize; ++i, --byte)
        {
            if(*byte == 0)
                ++result;
            else
                break;
        }

        return result;
    }

    uint64_t Hash::shiftBytesDown(unsigned int pByteShift) const
    {
        if(mData == NULL || pByteShift >= mSize)
            return 0;

        // Get least significant byte of shifted value
        const uint8_t *byte = mData + mSize - pByteShift;
        uint64_t result = 0;

        // Add all available bytes to value
        for(uint8_t i = 0; i < 8 && byte >= mData; ++i, --byte)
            result &= (uint64_t)*byte << (i * 8);

        return result;
    }

    const Hash &Hash::operator = (int64_t pValue)
    {
        if(mData == NULL)
            return *this;

        zeroize();
        for(uint8_t i = 0; i < 8 && i < mSize; ++i)
            mData[i] = pValue >> (i * 8);
        if(pValue < 0)
        {
            for(uint8_t i = 8; i < mSize; ++i)
                mData[i] = 0xff;
        }
        return *this;
    }

    Hash Hash::operator ~() const
    {
        if(mData == NULL)
            return Hash();
        Hash result(mSize);
        const uint8_t *byte = mData;
        uint8_t *resultByte = result.mData;
        for(uint8_t i = 0; i < mSize; ++i, ++byte, ++resultByte)
            *resultByte = ~*byte;
        return result;
    }

    Hash Hash::operator -() const
    {
        if(mData == NULL)
            return Hash();
        Hash result(*this);
        const uint8_t *byte = mData;
        uint8_t *resultByte = result.mData;
        for(uint8_t i = 0; i < mSize; ++i, ++byte, ++resultByte)
            *resultByte = ~*byte;
        ++result;
        return result;
    }

    Hash &Hash::operator ++()
    {
        if(mData == NULL)
            return *this;
        // Prefix operator
        uint8_t i = 0;
        while(++mData[i] == 0 && i < mSize - 1)
            ++i;
        return *this;
    }

    Hash &Hash::operator --()
    {
        if(mData == NULL)
            return *this;
        // Prefix operator
        uint8_t i = 0;
        while(--mData[i] == (uint8_t)-1 && i < mSize - 1)
            ++i;
        return *this;
    }

    Hash &Hash::operator +=(const Hash &pValue)
    {
        if(mData == NULL || pValue.mData == NULL)
            return *this;
        uint64_t carry = 0;
        uint8_t *byte = mData;
        const uint8_t *valueByte = pValue.mData;
        if(pValue.mSize != mSize)
            return *this; // Error
        for(uint8_t i = 0; i < mSize; ++i, ++byte, ++valueByte)
        {
            uint64_t n = carry + *byte + *valueByte;
            *byte = n & 0xff;
            carry = n >> 8;
        }
        return *this;
    }

    Hash &Hash::operator *=(const Hash &pValue)
    {
        if(mData == NULL || pValue.mData == NULL)
            return *this;

        Hash copy = *this;

        const uint8_t *valueByte;
        const uint8_t *copyByte = copy.mData;

        if(pValue.mSize != mSize)
            return *this; // Error

        zeroize();

        for(uint8_t j=0;j<mSize;++j)
        {
            uint64_t carry = 0;
            valueByte = pValue.mData;
            for(uint8_t i=0;i+j<mSize;++i)
            {
                uint64_t n = (uint64_t)carry + (uint64_t)mData[i + j] +
                  ((uint64_t)*copyByte * (uint64_t)*valueByte);
                mData[i + j] = n & 0xff;
                carry = n >> 8;
                ++valueByte;
            }
            ++copyByte;
        }

        return *this;
    }

    Hash &Hash::operator /= (const Hash &pValue)
    {
        if(mData == NULL || pValue.mData == NULL)
            return *this;

        Hash div(pValue); // make a copy, so we can shift.
        Hash num(*this); // make a copy, so we can subtract.

        zeroize();

        // The quotient.
        int numBits = (mSize * 8) - num.leadingZeroBits();
        int divBits = (mSize * 8) - div.leadingZeroBits();

        if(divBits == 0)
            return *this; // Divide by zero

        if(divBits > numBits)
            return *this; // The result is certainly zero

        // Shift so that div and num align.
        int shift = numBits - divBits;
        div <<= shift;
        while(shift >= 0)
        {
            if(num.compare(div) >= 0)
            {
                num -= div;
                mData[shift / 8] |= (1 << (shift & 7)); // Set a bit of the result.
            }

            // Shift back.
            div >>= 1;
            shift--;
        }
        // num now contains the remainder of the division.

        return *this;
    }

    Hash &Hash::operator <<=(unsigned int pShiftBits)
    {
        if(mData == NULL || pShiftBits == 0)
            return *this;

        Hash copy(*this);
        int offset = pShiftBits / 8;

        pShiftBits = pShiftBits % 8;
        zeroize();

        for(uint8_t i=0;i<mSize;++i)
        {
            if(i + offset + 1 < mSize && pShiftBits != 0)
                mData[i + offset + 1] |= (copy.mData[i] >> (8 - pShiftBits));
            if(i + offset < mSize)
                mData[i + offset] |= (copy.mData[i] << pShiftBits);
        }

        return *this;
    }

    Hash &Hash::operator >>=(unsigned int pShiftBits)
    {
        if(mData == NULL || pShiftBits == 0)
            return *this;

        Hash copy(*this);
        int offset = pShiftBits / 8;

        pShiftBits = pShiftBits % 8;
        zeroize();

        for(uint8_t i=0;i<mSize;++i)
        {
            if((int)i - offset - 1 >= 0 && pShiftBits != 0)
                mData[(int)i - offset - 1] |= (copy.mData[i] << (8 - pShiftBits));
            if((int)i - offset >= 0)
                mData[i - offset] |= (copy.mData[i] >> pShiftBits);
        }

        return *this;
    }

    void Hash::setDifficulty(uint32_t pTargetBits)
    {
        setSize(32);
        zeroize();

        int length = ((pTargetBits >> 24) & 0xff) - 1;

        // Starts with zero so increase
        if((pTargetBits & 0x00ff0000) == 0)
        {
            --length;
            pTargetBits <<= 8;
        }

        if(length >= 0 && length < 32)
            mData[length] = (pTargetBits >> 16) & 0xff;
        if(length - 1 >= 0 && length - 1 < 32)
            mData[length-1] = (pTargetBits >> 8) & 0xff;
        if(length - 2 >= 0 && length - 2 < 32)
            mData[length-2] = pTargetBits & 0xff;
    }

    void Hash::getDifficulty(uint32_t &pTargetBits, uint32_t pMax) const
    {
        if(mData == NULL)
        {
            pTargetBits = 0;
            return;
        }

        uint8_t length = mSize - leadingZeroBytes();
        uint32_t value = 0;

        for(int i=1;i<4;++i)
        {
            value <<= 8;
            if((int)length - i < (int)mSize)
                value += getByte(length - i);
        }

        // Apply maximum
        uint8_t maxLength = (pMax >> 24) & 0xff;
        uint32_t maxValue = pMax & 0x00ffffff;

        if(maxLength < length || (maxLength == length && maxValue < value))
        {
            length = maxLength;
            value = maxValue;
        }

        if(value & 0x00800000) // Pad with zero byte so it isn't negative
        {
            ++length;
            value >>= 8;
        }

        pTargetBits = length << 24;
        pTargetBits += value & 0x00ffffff;
    }

    void Hash::getWork(Hash &pWork) const
    {
        // We need to compute 2**256 / (bnTarget+1), but we can't represent 2**256
        // as it's too large for a arith_uint256. However, as 2**256 is at least as
        // large as bnTarget+1, it is equal to ((2**256 - bnTarget - 1) /
        // (bnTarget+1)) + 1, or ~bnTarget / (bnTarget+1) + 1.
        pWork = ~*this;
        pWork /= (*this + 1);
        ++pWork;
    }

    // Big endian (most significant bytes first, i.e. leading zeroes for block hashes)
    String Hash::hex() const
    {
        String result;
        if(mData == NULL || mSize == 0)
            return result;
        result.writeReverseHex(mData, mSize);
        return result;
    }

    // Little endian (least significant bytes first)
    String Hash::littleHex() const
    {
        String result;
        if(mData == NULL)
            return result;
        result.writeHex(mData, mSize);
        return result;
    }

    // Big endian (most significant bytes first, i.e. leading zeroes for block hashes)
    void Hash::setHex(const char *pHex)
    {
        unsigned int length = std::strlen(pHex);
        setSize(length / 2);

        const char *hexChar = pHex + length - 1;
        uint8_t *byte = mData;
        bool second = false;

        while(hexChar >= pHex)
        {
            if(second)
            {
                (*byte) |= Math::hexToNibble(*hexChar) << 4;
                ++byte;
            }
            else
                (*byte) = Math::hexToNibble(*hexChar);

            second = !second;
            --hexChar;
        }
    }

    // Little endian (least significant bytes first)
    void Hash::setLittleHex(const char *pHex)
    {
        unsigned int length = std::strlen(pHex);
        setSize(length / 2);

        const char *hexChar = pHex;
        uint8_t *byte = mData;
        bool second = false;

        for(unsigned int i=0;i<length;++i)
        {
            if(second)
            {
                (*byte) |= Math::hexToNibble(*hexChar);
                ++byte;
            }
            else
                (*byte) = Math::hexToNibble(*hexChar) << 4;

            second = !second;
            ++hexChar;
        }
    }

    bool HashList::insertSorted(const Hash &pHash)
    {
        if(size() == 0 || back().compare(pHash) < 0)
        {
            push_back(pHash);
            return true;
        }

        int compare;
        Hash *bottom = data();
        Hash *top = data() + size() - 1;
        Hash *current;

        while(true)
        {
            // Break the set in two halves
            current = bottom + ((top - bottom) / 2);

            if(current == bottom)
            {
                compare = bottom->compare(pHash);
                if(compare > 0)
                    current = bottom; // Insert before bottom
                else if(compare == 0)
                    return false; // Match found
                else
                {
                    if(current != top)
                    {
                        compare = top->compare(pHash);
                        if(compare > 0)
                            current = top; // Insert before top
                        else if(compare == 0)
                            return false; // Match found
                        else
                            current = top + 1; // Insert after top
                    }
                    else
                        current = top + 1; // Insert after top
                }
                break;
            }

            // Determine which half the desired item is in
            compare = pHash.compare(*current);
            if(compare > 0)
                bottom = current;
            else if(compare < 0)
                top = current;
            else
                return false; // Match found
        }

        iterator after = begin();
        after += (current - data());
        insert(after, pHash);
        return true;
    }

    bool HashList::containsSorted(const Hash &pHash)
    {
        if(size() == 0 || back().compare(pHash) < 0)
            return false;

        int compare;
        Hash *bottom = data();
        Hash *top = data() + size() - 1;
        Hash *current;

        while(true)
        {
            // Break the set in two halves
            current = bottom + ((top - bottom) / 2);
            compare = pHash.compare(*current);

            if(current == bottom)
                return *bottom == pHash;

            // Determine which half the desired item is in
            if(compare > 0)
                bottom = current;
            else if(compare < 0)
                top = current;
            else
                return true;
        }
    }

    bool HashList::removeSorted(const Hash &pHash)
    {
        if(size() == 0 || back().compare(pHash) < 0)
            return false;

        int compare;
        Hash *bottom = data();
        Hash *top = data() + size() - 1;
        Hash *current;

        while(true)
        {
            // Break the set in two halves
            current = bottom + ((top - bottom) / 2);

            if(current == bottom)
            {
                if(*bottom == pHash)
                {
                    // Remove bottom
                    erase(begin() + (bottom - data()));
                    return true;
                }
                else
                    return false;
            }

            // Determine which half the desired item is in
            compare = pHash.compare(*current);
            if(compare > 0)
                bottom = current;
            else if(compare < 0)
                top = current;
            else
            {
                // Remove current
                erase(begin() + (current - data()));
                return true;
            }
        }
    }

    bool stringEqual(String *&pLeft, String *&pRight)
    {
        return *pLeft == *pRight;
    }

    bool Hash::test()
    {
        Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "------------- Starting Hash Tests -------------");

        bool success = true;

        /***********************************************************************************************
         * Hash lookup distribution
         ***********************************************************************************************/
        unsigned int values[0x100];
        for(int i=0;i<0x100;++i)
            values[i] = 0;

        Hash hash(32);
        unsigned int count = 0x100 * 0x0f;
        for(unsigned int i=0;i<count;++i)
        {
            hash.randomize();
            // Log::addFormatted(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Random Hash : %s",
              // hash.hex().text());
            values[hash.lookup8()] += 1;
        }

        unsigned int highestCount = 0;
        unsigned int zeroCount = 0;
        for(unsigned int i=0;i<0x100;++i)
        {
            if(values[i] == 0)
            {
                Log::addFormatted(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Zero lookup : %d",
                  i);
                ++zeroCount;
            }
            else if(values[i] > highestCount)
                highestCount = values[i];
        }

        //Buffer line;
        //for(unsigned int i=0;i<0x100;i+=16)
        //{
        //    line.clear();
        //    line.writeFormatted("%d\t: ", i);
        //    for(unsigned int j=0;j<16;j++)
        //        line.writeFormatted("%d, ", values[i+j]);
        //    String lineText = line.readString(line.length());
        //    Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, lineText);
        //}

        if(highestCount < 100 && zeroCount < 10)
            Log::addFormatted(Log::INFO, NEXTCASH_HASH_LOG_NAME,
              "Passed hash lookup distribution : high %d, zeroes %d", highestCount, zeroCount);
        else
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME,
              "Failed hash lookup distribution : high %d, zeroes %d", highestCount, zeroCount);
            success = false;
        }

        /***********************************************************************************************
         * Hash set hex
         ***********************************************************************************************/
        Hash value;
        value.setHex("4d085aa37e61a1bf2a6a53b72394f57a6b5ecaca0e2c385a27f96551ea92ad96");
        String hex = "4d085aa37e61a1bf2a6a53b72394f57a6b5ecaca0e2c385a27f96551ea92ad96";

        if(value.hex() == hex)
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed Hash set hex");
        else
        {
            Log::add(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Failed Hash set hex");
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Hash    : %s", value.hex().text());
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Correct : %s", hex.text());
            success = false;
        }

        /***********************************************************************************************
         * Hash set little hex
         ***********************************************************************************************/
        value.setLittleHex("96ad92ea5165f9275a382c0ecaca5e6b7af59423b7536a2abfa1617ea35a084d");

        if(value.hex() == hex)
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed Hash set little hex");
        else
        {
            Log::add(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Failed Hash set little hex");
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Hash    : %s", value.hex().text());
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Correct : %s", hex.text());
            success = false;
        }

        /***********************************************************************************************
         * Hash little endian hex
         ***********************************************************************************************/
        hex = "96ad92ea5165f9275a382c0ecaca5e6b7af59423b7536a2abfa1617ea35a084d";

        if(value.littleHex() == hex)
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed Hash little endian hex");
        else
        {
            Log::add(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Failed Hash little endian hex");
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Hash    : %s", value.littleHex().text());
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Correct : %s", hex.text());
            success = false;
        }

        /***********************************************************************************************
         * Target Bits Decode 0x181bc330
         ***********************************************************************************************/
        Hash testDifficulty;
        Hash checkDifficulty(32);
        testDifficulty.setDifficulty(0x181bc330);
        Buffer testData;

        testData.writeHex("00000000000000000000000000000000000000000030c31b0000000000000000");
        checkDifficulty.read(&testData);

        if(testDifficulty == checkDifficulty)
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed Target Bits Decode 0x181bc330");
        else
        {
            Log::add(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Failed Target Bits Decode 0x181bc330");
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Difficulty : %s", testDifficulty.hex().text());
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Correct    : %s", checkDifficulty.hex().text());
            success = false;
        }

        /***********************************************************************************************
         * Target Bits Decode 0x1b0404cb
         ***********************************************************************************************/
        testDifficulty.setDifficulty(0x1b0404cb);
        testData.clear();
        testData.writeHex("000000000000000000000000000000000000000000000000cb04040000000000");
        checkDifficulty.read(&testData);

        if(testDifficulty == checkDifficulty)
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed Target Bits Decode 0x1b0404cb");
        else
        {
            Log::add(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Failed Target Bits Decode 0x1b0404cb");
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Difficulty : %s", testDifficulty.hex().text());
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Correct    : %s", checkDifficulty.hex().text());
            success = false;
        }

        /***********************************************************************************************
         * Target Bits Decode 0x1d00ffff
         ***********************************************************************************************/
        testDifficulty.setDifficulty(0x1d00ffff);
        testData.clear();
        testData.writeHex("0000000000000000000000000000000000000000000000000000ffff00000000");
        checkDifficulty.read(&testData);

        if(testDifficulty == checkDifficulty)
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed Target Bits Decode 0x1d00ffff");
        else
        {
            Log::add(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Failed Target Bits Decode 0x1d00ffff");
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Difficulty : %s", testDifficulty.hex().text());
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Correct    : %s", checkDifficulty.hex().text());
            success = false;
        }

        /***********************************************************************************************
         * Target Bits Encode 0x1b0404cb
         ***********************************************************************************************/
        testDifficulty.setDifficulty(0x1b0404cb);
        uint32_t checkTargetBits;
        testDifficulty.getDifficulty(checkTargetBits);

        if(checkTargetBits == 0x1b0404cb)
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed Target Bits Encode 0x1b0404cb");
        else
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Failed Target Bits Encode 0x1b0404cb : 0x%08x", checkTargetBits);
            success = false;
        }

        /***********************************************************************************************
         * Target Bits Encode 0x1d00ffff
         ***********************************************************************************************/
        testDifficulty.setDifficulty(0x1d00ffff);
        testDifficulty.getDifficulty(checkTargetBits);

        if(checkTargetBits == 0x1d00ffff)
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed Target Bits Encode 0x1d00ffff");
        else
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Failed Target Bits Encode 0x1d00ffff : 0x%08x", checkTargetBits);
            success = false;
        }

        /***********************************************************************************************
         * Target Bits Encode 0x181bc330
         ***********************************************************************************************/
        testDifficulty.setDifficulty(0x181bc330);
        testDifficulty.getDifficulty(checkTargetBits);

        if(checkTargetBits == 0x181bc330)
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed Target Bits Encode 0x181bc330");
        else
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Failed Target Bits Encode 0x181bc330 : 0x%08x", checkTargetBits);
            success = false;
        }

        /***********************************************************************************************
         * Target Bits Check less than
         ***********************************************************************************************/
        testDifficulty.setDifficulty(486604799); //0x1d00ffff
        testData.clear();
        testData.writeHex("43497fd7f826957108f4a30fd9cec3aeba79972084e90ead01ea330900000000");
        checkDifficulty.read(&testData);

        if(checkDifficulty <= testDifficulty)
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed Target Bits Check less than");
        else
        {
            Log::add(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Failed Target Bits Check less than");
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Check   : %s", checkDifficulty.hex().text());
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Highest : %s", testDifficulty.hex().text());
            success = false;
        }

        /***********************************************************************************************
         * Target Bits Check equal
         ***********************************************************************************************/
        testDifficulty.setDifficulty(486604799);
        checkDifficulty.setDifficulty(0x1d00ffff);

        if(checkDifficulty <= testDifficulty)
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed Target Bits Check equal");
        else
        {
            Log::add(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Failed Target Bits Check equal");
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Check   : %s", checkDifficulty.hex().text());
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Highest : %s", testDifficulty.hex().text());
            success = false;
        }

        /***********************************************************************************************
         * Target Bits Check not less than
         ***********************************************************************************************/
        testDifficulty.setDifficulty(486604799); //0x1d00ffff
        testData.clear();
        testData.writeHex("43497fd7f826957108f4a30fd9cec3aeba79972084e90ead01ea330910000000");
        checkDifficulty.read(&testData);

        if(checkDifficulty <= testDifficulty)
        {
            Log::add(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Failed Target Bits Check not less than");
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Check   : %s", checkDifficulty.hex().text());
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Highest : %s", testDifficulty.hex().text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed Target Bits Check not less than");

        /***********************************************************************************************
         * Test hash compare equal
         ***********************************************************************************************/
        Hash leftHash("0010");
        Hash rightHash("0010");

        if(leftHash.compare(rightHash) == 0)
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed hash compare equal");
        else
        {
            Log::add(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Failed hash compare equal");
            success = false;
        }

        /***********************************************************************************************
         * Test hash compare less than
         ***********************************************************************************************/
        leftHash.setHex("0010");
        rightHash.setHex("0020");

        if(leftHash.compare(rightHash) < 0)
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed hash compare less than");
        else
        {
            Log::add(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Failed hash compare less than");
            success = false;
        }

        /***********************************************************************************************
         * Test hash compare greater than
         ***********************************************************************************************/
        leftHash.setHex("0020");
        rightHash.setHex("0010");

        if(leftHash.compare(rightHash) > 0)
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed hash compare greater than");
        else
        {
            Log::add(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Failed hash compare greater than");
            success = false;
        }

        /***********************************************************************************************
         * Add Hash
         ***********************************************************************************************/
        Hash a(32, 5);
        Hash b(32, 1000);
        Hash answer(32, 1005);

        a += b;
        if(a == answer)
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed add assign hash 1005");
        else
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Failed add assign hash 1005 : %s",
              a.hex().text());
            success = false;
        }

        a = 5;
        if(a + b == answer)
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed add hash 1005");
        else
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Failed add hash 1005 : %s",
              (a + b).hex().text());
            success = false;
        }

        a = 1005;
        answer = 1010;
        if(a + 5 == answer)
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed add hash 1010");
        else
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Failed add hash 1010 : %s",
              (a + 5).hex().text());
            success = false;
        }

        a = 16589;
        answer = 16590;
        ++a;
        if(a == answer)
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed increment");
        else
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Failed increment : %s",
              a.hex().text());
            success = false;
        }

        /***********************************************************************************************
         * Subtract Hash
         ***********************************************************************************************/
        a = 1000;
        b = 5;
        a -= b;
        answer = 995;
        if(a == answer)
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed subtract assign hash 995");
        else
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Failed subtract assign hash 995 : %s",
              a.hex().text());
            success = false;
        }

        a = 1000;
        Hash c = a - b;
        if(c == answer)
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed subtract hash 995");
        else
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Failed subtract hash 995 : %s",
              c.hex().text());
            success = false;
        }

        a = 16589;
        answer = 16588;
        --a;
        if(a == answer)
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed decrement");
        else
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Failed decrement : %s",
              a.hex().text());
            success = false;
        }

        /***********************************************************************************************
         * Assign negative Hash
         ***********************************************************************************************/
        a = -1;
        answer.setHex("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        if(a == answer)
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed assign negative");
        else
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Failed assign negative : %s",
              a.hex().text());
            success = false;
        }

        /***********************************************************************************************
         * Multiply Hash
         ***********************************************************************************************/
        a = 100000;
        b = 1000;
        answer = 100000000;
        a *= b;
        if(a == answer)
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed multiply assign 100000000");
        else
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Failed multiply assign 100000000 : %s",
              a.hex().text());
            success = false;
        }

        a = 100000;
        a *= 1000;
        if(a == answer)
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed multiply assign int 100000000");
        else
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Failed multiply assign int 100000000 : %s",
              a.hex().text());
            success = false;
        }

        /***********************************************************************************************
         * Divide Hash
         ***********************************************************************************************/
        a = 100000;
        b = 1000;
        answer = 100;
        a /= b;
        if(a == answer)
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed divide assign 100");
        else
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Failed divide assign 100 : %s",
              a.hex().text());
            success = false;
        }

        a = 100000;
        a /= 1000;
        if(a == answer)
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed divide assign int 100");
        else
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Failed divide assign int 100 : %s",
              a.hex().text());
            success = false;
        }

        /***********************************************************************************************
         * Negate Hash
         ***********************************************************************************************/
        a.setHex("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        b = -a;
        answer = 1;
        if(b == answer)
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed negate -1 hash");
        else
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Failed negate -1 hash : %s",
              b.hex().text());
            success = false;
        }

        a.setHex("0000000000000000000000000000000000000000000000000000000000000001");
        b = -a;
        answer.setHex("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        if(b == answer)
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed negate 1 hash");
        else
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Failed negate 1 hash : %s",
              b.hex().text());
            success = false;
        }

        a = 1950;
        b = -a;
        answer.setHex("fffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff862");
        if(b == answer)
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed negate 1950 hash");
        else
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Failed negate 1950 hash : %s",
              b.hex().text());
            success = false;
        }

        /***********************************************************************************************
         * Hash work
         ***********************************************************************************************/
        Hash proofHash("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        Hash workHash;
        Hash answerHash("0000000000000000000000000000000000000000000000000000000000000001");
        for(int i=0;i<8;++i)
        {
            proofHash.setByte(31, 0xff >> i);
            proofHash.getWork(workHash);
            if(workHash == answerHash)
                Log::addFormatted(Log::INFO, NEXTCASH_HASH_LOG_NAME,
                  "Passed hash work %d zeroes", i);
            else
            {
                Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME,
                  "Failed hash work %d zeroes : %s", i, workHash.hex().text());
                success = false;
            }
            answerHash <<= 1;
        }

        proofHash.setHex("0001ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        answerHash.setHex("0000000000000000000000000000000000000000000000000000000000008000");
        proofHash.getWork(workHash);
        if(workHash == answerHash)
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed hash work 0001");
        else
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Failed hash work 0001 : %s",
              workHash.hex().text());
            success = false;
        }

        /***********************************************************************************************
         * Hash container list
         ***********************************************************************************************/
        HashContainerList<String *> hashStringList;
        HashContainerList<String *>::Iterator retrieve;

        Hash l1(32);
        l1.randomize();
        Hash l2(32);
        l2.randomize();

        String *string1 = new String("test1");
        String *string2 = new String("test2");

        hashStringList.insertIfNotMatching(l1, string1, stringEqual);

        retrieve = hashStringList.get(l1);
        if(retrieve == hashStringList.end())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME,
              "Failed hash string list 0 : not found", workHash.hex().text());
            success = false;
        }
        else if(*retrieve != string1)
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Failed hash string list 0 : %s",
              (*retrieve)->text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed hash string list 0");

        hashStringList.insertIfNotMatching(l2, string2, stringEqual);

        retrieve = hashStringList.get(l1);
        if(retrieve == hashStringList.end())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME,
              "Failed hash string list 1 : not found", workHash.hex().text());
            success = false;
        }
        else if(*retrieve != string1)
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Failed hash string list 1 : %s",
              (*retrieve)->text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed hash string list 1");

        retrieve = hashStringList.get(l2);
        if(retrieve == hashStringList.end())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME,
              "Failed hash string list 2 : not found", workHash.hex().text());
            success = false;
        }
        else if(*retrieve != string2)
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Failed hash string list 2 : %s",
              (*retrieve)->text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed hash string list 2");

        Hash lr(32);
        String *newString = NULL;
        for(unsigned int i=0;i<100;++i)
        {
            lr.randomize();
            newString = new String();
            newString->writeFormatted("String %04d", Math::randomInt() % 1000);
            //hashStringList.insert(lr, newString);
            hashStringList.insertIfNotMatching(lr, newString, stringEqual);

            // for(HashContainerList<String *>::Iterator item=hashStringList.begin();item!=hashStringList.end();++item)
            // {
                // Log::addFormatted(Log::DEBUG, NEXTCASH_HASH_LOG_NAME, "Hash string list : %s <- %s",
                  // (*item)->text(), item.hash().hex().text());
            // }
        }

        retrieve = hashStringList.get(l1);
        if(retrieve == hashStringList.end())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME,
              "Failed hash string list r1 : not found", workHash.hex().text());
            success = false;
        }
        else if(*retrieve != string1)
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME,
              "Failed hash string list r1 : %s", (*retrieve)->text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed hash string list r1");

        retrieve = hashStringList.get(l2);
        if(retrieve == hashStringList.end())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME,
              "Failed hash string list r2 : not found", workHash.hex().text());
            success = false;
        }
        else if(*retrieve != string2)
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME,
              "Failed hash string list r2 : %s", (*retrieve)->text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed hash string list r2");

        String *firstString = new String("first");
        String *lastString = new String("last");

        l1.zeroize();
        hashStringList.insertIfNotMatching(l1, firstString, stringEqual);

        l1.setMax();
        hashStringList.insertIfNotMatching(l1, lastString, stringEqual);

        String *firstTest = NULL;
        String *lastTest = NULL;
        for(HashContainerList<String *>::Iterator item = hashStringList.begin();
          item != hashStringList.end(); ++item)
        {
            if(firstTest == NULL)
                firstTest = *item;
            lastTest = *item;
            // if(*item == NULL)
                // Log::addFormatted(Log::DEBUG, NEXTCASH_HASH_LOG_NAME, "Hash string list : NULL <- %s",
                  // item.hash().hex().text());
            // else
                // Log::addFormatted(Log::DEBUG, NEXTCASH_HASH_LOG_NAME, "Hash string list : %s <- %s",
                  // (*item)->text(), item.hash().hex().text());
        }

        if(firstTest == NULL)
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME,
              "Failed hash string list iterate first : not found", workHash.hex().text());
            success = false;
        }
        else if(firstTest != firstString)
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME,
              "Failed hash string list iterate first : %s", firstTest->text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed hash string list iterate first");

        if(lastTest == NULL)
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME,
              "Failed hash string list iterate last : not found", workHash.hex().text());
            success = false;
        }
        else if(lastTest != lastString)
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME,
              "Failed hash string list iterate last : %s", lastTest->text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed hash string list iterate last");

        l1.zeroize();
        retrieve = hashStringList.get(l1);
        if(retrieve == hashStringList.end())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME,
              "Failed hash string list get first : not found", workHash.hex().text());
            success = false;
        }
        else if(*retrieve != firstString)
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME,
              "Failed hash string list get first : %s", (*retrieve)->text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed hash string list get first");

        l1.setMax();
        retrieve = hashStringList.get(l1);
        if(retrieve == hashStringList.end())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME,
              "Failed hash string list get last : not found", workHash.hex().text());
            success = false;
        }
        else if(*retrieve != lastString)
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME,
              "Failed hash string list get last : %s", (*retrieve)->text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed hash string list get last");

        for(HashContainerList<String *>::Iterator item = hashStringList.begin();
          item != hashStringList.end(); ++item)
            if(*item != NULL)
                delete *item;

        return success;
    }
}
