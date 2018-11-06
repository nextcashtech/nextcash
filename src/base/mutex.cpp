/**************************************************************************
 * Copyright 2017-2018 NextCash, LLC                                      *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@nextcash.tech>                                  *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#include "mutex.hpp"

#ifdef PROFILER_ON
#include "profiler.hpp"
#endif

#include "log.hpp"
#include "thread.hpp"

#include <unistd.h>

#define NEXTCASH_MUTEX_LOG_NAME "Mutex"
// 1/200th of a second
#define MUTEX_WAIT 5000


namespace NextCash
{
    void Mutex::lock()
    {
        int sleeps = 0;
        while(!mMutex.try_lock())
        {
            usleep(MUTEX_WAIT);
            if(++sleeps > 200)
            {
                // It has been over a second. So notify that this wait is taking too long
                Log::addFormatted(Log::WARNING, NEXTCASH_MUTEX_LOG_NAME,
                  "Waiting for lock on %s (Locked by thread %s %s)", mName.text(),
                  Thread::name(mLockedThread), Thread::stringID(mLockedThread).text());
                sleeps = 0;
            }
        }

        mLockedThread = Thread::currentID();
    }

    void Mutex::unlock()
    {
        mLockedThread = Thread::NullThreadID;
        mMutex.unlock();
    }

    void MutexWithConstantName::lock()
    {
        int sleeps = 0;
        while(!mMutex.try_lock())
        {
            usleep(MUTEX_WAIT);
            if(++sleeps > 200)
            {
                // It has been over a second. So notify that this wait is taking too long
                Log::addFormatted(Log::WARNING, NEXTCASH_MUTEX_LOG_NAME,
                  "Waiting for lock on %s", mName);
                sleeps = 0;
            }
        }
    }

    void MutexWithConstantName::unlock()
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
                    Log::addFormatted(Log::VERBOSE, NEXTCASH_MUTEX_LOG_NAME,
                      "Waiting for read lock on %s (locked by %s, thread %s %s)",
                      mName.text(), mWriteLockName, Thread::name(mWriteLockedThread),
                      Thread::stringID(mWriteLockedThread).text());
                else
                    Log::addFormatted(Log::VERBOSE, NEXTCASH_MUTEX_LOG_NAME,
                      "Waiting for read lock on %s", mName.text());
                sleeps = 0;
            }

            // Wait for writer to unlock
            mMutex.unlock();
            usleep(MUTEX_WAIT);
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

            if(++sleeps > 1000)
            {
                // It has been over 5 seconds. So notify that this wait is taking too long
                if(mWriterLocked)
                    Log::addFormatted(Log::VERBOSE, NEXTCASH_MUTEX_LOG_NAME,
                      "Waiting for write lock for %s on %s (write locked by %s, thread %s %s)",
                      pRequestName, mName.text(), mWriteLockName,
                      Thread::name(mWriteLockedThread),
                      Thread::stringID(mWriteLockedThread).text());
                else
                    Log::addFormatted(Log::VERBOSE, NEXTCASH_MUTEX_LOG_NAME,
                      "Waiting for write lock for %s on %s (other writer waiting)",
                      pRequestName, mName.text());
                sleeps = 0;
            }

            // Wait for readers to unlock
            mMutex.unlock();
            usleep(MUTEX_WAIT);
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
                mWriteLockedThread = Thread::currentID();
                mMutex.unlock();
                return;
            }

            if(++sleeps > 1000)
            {
                // It has been over 5 seconds. So notify that this wait is taking too long
                if(pRequestName != NULL)
                    Log::addFormatted(Log::VERBOSE, NEXTCASH_MUTEX_LOG_NAME,
                      "Waiting for write lock for %s on %s (%d readers locked)",
                      pRequestName, mName.text(), mReaderCount);
                else
                    Log::addFormatted(Log::VERBOSE, NEXTCASH_MUTEX_LOG_NAME,
                      "Waiting for write lock on %s (%d readers locked)",
                      mName.text(), mReaderCount);
                sleeps = 0;
            }

            // Wait for readers to unlock
            mMutex.unlock();
            usleep(MUTEX_WAIT);
        }
    }

    void ReadersLock::writeUnlock()
    {
        mMutex.lock();
        mWriteLockName = NULL;
        mWriterLocked = false;
        mWriteLockedThread = Thread::NullThreadID;
        mMutex.unlock();
    }
}
