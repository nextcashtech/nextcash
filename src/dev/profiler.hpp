#ifndef ARCMIST_PROFILER_HPP
#define ARCMIST_PROFILER_HPP

#include "arcmist/io/stream.hpp"

#include <ctime>
#include <cstring>
#include <vector>


namespace ArcMist
{
    class Profiler
    {
    public:

        Profiler(const char *pName) : hits(0), seconds(0.0f)
        {
            name = pName;
        }

        void start()
        {
            hits++;
            begin = clock();
        }
        void stop()
        {
            seconds += float(clock() - begin) / CLOCKS_PER_SEC;
        }
        void write(OutputStream *pStream);

        const char *name;
        std::clock_t begin;
        unsigned long hits;
        float seconds;
    };

    class ProfilerManager
    {
    public:

        static Profiler &profiler(const char *pName)
        {
            return manager().profilerLookup(pName);
        }

        static void write(OutputStream *pStream);

    private:

        static ProfilerManager *mInstance;
        static void destroy();

        ProfilerManager();
        ~ProfilerManager();

        Profiler &profilerLookup(const char *pName)
        {
            std::vector<Profiler *>::iterator iter = profilers.begin(), endIter = profilers.end();
            while(iter != endIter)
            {
                if(std::strcmp((*iter)->name, pName) == 0)
                    return **iter;
                iter++;
            }

            Profiler *newProfiler = new Profiler(pName);
            profilers.push_back(newProfiler);
            return *newProfiler;
        }

        static ProfilerManager &manager();

        std::vector<Profiler *> profilers;
    };
}

#endif