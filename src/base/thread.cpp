/**************************************************************************
 * Copyright 2017-2018 NextCash, LLC                                      *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@nextcash.tech>                                  *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#include "thread.hpp"

#include "log.hpp"

#include <cstring>
#include <sstream>
#include <unistd.h>

#define THREAD_LOG_NAME "Thread"


namespace NextCash
{
    std::thread::id Thread::sMainThreadID = std::this_thread::get_id();
    std::mutex Thread::sThreadMutex;
    Thread::ID Thread::sNextThreadID = 2;
    std::map<std::thread::id, Thread::ID> Thread::sThreadIDs;
    std::map<Thread::ID, Thread::Data *> Thread::sThreads;

    Thread::Thread(const char *pName, void (*pFunction)(), void *pParameter)
    {
        sThreadMutex.lock();

        mName = pName;
        mID = sNextThreadID++;

        // Setup thread data
        sThreads.emplace(mID, new Data(mID, pName, pParameter));

        // Create thread
        mThread = new std::thread(pFunction);
        mInternalID = mThread->get_id();
        sThreadIDs[mInternalID] = mID;
        sThreads[mID]->internalID = mInternalID;

        if(sThreads.size() > 50)
            Log::addFormatted(Log::WARNING, THREAD_LOG_NAME, "There are %d active threads",
              sThreads.size());

        sThreadMutex.unlock();

        Log::addFormatted(Log::DEBUG, THREAD_LOG_NAME, "Started thread : %s", mName.text());
    }

    Thread::~Thread()
    {
        Log::addFormatted(Log::DEBUG, THREAD_LOG_NAME, "Stopping thread : %s", mName.text());
        mThread->join();

        sThreadMutex.lock();

        std::map<std::thread::id, ID>::iterator id = sThreadIDs.find(mInternalID);
        if(id == sThreadIDs.end())
            Log::addFormatted(Log::WARNING, THREAD_LOG_NAME,
              "Failed to find thread id to destroy : (0x%04x) %s", mID, mName.text());
        else
            sThreadIDs.erase(id);

        std::map<ID, Data *>::iterator data = sThreads.find(mID);
        if(data == sThreads.end())
            Log::addFormatted(Log::WARNING, THREAD_LOG_NAME,
              "Failed to find thread data to destroy : (0x%04x) %s", mID, mName.text());
        else
        {
            delete data->second;
            sThreads.erase(data);
        }

        sThreadMutex.unlock();
        delete mThread;
    }

    Thread::Data *Thread::currentData(bool pLocked)
    {
        std::thread::id currentID = std::this_thread::get_id();
        if(currentID == sMainThreadID)
            return NULL;

        if(!pLocked)
            sThreadMutex.lock();

        Thread::Data *result = NULL;
        std::map<std::thread::id, ID>::iterator id = sThreadIDs.find(currentID);
        if(id != sThreadIDs.end())
        {
            std::map<ID, Data *>::iterator data = sThreads.find(id->second);
            if(data != sThreads.end())
                result = data->second;
        }

        if(!pLocked)
            sThreadMutex.unlock();
        return result;
    }

    Thread::Data *Thread::getData(const ID &pID, bool pLocked)
    {
        if(!pLocked)
            sThreadMutex.lock();

        Thread::Data *result = NULL;
        std::map<ID, Data *>::iterator data = sThreads.find(pID);
        if(data != sThreads.end())
            result = data->second;

        if(!pLocked)
            sThreadMutex.unlock();
        return result;
    }

    const char *Thread::currentName(int pTimeoutMilliseconds)
    {
        std::thread::id currentID = std::this_thread::get_id();
        if(currentID == sMainThreadID)
            return "Main";

        sThreadMutex.lock();

        const char *result = NULL;
        Data *data = currentData(true);
        if(data != NULL)
            result = data->name.text();

        sThreadMutex.unlock();
        return result;
    }

    void *Thread::getParameter(int pTimeoutMilliseconds)
    {
        sThreadMutex.lock();

        void *result = NULL;
        Data *data = currentData(true);
        if(data != NULL && !data->parameterUsed)
        {
            result = data->parameter;
            data->parameterUsed = true;
        }

        sThreadMutex.unlock();
        return result;
    }

    Thread::ID Thread::currentID(int pTimeoutMilliseconds)
    {
        std::thread::id currentID = std::this_thread::get_id();
        if(currentID == sMainThreadID)
            return 1;

        sThreadMutex.lock();

        ID result = NullThreadID;
        std::map<std::thread::id, ID>::iterator id = sThreadIDs.find(currentID);
        if(id != sThreadIDs.end())
            result = id->second;

        sThreadMutex.unlock();
        return result;
    }

    String Thread::stringID(const ID &pID)
    {
        String result;
        if(pID == NullThreadID)
            result = "0xNULL";
        else
            result.writeFormatted("0x%04x", pID);
        return result;
    }

    const char *Thread::name(const ID &pID)
    {
        sThreadMutex.lock();

        const char *result = NULL;
        Data *data = getData(pID, true);
        if(data != NULL)
            result = data->name.text();

        sThreadMutex.unlock();
        return result;
    }

    void Thread::sleep(unsigned int pMilliseconds)
    {
        usleep(pMilliseconds * 1000);
    }
}
