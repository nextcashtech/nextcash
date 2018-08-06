/**************************************************************************
 * Copyright 2017 NextCash, LLC                                           *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@nextcash.tech>                                  *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#ifndef NEXTCASH_FILESTREAM_HPP
#define NEXTCASH_FILESTREAM_HPP

#include "stream.hpp"

#include <iostream>
#include <fstream>
#include <cstdint>
#include <cstdlib>


namespace NextCash
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

    inline bool removeDirectory(const char *pPathDirectoryName)
    {
        String command = "rm -r ";
        command += pPathDirectoryName;
        int result = system(command.text());
        return result >= 0;
    }

    inline bool renameFile(const char *pSourceFileName, const char *pDestinationFileName)
    {
        String command = "mv ";
        command += pSourceFileName;
        command += " ";
        command += pDestinationFileName;
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
            mStream = new std::fstream();
            ((std::fstream *)mStream)->open(pFilePathName, std::ios_base::in | std::ios_base::binary);
            mValid = ((std::fstream *)mStream)->is_open();
            initialize();
        }
        ~FileInputStream() { if(mStreamNeedsDelete) delete mStream; }

        void close() { ((std::fstream *)mStream)->close();}

        bool isValid() { return mValid; }
        stream_size length() const { return mEndOffset; }
        stream_size readOffset() const { return mReadOffset; }
        bool setReadOffset(stream_size pOffset)
        {
            if(pOffset <= mEndOffset)
            {
                if(mStream->eof())
                    mStream->clear(); // Clear eof flag
                mStream->seekg(pOffset);
                mReadOffset = mStream->tellg();
                return true;
            }
            return false;
        }
        operator bool() const { return mStream->good() && (mReadOffset == (stream_size)-1 || mReadOffset < mEndOffset); }
        bool operator !() const { return !mStream->good(); }
        void read(void *pOutput, stream_size pSize)
        {
            mStream->read((char *)pOutput, pSize);
            if(mReadOffset != INVALID_STREAM_SIZE && mReadOffset < mEndOffset)
                mReadOffset += pSize;
        }

    private:

        // Setup offsets
        void initialize()
        {
            if(mStream->tellg() == -1)
            {
                mReadOffset = INVALID_STREAM_SIZE;
                mEndOffset = INVALID_STREAM_SIZE;
                return;
            }

            mReadOffset = mStream->tellg();
            mStream->seekg(0, std::ios_base::end);
            mEndOffset = mStream->tellg();
            mStream->seekg(mReadOffset, std::ios_base::beg);
        }

        bool mValid;
        bool mStreamNeedsDelete;
        std::istream *mStream;
        stream_size mReadOffset, mEndOffset;
    };

    class FileOutputStream : public OutputStream
    {
    public:

        FileOutputStream(std::ostream &pOutputStream)
        {
            mValid = true;
            mStreamNeedsDelete = false;
            mStream = &pOutputStream;
            initialize(false);
        }
        FileOutputStream(const char *pFilePathName, bool pTruncate = false, bool pAppend = false)
        {
            mStreamNeedsDelete = true;
            std::ios_base::openmode mode = std::ios_base::out | std::ios_base::binary;
            if(pTruncate)
                mode |= std::ios_base::trunc;
            else
            {
                // This "hack" is required to prevent a file that already exists from being
                //   deleted. However if the file doesn't yet exist this causes failure.
                mode |= std::ios_base::in;
            }
            mStream = new std::ofstream(pFilePathName, mode);

            if(!pTruncate && !mStream->good())
            {
                // Remove "in" flag and try to open again
                mode ^= std::ios_base::in;
                delete mStream;
                mStream = new std::ofstream(pFilePathName, mode);
            }

            mValid = ((std::ofstream *)mStream)->is_open();
            initialize(pAppend);
        }
        ~FileOutputStream() { mStream->flush(); if(mStreamNeedsDelete) delete mStream; }

        void close() { mStream->flush(); ((std::fstream *)mStream)->close();}

        bool isValid() { return mValid; }
        stream_size length() const { return mEndOffset; }
        stream_size writeOffset() const { return mWriteOffset; }
        bool setWriteOffset(stream_size pOffset)
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
        void write(const void *pInput, stream_size pSize)
        {
            mStream->write((const char *)pInput, pSize);
            if(mWriteOffset != INVALID_STREAM_SIZE)
            {
                mWriteOffset += pSize;
                if(mEndOffset < mWriteOffset)
                    mEndOffset = mWriteOffset;
            }
        }
        void flush() { mStream->flush(); }

    private:

        // Setup offsets
        void initialize(bool pAppend)
        {
            if(mStream->tellp() == -1)
            {
                mWriteOffset = INVALID_STREAM_SIZE;
                mEndOffset = INVALID_STREAM_SIZE;
                return;
            }

            // Go to end of file to determine size
            mStream->seekp(0, std::ios_base::end);
            mEndOffset = mStream->tellp();

            if(pAppend)
                mWriteOffset = mEndOffset; // Stay at end of file
            else
            {
                // Go back to beginning of file
                mWriteOffset = 0;
                mStream->seekp(mWriteOffset, std::ios_base::beg);
            }
        }

        bool mValid;
        bool mStreamNeedsDelete;
        std::ostream *mStream;
        stream_size mWriteOffset, mEndOffset;
    };

    // Note: Seems to corrupt the read buffer after some writes and then attempting to read again
    // class FileStream : public InputStream, public OutputStream
    // {
    // public:

        // FileStream(const char *pFilePathName, bool pTruncate = false, bool pAppend = false)
        // {
            // if(!fileExists(pFilePathName)) // Create the file so it can be successfully opened with std::ios_base::in
                // std::fstream createFile(pFilePathName, std::ios_base::out | std::ios_base::binary);
            // std::ios_base::openmode mode = std::ios_base::out | std::ios_base::in | std::ios_base::binary;
            // if(pTruncate)
                // mode |= std::ios_base::trunc;
            // else if(pAppend)
                // mode |= std::ios_base::app;
            // mStream.open(pFilePathName, mode);
            // mValid = mStream.is_open();
            // initialize();
        // }
        // ~FileStream() { mStream.flush(); }

        // void close() { mStream.flush(); mStream.close();}

        // bool isValid()
        // {
            // if(mValid && mStream.fail())
                // mValid = false;
            // return mValid;
        // }
        // stream_size length() const { return mEndOffset; }

        // // Output Stream
        // stream_size writeOffset() const { return mWriteOffset; }
        // bool setWriteOffset(stream_size pOffset)
        // {
            // if(pOffset <= mEndOffset)
            // {
                // flush();
                // mStream.seekp(pOffset);
                // mWriteOffset = mStream.tellp();
                // return true;
            // }
            // return false;
        // }
        // void write(const void *pInput, stream_size pSize)
        // {
            // mStream.write((const char *)pInput, pSize);
            // if(mWriteOffset != INVALID_STREAM_SIZE)
            // {
                // mWriteOffset += pSize;
                // if(mEndOffset < mWriteOffset)
                    // mEndOffset = mWriteOffset;
            // }
        // }
        // void flush() { mStream.flush(); }

        // // Input Stream
        // stream_size readOffset() const { return mReadOffset; }
        // bool setReadOffset(stream_size pOffset)
        // {
            // if(pOffset <= mEndOffset)
            // {
                // if(mStream.eof())
                    // mStream.clear(); // Clear eof flag
                // mStream.seekg(pOffset);
                // mReadOffset = mStream.tellg();
                // return true;
            // }
            // return false;
        // }
        // operator bool() const { return mStream.good() && (mReadOffset == (stream_size)-1 || mReadOffset < mEndOffset); }
        // bool operator !() const { return !mStream.good(); }
        // void read(void *pOutput, stream_size pSize)
        // {
            // mStream.read((char *)pOutput, pSize);
            // if(mReadOffset != INVALID_STREAM_SIZE)
            // {
                // mReadOffset += pSize;
                // if(mEndOffset < mReadOffset)
                    // mEndOffset = mReadOffset;
            // }
        // }

    // private:

        // // Setup offsets
        // void initialize()
        // {
            // if(mStream.tellp() == -1)
            // {
                // mWriteOffset = INVALID_STREAM_SIZE;
                // mEndOffset = INVALID_STREAM_SIZE;
                // return;
            // }

            // mReadOffset = 0;
            // mWriteOffset = mStream.tellp();
            // mStream.seekg(0, std::ios_base::end);
            // mEndOffset = mStream.tellg();
            // mStream.seekg(mReadOffset, std::ios_base::beg);
        // }

        // bool mValid;
        // std::fstream mStream;
        // stream_size mReadOffset, mWriteOffset, mEndOffset;
    // };
}

#endif
