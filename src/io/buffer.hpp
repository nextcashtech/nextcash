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
        Buffer(unsigned int pSize);
        ~Buffer();

        // InputStream virtual functions
        unsigned int readOffset() const { return mReadOffset; }
        unsigned int length() const { return mEndOffset; }
        void read(void *pOutput, unsigned int pSize);

        // OutputStream virtual functions
        unsigned int writeOffset() const { return mWriteOffset; }
        void write(const void *pInput, unsigned int pSize);

        uint8_t operator [](unsigned int pOffset) { return mData[mReadOffset + pOffset]; }
        void moveReadOffset(int pOffset);
        void setReadOffset(int pOffset) { mReadOffset = pOffset; } // Only safe when auto flush is false
        void setWriteOffset(int pOffset) { mWriteOffset = pOffset; } // Only safe when auto flush is false

        const uint8_t *startPointer() { return mData; }
        const uint8_t *readPointer() { return mData + mReadOffset; }
        const uint8_t *endPointer() { return mData + mEndOffset; }

        bool autoFlush() { return mAutoFlush; } // Will flush to allocate room when writing
        void setAutoFlush(bool pValue) { mAutoFlush = pValue; }
        void flush(); // Release read data

        void zeroize();
        void clear(); // Clear all data
        void compact(); // Reduce memory footprint. Flush first if you want to remove read data
        void setSize(unsigned int pSize); // Sets the allocated memory size (flushes if readOffset is not zero and autoFlush is true)

        // Reuse the memory of the other buffer.
        //   Only use this function when the pInput buffer will not change until after this buffer is done
        void copyBuffer(Buffer &pInput, unsigned int pSize);

        // The same as writeStream except allocates exactly enough memory and no extra
        void writeStreamCompact(InputStream &pInput, unsigned int pSize);

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
        unsigned int mSize, mReadOffset, mWriteOffset, mEndOffset;
        bool mAutoFlush;
        bool mSharing;

        // Copy data to unshared memory
        void unShare();

        void reallocate(unsigned int pSize);
    };
}

#endif
