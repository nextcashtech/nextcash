/**************************************************************************
 * Copyright 2017 NextCash, LLC                                           *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@nextcash.tech>                                  *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#ifndef NEXTCASH_MUTEX_HPP
#define NEXTCASH_MUTEX_HPP

#include "string.hpp"

#include <mutex>


namespace NextCash
{
    class Mutex
    {
    public:

        Mutex(const char *pName) { mName = pName; }

        void lock();
        void unlock();

    private:

        std::mutex mMutex;
        String mName;

    };

    class MutexWithConstantName
    {
    public:

        MutexWithConstantName(const char *pName) { mName = pName; }

        void lock();
        void unlock();

    private:

        std::mutex mMutex;
        const char *mName;

    };

    // Lock that allows multiple readers to lock concurrently, but a writer needs exclusive access.
    class ReadersLock
    {
    public:

        ReadersLock(const char *pName)
        {
            mName = pName;
            mReaderCount = 0;
            mWriterWaiting = false;
            mWriterLocked = false;
            mWriteLockName = NULL;
        }

        void readLock();
        void readUnlock();
        void writeLock(const char *pRequestName = NULL);
        void writeUnlock();

    private:

        std::mutex mMutex;
        String mName;
        const char *mWriteLockName;
        unsigned int mReaderCount;
        bool mWriterWaiting;
        bool mWriterLocked;

    };
}

#endif
