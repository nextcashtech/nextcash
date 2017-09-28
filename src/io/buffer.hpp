/**************************************************************************
 * Copyright 2017 ArcMist, LLC                                            *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@arcmist.com>                                    *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#ifndef ARCMIST_BUFFER_HPP
#define ARCMIST_BUFFER_HPP

#include "stream.hpp"

#include <iostream>
#include <cstdint>


namespace ArcMist
{
    class Buffer : public ArcMist::Stream
    {
    public:

        Buffer();
        Buffer(const Buffer &pCopy);
        Buffer(stream_size pSize);
        ~Buffer();

        // InputStream virtual functions
        stream_size readOffset() const { return mReadOffset; }
        stream_size length() const { return mEndOffset; }
        void read(void *pOutput, stream_size pSize);

        // OutputStream virtual functions
        stream_size writeOffset() const { return mWriteOffset; }
        void write(const void *pInput, stream_size pSize);

        uint8_t operator [](stream_size pOffset) { return mData[mReadOffset + pOffset]; }
        void moveReadOffset(int pOffset);
        bool setReadOffset(stream_size pOffset) { mReadOffset = pOffset; return true; } // Only safe when auto flush is false
        bool setWriteOffset(stream_size pOffset) { mWriteOffset = pOffset; return true; } // Only safe when auto flush is false

        const uint8_t *startPointer() { return mData; }
        const uint8_t *readPointer() { return mData + mReadOffset; }
        const uint8_t *endPointer() { return mData + mEndOffset; }

        bool autoFlush() { return mAutoFlush; } // Will flush to allocate room when writing
        void setAutoFlush(bool pValue) { mAutoFlush = pValue; }
        void flush(); // Release read data

        void zeroize();
        void clear(); // Clear all data
        void compact(); // Reduce memory footprint. Flush first if you want to remove read data
        void setSize(stream_size pSize); // Sets the allocated memory size (flushes if readOffset is not zero and autoFlush is true)
        void reset() { mReadOffset = 0; mEndOffset = 0; mWriteOffset = 0; }

        // Reuse the memory of the other buffer.
        //   Only use this function when the pInput buffer will not change until after this buffer is done
        void copyBuffer(Buffer &pInput, stream_size pSize);

        // The same as writeStream except allocates exactly enough memory and no extra
        void writeStreamCompact(InputStream &pInput, stream_size pSize);

        const Buffer &operator = (const Buffer &pRight);

        bool operator == (Buffer &pRight) const
        {
            if(remaining() != pRight.remaining())
                return false;
            if(remaining() == 0)
                return true;
            return std::memcmp(mData + mReadOffset, pRight.mData + pRight.mReadOffset, remaining()) == 0;
        }
        bool operator != (Buffer &pRight) const
        {
            if(remaining() != pRight.remaining())
                return true;
            if(remaining() == 0)
                return false;
            return std::memcmp(mData + mReadOffset, pRight.mData + pRight.mReadOffset, remaining()) != 0;
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
