#include "thread.hpp"

#include "arcmist/base/log.hpp"

#include <cstring>
#include <unistd.h>

#define THREAD_LOG_NAME "Thread"


namespace ArcMist
{
    std::thread::id *Thread::sMainThreadID = NULL;
    std::map<std::thread::id, String> Thread::sThreadNames;
    std::map<std::thread::id, void *> Thread::sThreadParameters;

    Thread::Thread(const char *pName, void (*pFunction)(), void *pParameter) : mThread(pFunction)
    {
        if(sMainThreadID == NULL)
            sMainThreadID = new std::thread::id(std::this_thread::get_id());

        mName = pName;
        sThreadNames[mThread.get_id()] = pName;
        sThreadParameters[mThread.get_id()] = pParameter;
        Log::addFormatted(Log::VERBOSE, THREAD_LOG_NAME, "Starting thread : %s", mName.text());
    }

    Thread::~Thread()
    {
        Log::addFormatted(Log::VERBOSE, THREAD_LOG_NAME, "Stopping thread : %s", mName.text());
        mThread.join();
        std::map<std::thread::id, String>::iterator name = sThreadNames.find(mThread.get_id());
        if(name != sThreadNames.end())
            sThreadNames.erase(name);
        std::map<std::thread::id, void *>::iterator parameter = sThreadParameters.find(mThread.get_id());
        if(parameter != sThreadParameters.end())
            sThreadParameters.erase(parameter);
    }

    const char *Thread::currentName(int pTimeoutMilliseconds)
    {
        if(sMainThreadID == NULL)
            sMainThreadID = new std::thread::id(std::this_thread::get_id());

        std::thread::id currentID = std::this_thread::get_id();
        if(currentID == *sMainThreadID)
            return "Main";

        std::map<std::thread::id, String>::iterator name = sThreadNames.find(currentID);
        // Wait for it to be set
        while(name == sThreadNames.end() && pTimeoutMilliseconds > 0)
        {
            name = sThreadNames.find(currentID);
            sleep(100);
            pTimeoutMilliseconds -= 100;
        }
        if(name != sThreadNames.end())
            return name->second.text();
        return "Unknown";
    }

    void *Thread::currentParameter(int pTimeoutMilliseconds)
    {
        if(sMainThreadID == NULL)
            sMainThreadID = new std::thread::id(std::this_thread::get_id());

        std::thread::id currentID = std::this_thread::get_id();
        if(currentID == *sMainThreadID)
            return NULL;

        std::map<std::thread::id, void *>::iterator parameter = sThreadParameters.find(currentID);
        // Wait for it to be set
        while(parameter == sThreadParameters.end() && pTimeoutMilliseconds > 0)
        {
            parameter = sThreadParameters.find(currentID);
            sleep(100);
            pTimeoutMilliseconds -= 100;
        }
        if(parameter != sThreadParameters.end())
            return parameter->second;
        return NULL;
    }

    void Thread::sleep(unsigned int pMilliseconds)
    {
        usleep(pMilliseconds * 1000);
    }
}
