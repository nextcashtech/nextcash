/**************************************************************************
 * Copyright 2017-2018 NextCash, LLC                                      *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@nextcash.tech>                                  *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#include "profiler.hpp"

#include <cstdlib>


namespace NextCash
{
    class ProfilerData
    {
    public:

        ProfilerData(const char *pName) : hits(0), seconds(0.0)
          { name = pName; started = false; }

        void start() { hits++; begin = std::chrono::steady_clock::now(); started = true; }
        void stop()
        {
            if(!started)
                return;
            seconds += std::chrono::duration_cast<std::chrono::duration<double>>(
              std::chrono::steady_clock::now() - begin).count();
            started = false;
        }
        void write(OutputStream *pStream);

        String name;
        std::chrono::steady_clock::time_point begin;
        unsigned long hits;
        bool started;
        double seconds;

    private:

        ProfilerData(ProfilerData &pCopy);
        ProfilerData &operator = (const ProfilerData &pRight);

    };

    class ProfilerManager
    {
    public:

        static ProfilerData *profilerData(const char *pName)
        {
            std::vector<ProfilerData *> &profilers = instance().mProfilers;
            for(std::vector<ProfilerData *>::iterator iter = profilers.begin();
              iter != profilers.end(); ++iter)
                if((*iter)->name == pName)
                    return *iter;

            ProfilerData *newProfiler = new ProfilerData(pName);
            profilers.push_back(newProfiler);
            return newProfiler;
        }

        // Write text results that were collected by profiler
        static void write(OutputStream *pStream);

        static void reset();

    private:

        static ProfilerManager *mInstance;
        static void destroy();

        ProfilerManager();
        ~ProfilerManager();

        static ProfilerManager &instance();

        std::vector<ProfilerData *> mProfilers;

    };

    ProfilerManager *ProfilerManager::mInstance = 0;

    ProfilerManager &ProfilerManager::instance()
    {
        if(!mInstance)
        {
            mInstance = new ProfilerManager();
            std::atexit(ProfilerManager::destroy);
        }

        return *mInstance;
    }

    ProfilerManager::ProfilerManager()
    {

    }

    ProfilerManager::~ProfilerManager()
    {
        for(std::vector<ProfilerData *>::iterator iter = mProfilers.begin();
          iter != mProfilers.end(); ++iter)
            delete *iter;
    }

    void ProfilerManager::destroy()
    {
        if(mInstance)
            delete mInstance;
    }

    void ProfilerData::write(OutputStream *pStream)
    {
        pStream->writeFormatted("  %-40s %12d %14.6f %9.6f\n", name.text(), hits, seconds,
          seconds / (float)hits);
    }

    void ProfilerManager::write(OutputStream *pStream)
    {
        pStream->writeString("Profiler:\n");
        pStream->writeString(
          "  Name                                             Hits           Time       Avg\n");

        std::vector<ProfilerData *> &profilers = instance().mProfilers;
        for(std::vector<ProfilerData *>::iterator iter = profilers.begin();
          iter != profilers.end(); ++iter)
            (*iter)->write(pStream);

        pStream->flush();
    }

    void ProfilerManager::reset()
    {
        std::vector<ProfilerData *> &profilers = instance().mProfilers;
        for(std::vector<ProfilerData *>::iterator iter = profilers.begin();
          iter != profilers.end(); ++iter)
            delete *iter;
        profilers.clear();
    }

    Profiler::Profiler(const char *pName, bool pStart)
    {
        mData = ProfilerManager::profilerData(pName);
        if(pStart)
            mData->start();
    }
    Profiler::~Profiler() { mData->stop(); }
    void Profiler::start() { mData->start(); }
    void Profiler::stop() { mData->stop(); }
    double Profiler::seconds() { return mData->seconds; }
}
