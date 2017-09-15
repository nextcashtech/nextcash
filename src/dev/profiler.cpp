/**************************************************************************
 * Copyright 2017 ArcMist, LLC                                            *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@arcmist.com>                                    *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#include "profiler.hpp"

#include <cstdlib>


namespace ArcMist
{
    ProfilerManager *ProfilerManager::mInstance = 0;

    ProfilerManager &ProfilerManager::manager()
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
        std::vector<Profiler *>::iterator iter = profilers.begin(), endIter = profilers.end();
        while(iter != endIter)
        {
            delete *iter;
            iter++;
        }
    }

    void ProfilerManager::destroy()
    {
        if(mInstance)
            delete mInstance;
    }

    void Profiler::write(OutputStream *pStream)
    {
        pStream->writeFormatted("  %-32s %12d %9.6f %9.6f\n", name, hits, seconds, seconds / (float)hits);
    }

    void ProfilerManager::write(OutputStream *pStream)
    {
        ProfilerManager &theManager = manager();

        pStream->writeString("Profiler:\n");
        pStream->writeString("  Name                                     Hits      Time       Avg\n");

        std::vector<Profiler *>::iterator iter = theManager.profilers.begin(), endIter = theManager.profilers.end();
        while(iter != endIter)
        {
            (*iter)->write(pStream);
            iter++;
        }
    }
}
