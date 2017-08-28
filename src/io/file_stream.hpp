#ifndef ARCMIST_CSTREAM_HPP
#define ARCMIST_CSTREAM_HPP

#include "stream.hpp"

#include <iostream>
#include <fstream>
#include <cstdint>


namespace ArcMist
{
    inline bool fileExists(const char *pPathFileName)
    {
        std::ifstream f(pPathFileName);
        return f.good();
    }

    class FileInputStream : public InputStream
    {
    public:

        FileInputStream(std::istream &pInputStream)
        {
            mStreamNeedsDelete = false;
            mStream = &pInputStream;
            initialize();
        }
        FileInputStream(const char *pFilePathName)
        {
            mStreamNeedsDelete = true;
            mStream = new std::ifstream();
            ((std::fstream *)mStream)->open(pFilePathName, std::ios::binary);
            initialize();
        }
        ~FileInputStream() { if(mStreamNeedsDelete) delete mStream; }

        unsigned int readOffset() const { return mReadOffset; }
        unsigned int length() const { return mEndOffset; }
        operator bool() const { return mStream->good() && (mReadOffset == (unsigned int)-1 || mReadOffset < mEndOffset); }
        bool operator !() const { return !mStream->good(); }
        void read(void *pOutput, unsigned int pSize)
        {
            mStream->read((char *)pOutput, pSize);
            if(mReadOffset != (unsigned int)-1)
            {
                mReadOffset += pSize;
                if(mEndOffset < mReadOffset)
                    mEndOffset = mReadOffset;
            }
        }

    private:

        // Setup offsets
        void initialize()
        {
            if(mStream->tellg() == -1)
            {
                mReadOffset = -1;
                mEndOffset = -1;
                return;
            }

            mReadOffset = mStream->tellg();
            mStream->seekg(0, std::ios::end);
            mEndOffset = mStream->tellg();
            mStream->seekg(mReadOffset, std::ios::beg);
        }

        bool mStreamNeedsDelete;
        std::istream *mStream;
        unsigned int mReadOffset, mEndOffset;
    };

    class FileOutputStream : public OutputStream
    {
    public:

        FileOutputStream(std::ostream &pOutputStream)
        {
            mStreamNeedsDelete = false;
            mStream = &pOutputStream;
            initialize();
        }
        FileOutputStream(const char *pFilePathName, bool pAppend = false, bool pTruncate = false)
        {
            mStreamNeedsDelete = true;
            mStream = new std::ofstream();
            std::ios_base::openmode mode = std::ios::binary;
            if(pTruncate)
                mode |= std::ios::trunc;
            else if(pAppend)
                mode |= std::ios::app;
            ((std::fstream *)mStream)->open(pFilePathName, mode);
            initialize();
        }
        ~FileOutputStream() { mStream->flush(); if(mStreamNeedsDelete) delete mStream; }

        unsigned int writeOffset() const { return mWriteOffset; }
        void write(const void *pInput, unsigned int pSize)
        {
            mStream->write((const char *)pInput, pSize);
            if(mWriteOffset != (unsigned int)-1)
            {
                mWriteOffset += pSize;
                if(mEndOffset < mWriteOffset)
                    mEndOffset = mWriteOffset;
            }
        }
        void flush() { mStream->flush(); }

    private:

        // Setup offsets
        void initialize()
        {
            if(mStream->tellp() == -1)
            {
                mWriteOffset = -1;
                mEndOffset = -1;
                return;
            }

            mWriteOffset = mStream->tellp();
            mStream->seekp(0, std::ios::end);
            mEndOffset = mStream->tellp();
            mStream->seekp(mWriteOffset, std::ios::beg);
        }

        bool mStreamNeedsDelete;
        std::ostream *mStream;
        unsigned int mWriteOffset, mEndOffset;
    };

    class FileStream : public Stream
    {
    public:

        FileStream(std::iostream &pStream)
        {
            mStreamNeedsDelete = false;
            mStream = &pStream;
            initialize();
        }
        FileStream(const char *pFilePathName, bool pAppend = false, bool pTruncate = false)
        {
            mStreamNeedsDelete = true;
            mStream = new std::fstream();
            std::ios_base::openmode mode = std::ios::binary;
            if(pTruncate)
                mode |= std::ios::trunc;
            else if(pAppend)
                mode |= std::ios::app;
            ((std::fstream *)mStream)->open(pFilePathName, mode);
            initialize();
        }
        ~FileStream() { mStream->flush(); if(mStreamNeedsDelete) delete mStream; }

        unsigned int readOffset() const { return mReadOffset; }
        void setReadOffset(unsigned int pOffset)
        {
            mStream->seekg(pOffset);
            mReadOffset = pOffset;
        }
        unsigned int length() const { return mEndOffset; }
        operator bool() const { return mStream->good() && mReadOffset < mEndOffset; }
        bool operator !() const { return !mStream->good(); }
        void read(void *pOutput, unsigned int pSize)
        {
            mStream->read((char *)pOutput, pSize);
            mReadOffset += pSize;
        }
        unsigned int writeOffset() const { return mWriteOffset; }
        void setWriteOffset(unsigned int pOffset)
        {
            mStream->seekp(pOffset);
            mWriteOffset = pOffset;
        }
        void write(const void *pInput, unsigned int pSize)
        {
            mStream->write((const char *)pInput, pSize);
            mWriteOffset += pSize;
            if(mEndOffset < mWriteOffset)
                mEndOffset = mWriteOffset;
        }
        void flush() { mStream->flush(); }

    private:

        // Setup offsets
        void initialize()
        {
            mWriteOffset = mStream->tellp();
            mReadOffset = mStream->tellg();
            mStream->seekg(0, std::ios::end);
            mEndOffset = mStream->tellg();
            mStream->seekg(mReadOffset, std::ios::beg);
        }

        bool mStreamNeedsDelete;
        std::iostream *mStream;
        unsigned int mReadOffset, mWriteOffset, mEndOffset;
    };
}

#endif
