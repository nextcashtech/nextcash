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
        ID newID = sNextThreadID++;

        // Setup thread data
        sThreads.emplace(newID, new Data(newID, pName, pParameter));

        // Create thread
        mThread = new std::thread(pFunction);
        std::thread::id internalID = mThread->get_id();
        sThreadIDs[internalID] = newID;
        sThreads[newID]->internalID = internalID;

        sThreadMutex.unlock();

        Log::addFormatted(Log::DEBUG, THREAD_LOG_NAME, "Started thread : %s", mName.text());
    }

    Thread::~Thread()
    {
        Log::addFormatted(Log::DEBUG, THREAD_LOG_NAME, "Stopping thread : %s", mName.text());
        mThread->join();

        sThreadMutex.lock();

        std::map<std::thread::id, ID>::iterator id = sThreadIDs.find(mThread->get_id());
        if(id != sThreadIDs.end())
        {
            std::map<ID, Data *>::iterator data = sThreads.find(id->second);
            if(data != sThreads.end())
            {
                delete data->second;
                sThreads.erase(data);
            }
            sThreadIDs.erase(id);
        }

        sThreadMutex.unlock();

        delete mThread;
    }

    Thread::Data *Thread::currentData()
    {
        std::thread::id currentID = std::this_thread::get_id();
        if(currentID == sMainThreadID)
            return NULL;

        std::map<std::thread::id, ID>::iterator id = sThreadIDs.find(currentID);
        if(id != sThreadIDs.end())
        {
            std::map<ID, Data *>::iterator data = sThreads.find(id->second);
            if(data != sThreads.end())
                return data->second;
        }

        return NULL;
    }

    Thread::Data *Thread::getData(const ID &pID)
    {
        std::map<ID, Data *>::iterator data = sThreads.find(pID);
        if(data != sThreads.end())
            return data->second;
        else
            return NULL;
    }

    const char *Thread::currentName(int pTimeoutMilliseconds)
    {
        std::thread::id currentID = std::this_thread::get_id();
        if(currentID == sMainThreadID)
            return "Main";

        const char *result = NULL;

        sThreadMutex.lock();

        Data *data = currentData();
        if(data != NULL)
            result = data->name.text();

        sThreadMutex.unlock();
        return result;
    }

    void *Thread::getParameter(int pTimeoutMilliseconds)
    {
        void *result = NULL;

        sThreadMutex.lock();

        Data *data = currentData();
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
        {
            sThreadMutex.unlock();
            return 1;
        }

        ID result = NullThreadID;

        sThreadMutex.lock();

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
        const char *result = NULL;

        sThreadMutex.lock();

        Data *data = getData(pID);
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
