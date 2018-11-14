/**************************************************************************
 * Copyright 2017 NextCash, LLC                                           *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@nextcash.tech>                                  *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#include "string.hpp"
#include "hash.hpp"
#include "hash_set.hpp"
#include "hash_container_list.hpp"
#include "hash_data_file_set.hpp"
#include "distributed_vector.hpp"
#include "log.hpp"
#include "thread.hpp"
#include "buffer.hpp"
#include "file_stream.hpp"
#include "digest.hpp"
#include "encrypt.hpp"
#include "profiler.hpp"


namespace NextCash
{
    bool test()
    {
        int failed = 0;

#ifndef ANDROID
        NextCash::Log::setLevel(NextCash::Log::DEBUG);
#endif

        if(!NextCash::String::test())
            failed++;

        if(!NextCash::Buffer::test())
            failed++;

        if(!NextCash::testDistributedVector())
            failed++;

        if(!NextCash::Hash::test())
            failed++;

        if(!NextCash::HashSet::test())
            failed++;

        if(!NextCash::testHashContainerList())
            failed++;

        if(!NextCash::testHashDataFileSet())
            failed++;

        if(!NextCash::Digest::test())
            failed++;

        if(!NextCash::Encryption::test())
            failed++;

        NextCash::Log::add(NextCash::Log::INFO, "Test", "------------- Starting General Tests -------------");

        Profiler &testProfiler = getProfiler(PROFILER_SET, 0, "TestProfiler");
        Timer timer(true);
        NextCash::Thread::sleep(100);
        timer.stop();
        testProfiler.addHit(timer.microseconds());
        NextCash::Log::addFormatted(NextCash::Log::INFO, "Test", "Profiler test : %d ms", testProfiler.milliseconds());
        NextCash::printProfilerDataToLog(NextCash::Log::INFO);

        NextCash::Log::addFormatted(Log::WARNING, "Test", "Current thread is %s %s",
          Thread::currentName(), Thread::stringID(Thread::currentID()).text());

        NextCash::Log::add(NextCash::Log::DEBUG, "Test", "Debug Color Test");
        NextCash::Log::add(NextCash::Log::VERBOSE, "Test", "Verbose Color Test");
        NextCash::Log::add(NextCash::Log::INFO, "Test", "Info Color Test");
        NextCash::Log::add(NextCash::Log::WARNING, "Test", "Warning Color Test");
        NextCash::Log::add(NextCash::Log::ERROR, "Test", "Error Color Test");
        NextCash::Log::add(NextCash::Log::NOTIFICATION, "Test", "Notification Color Test");
        NextCash::Log::add(NextCash::Log::CRITICAL, "Test", "Critical Color Test");

        // Note: See issues in file_stream.hpp with FileStream
        // // Write some data to a test file
        // NextCash::removeFile("test_file");
        // NextCash::FileStream *writeFile = new NextCash::FileStream("test_file");
        // writeFile->writeString("Test data");
        // writeFile->flush();
        // writeFile->setReadOffset(0);
        // NextCash::String testData = writeFile->readString(9);
        // if(testData != "Test data")
        // {
            // NextCash::Log::addFormatted(NextCash::Log::INFO, "Test", "Failed file stream initial read test : %s", testData.text());
            // failed = true;
        // }
        // else
            // NextCash::Log::add(NextCash::Log::INFO, "Test", "Passed file stream initial read test");
        // delete writeFile;

        // NextCash::FileStream *test1File = new NextCash::FileStream("test_file");
        // NextCash::String test1Data = test1File->readString(9);
        // if(test1Data != "Test data")
        // {
            // NextCash::Log::addFormatted(NextCash::Log::INFO, "Test", "Failed file stream test 1 : %s", test1Data.text());
            // failed = true;
        // }
        // else
            // NextCash::Log::add(NextCash::Log::INFO, "Test", "Passed file stream test 1");

        // test1File->setWriteOffset(0);
        // test1File->writeString("Test 2 Data");
        // delete test1File;

        // NextCash::FileStream *test2File = new NextCash::FileStream("test_file");
        // NextCash::String test2Data = test2File->readString(11);
        // if(test2Data != "Test 2 Data")
        // {
            // NextCash::Log::addFormatted(NextCash::Log::INFO, "Test", "Failed file stream test 2 : %s", test2Data.text());
            // failed = true;
        // }
        // else
            // NextCash::Log::add(NextCash::Log::INFO, "Test", "Passed file stream test 2");
        // delete test2File;
        // NextCash::removeFile("test_file");

        NextCash::Log::addFormatted(NextCash::Log::INFO, "Test", "Max stream size : 0x%08x%08x",
          NextCash::INVALID_STREAM_SIZE >> 32, NextCash::INVALID_STREAM_SIZE & 0xffffffff);

        return failed == 0;
    }
}

int main(int pArgumentCount, char **pArguments)
{
    if(NextCash::test())
        return 0;
    else
        return 1;
}
