/**************************************************************************
 * Copyright 2017-2018 NextCash, LLC                                      *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@nextcash.tech>                                  *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#include "profiler.hpp"


namespace NextCash
{
    static std::vector<std::vector<Profiler>> sProfilerSets;

    Profiler &getProfiler(unsigned int pSetID, unsigned int pID, const char *pName, bool pStart)
    {
        if(sProfilerSets.size() <= pSetID)
            sProfilerSets.resize(pSetID + 1);

        std::vector<Profiler> &set = sProfilerSets.at(pSetID);

        if(set.size() <= pID)
            set.resize(pID + 1);

        Profiler &result = set.at(pID);
        if(!result.name())
            result.setName(pName);
        if(pStart)
            result.start();
        return result;
    }

    void printProfilerDataToLog(Log::Level pLevel)
    {
        for(std::vector<std::vector<Profiler>>::iterator set = sProfilerSets.begin();
          set != sProfilerSets.end(); ++set)
            for(std::vector<Profiler>::iterator item = set->begin(); item != set->end(); ++item)
                if(item->name())
                    Log::addFormatted(pLevel, "Profiler", "%s %llu hits, %llu ms", item->name().text(),
                      item->hits(), item->milliseconds());
    }
}
