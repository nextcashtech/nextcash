/**************************************************************************
 * Copyright 2017 NextCash, LLC                                           *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@nextcash.tech>                                  *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#include "profiler.hpp"

#include <cstdlib>


namespace NextCash
{
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
        for(std::vector<ProfilerData *>::iterator iter = mProfilers.begin();iter!=mProfilers.end();++iter)
            delete *iter;
    }

    void ProfilerManager::destroy()
    {
        if(mInstance)
            delete mInstance;
    }

    void ProfilerData::write(OutputStream *pStream)
    {
        //float seconds = ((float)time) / CLOCKS_PER_SEC;
        pStream->writeFormatted("  %-40s %12d %14.6f %9.6f\n", name.text(), hits, seconds, seconds / (float)hits);
    }

    void ProfilerManager::write(OutputStream *pStream)
    {
        pStream->writeString("Profiler:\n");
        pStream->writeString("  Name                                             Hits           Time       Avg\n");

        std::vector<ProfilerData *> &profilers = instance().mProfilers;
        for(std::vector<ProfilerData *>::iterator iter = profilers.begin();iter!=profilers.end();++iter)
            (*iter)->write(pStream);

        pStream->flush();
    }

    void ProfilerManager::reset()
    {
        std::vector<ProfilerData *> &profilers = instance().mProfilers;
        for(std::vector<ProfilerData *>::iterator iter = profilers.begin();iter!=profilers.end();++iter)
            delete *iter;
        instance().mProfilers.clear();
    }
}
