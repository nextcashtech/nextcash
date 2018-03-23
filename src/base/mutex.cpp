/**************************************************************************
 * Copyright 2017 NextCash, LLC                                            *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@nextcash.com>                                    *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#include "mutex.hpp"

#ifdef PROFILER_ON
#include "profiler.hpp"
#endif

#include "log.hpp"

#include <unistd.h>


namespace NextCash
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
                if(mWriteLockName != NULL)
                    Log::addFormatted(Log::VERBOSE, "Mutex", "Waiting for read lock on %s (locked by %s)",
                      mName.text(), mWriteLockName);
                else
                    Log::addFormatted(Log::VERBOSE, "Mutex", "Waiting for read lock on %s", mName.text());
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
        int sleeps = 0;

        // Wait for the writers to unlock
        while(true)
        {
            mMutex.lock();
            if(!mWriterWaiting && !mWriterLocked)
            {
                mWriterWaiting = true;
                mMutex.unlock();
                break;
            }

            if(++sleeps > 500)
            {
                // It has been over 5 seconds. So notify that this wait is taking too long
                if(mWriterLocked)
                    Log::addFormatted(Log::VERBOSE, "Mutex", "Waiting for write lock for %s on %s (write locked by %s)",
                      pRequestName, mName.text(), mWriteLockName);
                else
                    Log::addFormatted(Log::VERBOSE, "Mutex", "Waiting for write lock for %s on %s (other writer waiting)",
                      pRequestName, mName.text());
                sleeps = 0;
            }

            // Wait for readers to unlock
            mMutex.unlock();
            usleep(10000); // 1/100th of a second
        }

        // Wait for the readers to unlock
        sleeps = 0;
        while(true)
        {
            mMutex.lock();
            if(mReaderCount == 0)
            {
                mWriterWaiting = false;
                mWriterLocked = true;
                mWriteLockName = pRequestName;
                mMutex.unlock();
                return;
            }

            if(++sleeps > 500)
            {
                // It has been over 5 seconds. So notify that this wait is taking too long
                if(pRequestName != NULL)
                    Log::addFormatted(Log::VERBOSE, "Mutex", "Waiting for write lock for %s on %s (%d readers locked)",
                      pRequestName, mName.text(), mReaderCount);
                else
                    Log::addFormatted(Log::VERBOSE, "Mutex", "Waiting for write lock on %s (%d readers locked)",
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
        mWriteLockName = NULL;
        mWriterLocked = false;
        mMutex.unlock();
    }
}
