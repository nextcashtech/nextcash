/**************************************************************************
 * Copyright 2017 ArcMist, LLC                                            *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@arcmist.com>                                    *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#ifndef ARCMIST_CSTREAM_HPP
#define ARCMIST_CSTREAM_HPP

#include "stream.hpp"

#include <iostream>
#include <fstream>
#include <cstdint>
#include <cstdlib>


namespace ArcMist
{
    inline bool fileExists(const char *pPathFileName)
    {
        std::ifstream f(pPathFileName);
        return f.good();
    }

    inline bool removeFile(const char *pPathFileName)
    {
        return std::remove(pPathFileName) == 0;
    }

    inline bool createDirectory(const char *pPathName)
    {
        String command = "mkdir -p ";
        command += pPathName;
        int result = system(command.text());
        return result >= 0;
    }

    class FileInputStream : public InputStream
    {
    public:

        FileInputStream(std::istream &pInputStream)
        {
            mValid = true;
            mStreamNeedsDelete = false;
            mStream = &pInputStream;
            initialize();
        }
        FileInputStream(const char *pFilePathName)
        {
            mStreamNeedsDelete = true;
            mStream = new std::fstream;
            ((std::fstream *)mStream)->open(pFilePathName, std::ios::in | std::ios::binary);
            mValid = ((std::fstream *)mStream)->is_open();
            initialize();
        }
        ~FileInputStream() { if(mStreamNeedsDelete) delete mStream; }

        void close() { ((std::fstream *)mStream)->close();}

        bool isValid() { return mValid; }
        unsigned int length() const { return mEndOffset; }
        unsigned int readOffset() const { return mReadOffset; }
        bool setReadOffset(unsigned int pOffset)
        {
            if(pOffset <= mEndOffset)
            {
                mStream->seekg(pOffset);
                mReadOffset = mStream->tellg();
                return true;
            }
            return false;
        }
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

        bool mValid;
        bool mStreamNeedsDelete;
        std::istream *mStream;
        unsigned int mReadOffset, mEndOffset;
    };

    class FileOutputStream : public OutputStream
    {
    public:

        FileOutputStream(std::ostream &pOutputStream)
        {
            mValid = true;
            mStreamNeedsDelete = false;
            mStream = &pOutputStream;
            initialize();
        }
        FileOutputStream(const char *pFilePathName, bool pTruncate = false, bool pAppend = false)
        {
            mStreamNeedsDelete = true;
            std::ios_base::openmode mode = std::ios::out | std::ios::binary;
            if(pTruncate)
                mode |= std::ios::trunc;
            else if(pAppend)
                mode |= std::ios::app;
            else if(fileExists(pFilePathName))
                mode |= std::ios::in; // This "hack" is required to prevent a file that already exists from being deleted. However if the file doesn't yet exist this causes failure.
            mStream = new std::fstream;
            ((std::fstream *)mStream)->open(pFilePathName, mode);
            mValid = ((std::fstream *)mStream)->is_open();
            initialize();
        }
        ~FileOutputStream() { mStream->flush(); if(mStreamNeedsDelete) delete mStream; }

        void close() { mStream->flush(); ((std::fstream *)mStream)->close();}

        bool isValid() { return mValid; }
        unsigned int length() const { return mEndOffset; }
        unsigned int writeOffset() const { return mWriteOffset; }
        bool setWriteOffset(unsigned int pOffset)
        {
            if(pOffset <= mEndOffset)
            {
                flush();
                mStream->seekp(pOffset);
                mWriteOffset = mStream->tellp();
                return true;
            }
            return false;
        }
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

        bool mValid;
        bool mStreamNeedsDelete;
        std::ostream *mStream;
        unsigned int mWriteOffset, mEndOffset;
    };
}

#endif
