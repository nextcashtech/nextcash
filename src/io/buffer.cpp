#include "buffer.hpp"

#include "arcmist/base/endian.hpp"
#include "arcmist/base/string.hpp"
#include "arcmist/base/log.hpp"

#include <cstring>

#define BUFFER_LOG_NAME "Buffer"


namespace ArcMist
{
    Buffer::Buffer()
    {
        mData = NULL;
        mSize = 0;
        mReadOffset = 0;
        mWriteOffset = 0;
        mEndOffset = 0;
        mAutoFlush = true;
    }

    Buffer::~Buffer()
    {
        if(mData)
            delete[] mData;
    }

    void Buffer::read(void *pOutput, unsigned int pSize)
    {
        unsigned int toRead = pSize;
        if(toRead > remaining())
            toRead = remaining();
        if(toRead > 0)
        {
            std::memcpy(pOutput, mData + mReadOffset, toRead);
            mReadOffset += toRead;
        }
    }

    void Buffer::write(const void *pInput, unsigned int pSize)
    {
        if(pSize == 0)
            return;

        if(mData == NULL || mSize - mWriteOffset < pSize)
            reallocate(pSize);

        std::memcpy(mData + mWriteOffset, pInput, pSize);
        mWriteOffset += pSize;
        if(mWriteOffset > mEndOffset)
            mEndOffset = mWriteOffset;
    }

    void Buffer::moveReadOffset(int pOffset)
    {
        if(mReadOffset + pOffset > mWriteOffset)
        {
            Log::error(BUFFER_LOG_NAME, "Move read offset too large");
            mReadOffset = mWriteOffset;
        }
        else if(mReadOffset + pOffset < 0)
        {
            Log::error(BUFFER_LOG_NAME, "Move read offset too small");
            mReadOffset = 0;
        }
        else
            mReadOffset += pOffset;
    }

    void Buffer::zeroize()
    {
        if(mData == NULL)
            return;

        std::memset(mData, 0, mSize);
    }

    void Buffer::clear()
    {
        mReadOffset = 0;
        mWriteOffset = 0;
        mEndOffset = 0;
        if(mData)
        {
            delete[] mData;
            mData = NULL;
            mSize = 0;
        }
    }

    void Buffer::compact()
    {
        if(mData == NULL)
            return; // Empty

        if(mEndOffset == mSize)
            return; // Already compact

        if(mEndOffset == 0)
        {
            delete[] mData;
            mData = NULL;
            return;
        }

        // Allocate and populate new memory
        uint8_t *newData = new uint8_t[mEndOffset];
        std::memcpy(newData, mData, mEndOffset);

        // Remove and replace old memory
        delete[] mData;
        mData = newData;
        mSize = mEndOffset;
    }

    // Flush any read bytes
    void Buffer::flush()
    {
        if(mReadOffset == 0)
            return;

        if(mReadOffset >= mEndOffset)
        {
            if(mReadOffset > mEndOffset)
                Log::error(BUFFER_LOG_NAME, "Flush with read offset higher than end offset");
            mReadOffset  = 0;
            mWriteOffset = 0;
            mEndOffset = 0;
            return;
        }

        
        std::memmove(mData, mData + mReadOffset, mEndOffset - mReadOffset);
        mEndOffset -= mReadOffset;
        mWriteOffset -= mReadOffset;
        mReadOffset = 0;
    }

    // Reallocate to have at least pSize bytes additional memory
    void Buffer::reallocate(unsigned int pSize)
    {
        // Allocate new memory
        unsigned int usedBytes;
        unsigned int newSize = mSize;

        if(newSize < 1024)
            newSize = 1024;

        if(mAutoFlush)
            usedBytes = remaining();
        else
            usedBytes = length();

        while(newSize - usedBytes < pSize)
            newSize *= 2;

        if(newSize <= mSize)
        {
            if(mAutoFlush)
                flush();
            return;
        }

        uint8_t *newData = new uint8_t[newSize];

        if(mData)
        {
            // Copy old data into new memory
            if(mAutoFlush)
            {
                std::memcpy(newData, mData + mReadOffset, mEndOffset - mReadOffset);
                mWriteOffset -= mReadOffset;
                mEndOffset -= mReadOffset;
                mReadOffset = 0;
            }
            else
                std::memcpy(newData, mData, mEndOffset);

            // Delete old memory
            delete[] mData;
        }

        mData = newData;
        mSize = newSize;
    }

    bool Buffer::test()
    {
        bool result = true;

        /******************************************************************************************
         * Test read hex string function
         ******************************************************************************************/
        Buffer hexBinary;
        hexBinary.setInputEndian(Endian::LITTLE);
        hexBinary.writeUnsignedInt(0x123456ff);
        String hexValue = hexBinary.readHexString(4);
        if(hexValue == "123456ff")
            Log::add(Log::INFO, BUFFER_LOG_NAME, "Passed read hex string function");
        else
        {
            Log::add(Log::ERROR, BUFFER_LOG_NAME, "Failed read hex string function");
            result = false;
        }

        /******************************************************************************************
         * Test write base58 string function
         ******************************************************************************************/
        Buffer base58Binary;
        String base58String = "12FpmoFq5cpWVRp4dCgkYB3HiTzx7";
        base58Binary.writeBase58AsBinary(base58String.text());
        hexValue = base58Binary.readHexString(21);
        if(hexValue == "005a1fc5dd9e6f03819fca94a2d89669469667f9a0")
            Log::addFormatted(Log::INFO, BUFFER_LOG_NAME, "Passed write base58 string function: %s", hexValue.text());
        else
        {
            Log::addFormatted(Log::ERROR, BUFFER_LOG_NAME, "Failed write base58 string function : %s", hexValue.text());
            result = false;
        }

        return result;
    }
}
