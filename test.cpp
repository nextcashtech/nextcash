/**************************************************************************
 * Copyright 2017 ArcMist, LLC                                            *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@arcmist.com>                                    *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#include "arcmist/base/string.hpp"
#include "arcmist/base/distributed_vector.hpp"
#include "arcmist/base/log.hpp"
#include "arcmist/base/thread.hpp"
#include "arcmist/io/buffer.hpp"
#include "arcmist/io/file_stream.hpp"
#include "arcmist/crypto/digest.hpp"
#include "arcmist/dev/profiler.hpp"


int main(int pArgumentCount, char **pArguments)
{
    int failed = 0;

    ArcMist::Log::setLevel(ArcMist::Log::DEBUG);

    if(!ArcMist::String::test())
        failed++;

    if(!ArcMist::testDistributedVector())
        failed++;

    if(!ArcMist::Buffer::test())
        failed++;

    if(!ArcMist::Digest::test())
        failed++;

    ArcMist::Profiler testProfiler("Test");
    ArcMist::Thread::sleep(100);
    testProfiler.stop();
    ArcMist::Log::addFormatted(ArcMist::Log::INFO, "Test", "Profiler test : %f s", testProfiler.seconds());

    ArcMist::Log::add(ArcMist::Log::DEBUG, "Test", "Debug Color Test");
    ArcMist::Log::add(ArcMist::Log::VERBOSE, "Test", "Verbose Color Test");
    ArcMist::Log::add(ArcMist::Log::INFO, "Test", "Info Color Test");
    ArcMist::Log::add(ArcMist::Log::WARNING, "Test", "Warning Color Test");
    ArcMist::Log::add(ArcMist::Log::ERROR, "Test", "Error Color Test");
    ArcMist::Log::add(ArcMist::Log::NOTIFICATION, "Test", "Notification Color Test");
    ArcMist::Log::add(ArcMist::Log::CRITICAL, "Test", "Critical Color Test");

    // Note: See issues in file_stream.hpp with FileStream
    // // Write some data to a test file
    // ArcMist::removeFile("test_file");
    // ArcMist::FileStream *writeFile = new ArcMist::FileStream("test_file");
    // writeFile->writeString("Test data");
    // writeFile->flush();
    // writeFile->setReadOffset(0);
    // ArcMist::String testData = writeFile->readString(9);
    // if(testData != "Test data")
    // {
        // ArcMist::Log::addFormatted(ArcMist::Log::INFO, "Test", "Failed file stream initial read test : %s", testData.text());
        // failed = true;
    // }
    // else
        // ArcMist::Log::add(ArcMist::Log::INFO, "Test", "Passed file stream initial read test");
    // delete writeFile;

    // ArcMist::FileStream *test1File = new ArcMist::FileStream("test_file");
    // ArcMist::String test1Data = test1File->readString(9);
    // if(test1Data != "Test data")
    // {
        // ArcMist::Log::addFormatted(ArcMist::Log::INFO, "Test", "Failed file stream test 1 : %s", test1Data.text());
        // failed = true;
    // }
    // else
        // ArcMist::Log::add(ArcMist::Log::INFO, "Test", "Passed file stream test 1");

    // test1File->setWriteOffset(0);
    // test1File->writeString("Test 2 Data");
    // delete test1File;

    // ArcMist::FileStream *test2File = new ArcMist::FileStream("test_file");
    // ArcMist::String test2Data = test2File->readString(11);
    // if(test2Data != "Test 2 Data")
    // {
        // ArcMist::Log::addFormatted(ArcMist::Log::INFO, "Test", "Failed file stream test 2 : %s", test2Data.text());
        // failed = true;
    // }
    // else
        // ArcMist::Log::add(ArcMist::Log::INFO, "Test", "Passed file stream test 2");
    // delete test2File;
    // ArcMist::removeFile("test_file");

    ArcMist::Log::addFormatted(ArcMist::Log::INFO, "Test", "Max stream size : 0x%08x%08x",
      ArcMist::INVALID_STREAM_SIZE >> 32, ArcMist::INVALID_STREAM_SIZE & 0xffffffff);

    if(failed)
        return 1;
    else
        return 0;
}
