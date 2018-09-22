/**************************************************************************
 * Copyright 2017-2018 NextCash, LLC                                      *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@nextcash.tech>                                  *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#ifndef NEXTCASH_THREAD_HPP
#define NEXTCASH_THREAD_HPP

#include "string.hpp"

#include <thread>
#include <map>
#include <mutex>


namespace NextCash
{
    class Thread
    {
    public:

        // Name specified when the thread that is currently executing was created
        static const char *currentName(int pTimeoutMilliseconds = 1000);

        // Parameter specified when the thread that is currently executing was created. NULL if not specified or available
        // Removes the parameter before returning
        static void *getParameter(int pTimeoutMilliseconds = 1000);

        typedef unsigned int ID;

        static const ID NullThreadID = 0;

        // A text representation of the id of the currently active thread.
        static ID currentID(int pTimeoutMilliseconds = 1000);

        static String stringID(const ID &pID);
        static const char *name(const ID &pID);

        // Have this thread sleep for specified milliseconds
        static void sleep(unsigned int pMilliseconds);

        Thread(const char *pName, void (*pFunction)(), void *pParameter = NULL);
        ~Thread();

        const char *name() { return mName; }
        ID id() const { return mID; }

    private:

        String mName;
        std::thread *mThread;
        ID mID;
        std::thread::id mInternalID;

        class Data
        {
        public:

            ID id;
            std::thread::id internalID;
            String name;
            void *parameter;
            bool parameterUsed;
            bool started;

            Data(ID &pID, const char *pName, void *pParameter)
            {
                id = pID;
                name = pName;
                parameter = pParameter;
                parameterUsed = pParameter == NULL;
                started = false;
            }

            Data(const Data &pCopy)
            {
                id = pCopy.id;
                internalID = pCopy.internalID;
                name = pCopy.name;
                parameter = pCopy.parameter;
                parameterUsed = pCopy.parameterUsed;
                started = pCopy.started;
            }

            Data &operator = (const Data &pRight)
            {
                id = pRight.id;
                internalID = pRight.internalID;
                name = pRight.name;
                parameter = pRight.parameter;
                parameterUsed = pRight.parameterUsed;
                started = pRight.started;
                return *this;
            }

        };

        static Data *internalData(std::thread::id pInternalID, bool pLocked = false);
        static Data *currentData(bool pLocked = false);
        static Data *getData(const ID &pID, bool pLocked = false);

        static std::thread::id sMainThreadID;
        static std::mutex sThreadMutex;
        static ID sNextThreadID;
        static std::map<std::thread::id, ID> sThreadIDs;
        static std::map<ID, Data *> sThreads;

    };
}

#endif
