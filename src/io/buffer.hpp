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
        ~Buffer();

        // Stream virtual functions
        unsigned int readOffset() const { return mReadOffset; }
        unsigned int length() const { return mWriteOffset; }
        void read(void *pOutput, unsigned int pSize);
        unsigned int writeOffset() const { return mWriteOffset; }
        void write(const void *pInput, unsigned int pSize);

        unsigned char operator [](unsigned int pOffset) { return mData[mReadOffset + pOffset]; }
        void moveReadOffset(int pOffset);
        void setReadOffset(int pOffset) { mReadOffset = pOffset; } // Only safe when auto flush is false

        const unsigned char *startPointer() { return mData; }
        const unsigned char *readPointer() { return mData + mReadOffset; }
        const unsigned char *endPointer() { return mData + mWriteOffset; }

        bool autoFlush() { return mAutoFlush; } // Will flush to allocate room when writing
        void setAutoFlush(bool pValue) { mAutoFlush = pValue; }
        void flush(); // Release read data

        void zeroize();
        void clear(); // Clear all data
        void compact(); // Reduce mempory footprint. Flush first if you want to remove read data

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
        unsigned int mSize, mReadOffset, mWriteOffset;
        bool mAutoFlush;

        void reallocate(unsigned int pSize);
    };
}

#endif
