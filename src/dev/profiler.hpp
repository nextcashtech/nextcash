/**************************************************************************
 * Copyright 2017-2018 NextCash, LLC                                      *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@nextcash.tech>                                  *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#ifndef NEXTCASH_PROFILER_HPP
#define NEXTCASH_PROFILER_HPP

#include <chrono>
#include <cstdint>

#include "string.hpp"
#include "log.hpp"


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

#ifdef PROFILER_ON
    static const unsigned int PROFILER_SET = 0;

    static const unsigned int PROFILER_HASH_SET_SUB_SAVE_ID = 0;
    static const char *PROFILER_HASH_SET_SUB_SAVE_NAME __attribute__ ((unused)) = "HashDataSet::SubSet::save";
    static const unsigned int PROFILER_HASH_SET_INSERT_ID = 1;
    static const char *PROFILER_HASH_SET_INSERT_NAME __attribute__ ((unused)) = "HashDataSet::insert";
    static const unsigned int PROFILER_HASH_SET_PULL_ID = 2;
    static const char *PROFILER_HASH_SET_PULL_NAME __attribute__ ((unused)) = "HashDataSet::SubSet::pull";

    static const unsigned int PROFILER_HASH_CONT_FIND_ID = 3;
    static const char *PROFILER_HASH_CONT_FIND_NAME __attribute__ ((unused)) = "HashContainer::findInsertBefore";
    static const unsigned int PROFILER_HASH_CONT_INSERT_ID = 4;
    static const char *PROFILER_HASH_CONT_INSERT_NAME __attribute__ ((unused)) = "HashContainer::insert";
    static const unsigned int PROFILER_HASH_CONT_INSERT_NM_ID = 5;
    static const char *PROFILER_HASH_CONT_INSERT_NM_NAME __attribute__ ((unused)) = "HashContainer::insertNotMatching";
#endif

    class Profiler : public Timer
    {
    public:

        Profiler() : Timer() {}
        Profiler(const Profiler &pCopy) : Timer(pCopy) { mName = pCopy.mName; }

        Profiler &operator = (const Profiler &pRight)
        {
            mName = pRight.mName;
            Timer::assign(pRight);
            return *this;
        }

        const String &name() const { return mName; }
        void setName(const char *pName) { mName = pName; }

    private:

        String mName;

    };

    // Profiler that stops when it goes out of scope.
    class ProfilerReference
    {
    public:
        ProfilerReference(Profiler &pProfiler) : mProfiler(pProfiler) { }

        ~ProfilerReference() { mProfiler.stop(); }

        Profiler &mProfiler;
    };

    Profiler &getProfiler(unsigned int pSetID, unsigned int pID, const char *pName,
      bool pStart = false);

    void printProfilerDataToLog(Log::Level pLevel);
}

#endif
