/**************************************************************************
 * Copyright 2017 ArcMist, LLC                                            *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@arcmist.com>                                    *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#include "arcmist/base/string.hpp"
#include "arcmist/base/log.hpp"
#include "arcmist/base/thread.hpp"
#include "arcmist/io/buffer.hpp"
#include "arcmist/crypto/digest.hpp"
#include "arcmist/dev/profiler.hpp"


int main(int pArgumentCount, char **pArguments)
{
    int failed = 0;

    ArcMist::Log::setLevel(ArcMist::Log::DEBUG);

    if(!ArcMist::String::test())
        failed++;

    if(!ArcMist::Buffer::test())
        failed++;

    if(!ArcMist::Digest::test())
        failed++;

    ArcMist::Profiler testProfiler("Test");
    ArcMist::Thread::sleep(100);
    testProfiler.stop();
    ArcMist::Log::addFormatted(ArcMist::Log::DEBUG, "Test", "Profiler test : %f s", testProfiler.seconds());

    ArcMist::Log::add(ArcMist::Log::DEBUG, "Test", "Debug Color Test");
    ArcMist::Log::add(ArcMist::Log::VERBOSE, "Test", "Verbose Color Test");
    ArcMist::Log::add(ArcMist::Log::INFO, "Test", "Info Color Test");
    ArcMist::Log::add(ArcMist::Log::WARNING, "Test", "Warning Color Test");
    ArcMist::Log::add(ArcMist::Log::ERROR, "Test", "Error Color Test");
    ArcMist::Log::add(ArcMist::Log::NOTIFICATION, "Test", "Notification Color Test");
    ArcMist::Log::add(ArcMist::Log::CRITICAL, "Test", "Critical Color Test");

    if(failed)
        return 1;
    else
        return 0;
}
