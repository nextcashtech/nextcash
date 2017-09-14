#include "mutex.hpp"

#include "arcmist/base/log.hpp"

#include <unistd.h>


namespace ArcMist
{
    void Mutex::lock()
    {
        int sleeps = 0;
        while(!mMutex.try_lock())
        {
            usleep(10000); // 1/100th of a second
            if(++sleeps > 100)
            {
                // It has been over a second. So notify that this wait is taking too long
                Log::addFormatted(Log::WARNING, "Mutex", "Waiting for lock on %s", mName.text());
                sleeps = 0;
            }
        }
    }

    void Mutex::unlock()
    {
        mMutex.unlock();
    }

    void ReadersLock::readLock()
    {
        int sleeps = 0;
        while(true)
        {
            mMutex.lock();
            if(!mWriterWaiting && !mWriterLocked)
            {
                ++mReaderCount;
                mMutex.unlock();
                return;
            }

            if(++sleeps > 100)
            {
                // It has been over a second. So notify that this wait is taking too long
                Log::addFormatted(Log::WARNING, "Mutex", "Waiting for read lock on %s", mName.text());
                sleeps = 0;
            }

            // Wait for writer to unlock
            mMutex.unlock();
            usleep(10000); // 1/100th of a second
        }
    }

    void ReadersLock::readUnlock()
    {
        mMutex.lock();
        --mReaderCount;
        mMutex.unlock();
    }

    void ReadersLock::writeLock(const char *pRequestName)
    {
        // Notify that writer lock is needed
        mMutex.lock();
        mWriterWaiting = true;
        mMutex.unlock();

        // Wait for the readers to unlock
        int sleeps = 0;
        while(true)
        {
            mMutex.lock();
            if(mReaderCount == 0)
            {
                mWriterWaiting = false;
                mWriterLocked = true;
                mMutex.unlock();
                return;
            }

            if(++sleeps > 500)
            {
                // It has been over a second. So notify that this wait is taking too long
                if(pRequestName != NULL)
                    Log::addFormatted(Log::WARNING, "Mutex", "Waiting for write lock for %s on %s (%d readers locked)",
                      pRequestName, mName.text(), mReaderCount);
                else
                    Log::addFormatted(Log::WARNING, "Mutex", "Waiting for write lock on %s (%d readers locked)",
                      mName.text(), mReaderCount);
                sleeps = 0;
            }

            // Wait for readers to unlock
            mMutex.unlock();
            usleep(10000); // 1/100th of a second
        }
    }

    void ReadersLock::writeUnlock()
    {
        mMutex.lock();
        mWriterLocked = false;
        mMutex.unlock();
    }
}
