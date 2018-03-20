/**************************************************************************
 * Copyright 2017 NextCash, LLC                                            *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@nextcash.com>                                    *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#include "buffer.hpp"

#include "nextcash/base/endian.hpp"
#include "nextcash/base/string.hpp"
#include "nextcash/base/log.hpp"

#include <cstring>
#include <new>

#define NEXTCASH_BUFFER_LOG_NAME "Buffer"


namespace NextCash
{
    Buffer::Buffer()
    {
        mData = NULL;
        mSize = 0;
        mReadOffset = 0;
        mWriteOffset = 0;
        mEndOffset = 0;
        mAutoFlush = true;
        mSharing = false;
    }

    Buffer::Buffer(const Buffer &pCopy)
    {
        mSize = 0;
        mData = NULL;
        mSharing = false;
        *this = pCopy;
    }

    Buffer::Buffer(stream_size pSize)
    {
        mSize = pSize;
        try
        {
            mData = new uint8_t[mSize];
        }
        catch(std::bad_alloc &pBadAlloc)
        {
            NextCash::Log::addFormatted(NextCash::Log::ERROR, NEXTCASH_BUFFER_LOG_NAME, "Bad allocation : %s", pBadAlloc.what());
            mData = NULL;
            mSize = 0;
        }
        catch(...)
        {
            NextCash::Log::add(NextCash::Log::ERROR, NEXTCASH_BUFFER_LOG_NAME, "Bad allocation : unknown");
            mData = NULL;
            mSize = 0;
        }
        mReadOffset = 0;
        mWriteOffset = 0;
        mEndOffset = 0;
        mAutoFlush = true;
        mSharing = false;
    }

    Buffer::~Buffer()
    {
        if(mData != NULL && !mSharing)
            delete[] mData;
    }

    const Buffer &Buffer::operator = (const Buffer &pRight)
    {
        clear();
        mSize = pRight.mEndOffset;
        if(mSize > 0)
        {
            mData = new uint8_t[mSize];
            std::memcpy(mData, pRight.mData, pRight.mEndOffset);
        }
        else
            mData = NULL;
        mReadOffset = pRight.mReadOffset;
        mWriteOffset = pRight.mWriteOffset;
        mEndOffset = pRight.mEndOffset;
        mAutoFlush = pRight.mAutoFlush;
        mSharing = false;
        return *this;
    }

    void Buffer::read(void *pOutput, stream_size pSize)
    {
        stream_size toRead = pSize;
        if(toRead > remaining())
            toRead = remaining();
        if(toRead > 0)
        {
            std::memcpy(pOutput, mData + mReadOffset, toRead);
            mReadOffset += toRead;
        }
    }

    void Buffer::write(const void *pInput, stream_size pSize)
    {
        if(pSize == 0)
            return;

        if(mSharing)
            unShare();

        if(mData == NULL || mSize - mWriteOffset < pSize)
            reallocate(pSize);

        std::memcpy(mData + mWriteOffset, pInput, pSize);
        mWriteOffset += pSize;
        if(mWriteOffset > mEndOffset)
            mEndOffset = mWriteOffset;
    }

    // Reuse the memory of the other buffer.
    //   Only use this function when the pInput buffer will not change until after this buffer is done
    void Buffer::copyBuffer(Buffer &pInput, stream_size pSize)
    {
        clear();
        mSharing = true;
        mData = pInput.mData + pInput.mReadOffset;
        mSize = pSize;
        mReadOffset = 0;
        mWriteOffset = pSize;
        mEndOffset = pSize;
        pInput.mReadOffset += pSize;
    }

    // The same as writeStream except allocates exactly enough memory and no extra
    void Buffer::writeStreamCompact(InputStream &pInput, stream_size pSize)
    {
        clear();
        mData = new uint8_t[pSize];
        pInput.read(mData, pSize);
        mSize = pSize;
        mReadOffset = 0;
        mWriteOffset = pSize;
        mEndOffset = pSize;
        mSharing = false;
    }

    void Buffer::moveReadOffset(int pOffset)
    {
        if(mReadOffset + pOffset > mWriteOffset)
        {
            Log::error(NEXTCASH_BUFFER_LOG_NAME, "Move read offset too large");
            mReadOffset = mWriteOffset;
        }
        else if(mReadOffset + pOffset < 0)
        {
            Log::error(NEXTCASH_BUFFER_LOG_NAME, "Move read offset too small");
            mReadOffset = 0;
        }
        else
            mReadOffset += pOffset;
    }

    void Buffer::zeroize()
    {
        if(mData == NULL)
            return;
        if(mSharing)
            unShare();

        std::memset(mData, 0, mSize);
    }

    void Buffer::clear()
    {
        mReadOffset = 0;
        mWriteOffset = 0;
        mEndOffset = 0;
        if(mData)
        {
            if(!mSharing)
                delete[] mData;
            mData = NULL;
            mSize = 0;
        }
        mSharing = false;
    }

    void Buffer::unShare()
    {
        if(mData == NULL || !mSharing)
            return;

        uint8_t *newData = new uint8_t[mSize];
        std::memcpy(newData, mData, mSize);
        mData = newData;
        mSharing = false;
    }

    void Buffer::compact()
    {
        if(mSharing)
            unShare();

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
        uint8_t *newData = NULL;
        try
        {
            newData = new uint8_t[mEndOffset];
        }
        catch(std::bad_alloc &pBadAlloc)
        {
            NextCash::Log::addFormatted(NextCash::Log::ERROR, NEXTCASH_BUFFER_LOG_NAME, "Bad allocation : %s", pBadAlloc.what());
            return;
        }
        catch(...)
        {
            NextCash::Log::add(NextCash::Log::ERROR, NEXTCASH_BUFFER_LOG_NAME, "Bad allocation : unknown");
            return;
        }
        std::memcpy(newData, mData, mEndOffset);

        // Remove and replace old memory
        delete[] mData;
        mData = newData;
        mSize = mEndOffset;
    }

    void Buffer::setSize(stream_size pSize)
    {
        if(mSharing)
            unShare();

        if(mSize - mReadOffset >= pSize)
            return; // Already big enough

        if(mEndOffset == 0)
        {
            // No data in buffer
            delete[] mData;
            mData = NULL;
        }

        // Allocate and populate new memory
        uint8_t *newData = NULL;
        unsigned newSize = pSize;
        if(!mAutoFlush)
            newSize += mReadOffset;

        try
        {
            newData = new uint8_t[newSize];
        }
        catch(std::bad_alloc &pBadAlloc)
        {
            NextCash::Log::addFormatted(NextCash::Log::ERROR, NEXTCASH_BUFFER_LOG_NAME, "Bad allocation : %s", pBadAlloc.what());
            return;
        }
        catch(...)
        {
            NextCash::Log::add(NextCash::Log::ERROR, NEXTCASH_BUFFER_LOG_NAME, "Bad allocation : unknown");
            return;
        }

        if(mData != NULL)
        {
            // Flush read data
            if(mAutoFlush)
            {
                mEndOffset -= mReadOffset;
                std::memcpy(newData, mData + mReadOffset, mEndOffset);
                mWriteOffset -= mReadOffset;
                mReadOffset = 0;
            }
            else
                std::memcpy(newData, mData, mEndOffset);

            // Remove and replace old memory
            delete[] mData;
        }

        mData = newData;
        mSize = newSize;
    }

    // Flush any read bytes
    void Buffer::flush()
    {
        if(mSharing)
            unShare();

        if(mReadOffset == 0)
            return;

        if(mReadOffset >= mEndOffset)
        {
            if(mReadOffset > mEndOffset)
                Log::error(NEXTCASH_BUFFER_LOG_NAME, "Flush with read offset higher than end offset");
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
    void Buffer::reallocate(stream_size pSize)
    {
        if(mSharing)
            unShare();

        // Allocate new memory
        stream_size usedBytes;
        stream_size newSize = mSize;

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

        uint8_t *newData = NULL;
        try
        {
            newData = new uint8_t[newSize];
        }
        catch(std::bad_alloc &pBadAlloc)
        {
            NextCash::Log::addFormatted(NextCash::Log::ERROR, NEXTCASH_BUFFER_LOG_NAME, "Bad allocation : %s", pBadAlloc.what());
            return;
        }
        catch(...)
        {
            NextCash::Log::add(NextCash::Log::ERROR, NEXTCASH_BUFFER_LOG_NAME, "Bad allocation : unknown");
            return;
        }

        if(mData != NULL)
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
        Log::add(NextCash::Log::INFO, NEXTCASH_BUFFER_LOG_NAME,
          "------------- Starting Buffer Tests -------------");

        bool result = true;

        /******************************************************************************************
         * Test read hex string function
         ******************************************************************************************/
        Buffer hexBinary;
        hexBinary.setOutputEndian(Endian::BIG);
        hexBinary.writeUnsignedInt(0x123456ff);
        String hexValue = hexBinary.readHexString(4);
        if(hexValue == "123456ff") // hex numbers in gdb are big endian
            Log::add(Log::INFO, NEXTCASH_BUFFER_LOG_NAME, "Passed read hex string function");
        else
        {
            Log::add(Log::ERROR, NEXTCASH_BUFFER_LOG_NAME, "Failed read hex string function");
            result = false;
        }

        /******************************************************************************************
         * Test read hex string function little endian
         ******************************************************************************************/
        hexBinary.setOutputEndian(Endian::LITTLE);
        hexBinary.writeUnsignedInt(0x123456ff);
        hexValue = hexBinary.readHexString(4);
        if(hexValue == "ff563412")
            Log::add(Log::INFO, NEXTCASH_BUFFER_LOG_NAME, "Passed read hex string function little endian");
        else
        {
            Log::add(Log::ERROR, NEXTCASH_BUFFER_LOG_NAME, "Failed read hex string function little endian");
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
            Log::addFormatted(Log::INFO, NEXTCASH_BUFFER_LOG_NAME, "Passed write base58 string function: %s", hexValue.text());
        else
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_BUFFER_LOG_NAME, "Failed write base58 string function : %s", hexValue.text());
            result = false;
        }

        /******************************************************************************************
         * Test write base32 string function
         ******************************************************************************************/
        Buffer base32Binary;
        String base32String = "vc";

        base32Binary.clear();
        base32Binary.writeBase32AsBinary(base32String.text());
        hexValue = base32Binary.readHexString(base32Binary.length());
        if(base32Binary.length() == 2 && hexValue == "6600")
            Log::addFormatted(Log::INFO, NEXTCASH_BUFFER_LOG_NAME, "Passed write base32 string function: %s", hexValue.text());
        else
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_BUFFER_LOG_NAME, "Failed write base32 string function : %s", hexValue.text());
            result = false;
        }

        base32String = "w3jhxaq";
        base32Binary.clear();
        base32Binary.writeBase32AsBinary(base32String.text());
        hexValue = base32Binary.readHexString(base32Binary.length());
        if(base32Binary.length() == 5 && hexValue == "7465737400")
            Log::addFormatted(Log::INFO, NEXTCASH_BUFFER_LOG_NAME, "Passed write base32 string function: %s", hexValue.text());
        else
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_BUFFER_LOG_NAME, "Failed write base32 string function : %s", hexValue.text());
            result = false;
        }

        return result;
    }
}
