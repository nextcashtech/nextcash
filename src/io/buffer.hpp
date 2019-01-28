/**************************************************************************
 * Copyright 2017 NextCash, LLC                                           *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@nextcash.tech>                                  *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#ifndef NEXTCASH_BUFFER_HPP
#define NEXTCASH_BUFFER_HPP

#include "stream.hpp"

#include <iostream>
#include <cstdint>


namespace NextCash
{
    class Buffer : public NextCash::Stream
    {
    public:

        Buffer();
        Buffer(const Buffer &pCopy);
        Buffer(Buffer &pCopy, bool pShare = false);
        Buffer(stream_size pSize);
        ~Buffer();

        // InputStream virtual functions
        stream_size readOffset() const { return mReadOffset; }
        stream_size length() const { return mEndOffset; }
        bool read(void *pOutput, stream_size pSize);

        // OutputStream virtual functions
        stream_size writeOffset() const { return mWriteOffset; }
        void write(const void *pInput, stream_size pSize);

        uint8_t operator [](stream_size pOffset) { return mData[mReadOffset + pOffset]; }
        void moveReadOffset(int pOffset);
        bool setReadOffset(stream_size pOffset) { mReadOffset = pOffset; return true; } // Only safe when auto flush is false
        bool setWriteOffset(stream_size pOffset) { mWriteOffset = pOffset; return true; } // Only safe when auto flush is false

        uint8_t *begin() { return mData; }
        uint8_t *current() { return mData + mReadOffset; }
        uint8_t *end() { return mData + mEndOffset; }

        bool autoFlush() { return mAutoFlush; } // Will flush to allocate room when writing
        void setAutoFlush(bool pValue) { mAutoFlush = pValue; }
        void flush(NextCash::stream_size pMinimumSize = 1024); // Release read data

        void zeroize();
        void clear(); // Clear all data
        void compact(); // Reduce memory footprint. Flush first if you want to remove read data
        void setSize(stream_size pSize); // Sets the allocated memory size (flushes if readOffset is not zero and autoFlush is true)
        void setEnd(stream_size pLength); // Sets the end of the stream at the specified length
        void reset() { mReadOffset = 0; mEndOffset = 0; mWriteOffset = 0; }

        // Reuse the memory of the other buffer.
        //   Only use this function when the pInput buffer will not change until after this buffer is done
        void copyBuffer(Buffer &pInput, stream_size pSize);

        // The same as writeStream except allocates exactly enough memory and no extra
        void writeStreamCompact(InputStream &pInput, stream_size pSize);

        const Buffer &operator = (const Buffer &pRight);

        bool operator == (Buffer &pRight) const
        {
            if(length() != pRight.length())
                return false;
            if(length() == 0)
                return true;
            return std::memcmp(mData, pRight.mData, length()) == 0;
        }
        bool operator != (Buffer &pRight) const
        {
            if(length() != pRight.length())
                return true;
            if(length() == 0)
                return false;
            return std::memcmp(mData, pRight.mData, length()) != 0;
        }

        static bool test();

    private:

        uint8_t *mData;
        stream_size mSize, mReadOffset, mWriteOffset, mEndOffset;
        bool mAutoFlush;
        bool mSharing;

        // Copy data to unshared memory
        void unShare();

        void reallocate(stream_size pSize);
    };
}

#endif
