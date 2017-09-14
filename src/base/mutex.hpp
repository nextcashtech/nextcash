#ifndef ARCMIST_MUTEX_HPP
#define ARCMIST_MUTEX_HPP

#include "string.hpp"

#include <mutex>


namespace ArcMist
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
