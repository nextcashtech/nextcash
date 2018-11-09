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
    static std::vector<std::vector<Profiler *>> *sProfilerSets = NULL;

    void deleteProfiler()
    {
        delete sProfilerSets;
        sProfilerSets = NULL;
    }

    Profiler &getProfiler(unsigned int pSetID, unsigned int pID, const char *pName)
    {
        if(sProfilerSets == NULL)
        {
            sProfilerSets = new std::vector<std::vector<Profiler *>>();
            std::atexit(deleteProfiler);
        }

        if(sProfilerSets->size() <= pSetID)
            sProfilerSets->resize(pSetID + 1);

        std::vector<Profiler *> &set = sProfilerSets->at(pSetID);

        if(set.size() <= pID)
            set.resize(pID + 1);

        std::vector<Profiler *>::iterator result = set.begin() + pID;
        if(*result == NULL)
            *result = new Profiler(pName);
        return **result;
    }

    void resetProfilers()
    {
        if(sProfilerSets == NULL)
            return;

        for(std::vector<std::vector<Profiler *>>::iterator set = sProfilerSets->begin();
          set != sProfilerSets->end(); ++set)
            for(std::vector<Profiler *>::iterator item = set->begin(); item != set->end(); ++item)
                if(*item != NULL)
                    (*item)->clear();
    }

    void printProfilerDataToLog(Log::Level pLevel)
    {
        if(sProfilerSets == NULL)
            return;

        for(std::vector<std::vector<Profiler *>>::iterator set = sProfilerSets->begin();
          set != sProfilerSets->end(); ++set)
            for(std::vector<Profiler *>::iterator item = set->begin(); item != set->end(); ++item)
                if(*item != NULL)
                    Log::addFormatted(pLevel, "Profiler", "%s %llu hits, %llu ms",
                      (*item)->name().text(), (*item)->hits(), (*item)->milliseconds());
    }


}
