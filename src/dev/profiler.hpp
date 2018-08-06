/**************************************************************************
 * Copyright 2017-2018 NextCash, LLC                                      *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@nextcash.tech>                                  *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#ifndef NEXTCASH_PROFILER_HPP
#define NEXTCASH_PROFILER_HPP

#include "string.hpp"
#include "stream.hpp"

#include <ctime>
#include <chrono>
#include <cstring>
#include <vector>


namespace NextCash
{
    class ProfilerData;

    class Profiler
    {
    public:

        // pStart means it will auto start when created.
        // It always auto stops when destroyed.
        Profiler(const char *pName, bool pStart = true);
        ~Profiler();

        void start();
        void stop();

        double seconds();

    private:
        ProfilerData *mData;
    };
}

#endif