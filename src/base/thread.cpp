/**************************************************************************
 * Copyright 2017 ArcMist, LLC                                            *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@arcmist.com>                                    *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#include "thread.hpp"

#include "arcmist/base/log.hpp"

#include <cstring>
#include <unistd.h>

#define THREAD_LOG_NAME "Thread"


namespace ArcMist
{
    std::thread::id *Thread::sMainThreadID = NULL;
    Mutex Thread::sThreadMutex("Thread");
    std::map<std::thread::id, String> Thread::sThreadNames;
    std::map<std::thread::id, void *> Thread::sThreadParameters;

    Thread::Thread(const char *pName, void (*pFunction)(), void *pParameter) : mThread(pFunction)
    {
        if(sMainThreadID == NULL)
            sMainThreadID = new std::thread::id(std::this_thread::get_id());

        mName = pName;
        sThreadMutex.lock();
        sThreadNames[mThread.get_id()] = pName;
        if(pParameter != NULL)
            sThreadParameters[mThread.get_id()] = pParameter;
        sThreadMutex.unlock();
        Log::addFormatted(Log::VERBOSE, THREAD_LOG_NAME, "Started thread : %s", mName.text());
    }

    Thread::~Thread()
    {
        sThreadMutex.lock();
        std::map<std::thread::id, String>::iterator name = sThreadNames.find(mThread.get_id());
        if(name != sThreadNames.end())
            sThreadNames.erase(name);
        sThreadMutex.unlock();
        Log::addFormatted(Log::VERBOSE, THREAD_LOG_NAME, "Stopping thread : %s", mName.text());
        mThread.join();
    }

    const char *Thread::currentName(int pTimeoutMilliseconds)
    {
        if(sMainThreadID == NULL)
            sMainThreadID = new std::thread::id(std::this_thread::get_id());

        std::thread::id currentID = std::this_thread::get_id();
        if(currentID == *sMainThreadID)
            return "Main";

        sThreadMutex.lock();
        bool found = sThreadNames.find(currentID) != sThreadNames.end();
        sThreadMutex.unlock();
        const char *result = "Unknown";

        // Wait for it to be available
        while(!found && pTimeoutMilliseconds > 0)
        {
            sleep(100);
            pTimeoutMilliseconds -= 100;
            sThreadMutex.lock();
            found = sThreadNames.find(currentID) != sThreadNames.end();
            sThreadMutex.unlock();
        }

        sThreadMutex.lock();
        std::map<std::thread::id, String>::iterator name = sThreadNames.find(currentID);
        if(name != sThreadNames.end())
            result = name->second.text();
        sThreadMutex.unlock();
        return result;
    }

    void *Thread::getParameter(int pTimeoutMilliseconds)
    {
        if(sMainThreadID == NULL)
            sMainThreadID = new std::thread::id(std::this_thread::get_id());

        std::thread::id currentID = std::this_thread::get_id();
        if(currentID == *sMainThreadID)
            return NULL;

        sThreadMutex.lock();
        bool found = sThreadParameters.find(currentID) != sThreadParameters.end();
        sThreadMutex.unlock();
        void *result = NULL;

        // Wait for it to be available
        while(!found && pTimeoutMilliseconds > 0)
        {
            sleep(100);
            pTimeoutMilliseconds -= 100;
            sThreadMutex.lock();
            found = sThreadParameters.find(currentID) != sThreadParameters.end();
            sThreadMutex.unlock();
        }

        sThreadMutex.lock();
        std::map<std::thread::id, void *>::iterator parameter = sThreadParameters.find(currentID);
        if(parameter != sThreadParameters.end())
        {
            result = parameter->second;
            sThreadParameters.erase(parameter);
        }
        sThreadMutex.unlock();
        return result;
    }

    void Thread::sleep(unsigned int pMilliseconds)
    {
        usleep(pMilliseconds * 1000);
    }
}
