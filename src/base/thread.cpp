#include "thread.hpp"

#include "arcmist/base/log.hpp"

#include <cstring>
#include <unistd.h>

#define THREAD_LOG_NAME "Thread"


namespace ArcMist
{
    std::thread::id *Thread::mMainThreadID = NULL;
    std::map<std::thread::id, char *> Thread::mThreadNames;

    Thread::Thread(const char *pName, void (*pFunction)()) : mThread(pFunction)
    {
        if(mMainThreadID == NULL)
            mMainThreadID = new std::thread::id(std::this_thread::get_id());

        mName = new char[std::strlen(pName)+1];
        std::strcpy(mName, pName);
        mThreadNames[mThread.get_id()] = mName;
        Log::addFormatted(Log::VERBOSE, THREAD_LOG_NAME, "Starting thread : %s", mName);
    }

    Thread::~Thread()
    {
        Log::addFormatted(Log::VERBOSE, THREAD_LOG_NAME, "Stopping thread : %s", mName);
        mThread.join();
        std::map<std::thread::id, char *>::iterator pair = mThreadNames.find(mThread.get_id());
        if(pair != mThreadNames.end())
            mThreadNames.erase(pair);
        delete[] mName;
    }

    const char *Thread::currentName()
    {
        if(mMainThreadID == NULL)
            mMainThreadID = new std::thread::id(std::this_thread::get_id());

        std::thread::id currentID = std::this_thread::get_id();
        if(currentID == *mMainThreadID)
            return "Main";

        std::map<std::thread::id, char *>::iterator pair = mThreadNames.find(currentID);
        if(pair != mThreadNames.end())
            return pair->second;

        return "Unknown";
    }

    void Thread::sleep(unsigned int pMilliseconds)
    {
        usleep(pMilliseconds * 1000);
    }
}
