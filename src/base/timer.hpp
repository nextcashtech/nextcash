/**************************************************************************
 * Copyright 2017-2018 NextCash, LLC                                      *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@nextcash.tech>                                  *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#ifndef NEXTCASH_TIMER_HPP
#define NEXTCASH_TIMER_HPP

#include <chrono>
#include <cstdint>


namespace NextCash
{
    typedef uint64_t Milliseconds;
    typedef uint64_t Microseconds;

    class Timer
    {
    public:

        Timer(bool pStart = false) : mHits(0), mMicroseconds(0L)
        {
            mStarted = false;
            if(pStart)
                start();
        }
        Timer(const Timer &pCopy)
        {
            mStartTime = pCopy.mStartTime;
            mStarted = pCopy.mStarted;
            mHits = pCopy.mHits;
            mMicroseconds = pCopy.mMicroseconds;
        }
        Timer &operator = (const Timer &pRight)
        {
            assign(pRight);
            return *this;
        }

        void assign(const Timer &pRight)
        {
            mStartTime = pRight.mStartTime;
            mStarted = pRight.mStarted;
            mHits = pRight.mHits;
            mMicroseconds = pRight.mMicroseconds;
        }

        void start()
        {
            mStartTime = std::chrono::steady_clock::now();
            mStarted = true;
        }

        void stop()
        {
            if(!mStarted)
                return;
            ++mHits;
            mMicroseconds += std::chrono::duration_cast<std::chrono::microseconds>(
              std::chrono::steady_clock::now() - mStartTime).count();
            mStarted = false;
        }

        void clear(bool pStart = false)
        {
            mStarted = false;
            mHits = 0;
            mMicroseconds = 0L;
            if(pStart)
                start();
        }

        uint64_t hits() const { return mHits; }
        Milliseconds milliseconds() const { return mMicroseconds / 1000L; }
        Microseconds microseconds() const { return mMicroseconds; }

    private:

        std::chrono::steady_clock::time_point mStartTime;
        bool mStarted;
        uint64_t mHits;
        Microseconds mMicroseconds;

    };
}

#endif
