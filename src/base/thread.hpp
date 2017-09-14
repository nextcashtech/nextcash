#ifndef ARCMIST_THREAD_HPP
#define ARCMIST_THREAD_HPP

#include "mutex.hpp"
#include "string.hpp"

#include <thread>
#include <map>


namespace ArcMist
{
    class Thread
    {
    public:

        // Name specified when the thread that is currently executing was created
        static const char *currentName(int pTimeoutMilliseconds = 1000);
        // Parameter specified when the thread that is currently executing was created. NULL if not specified or available
        // Removes the parameter before returning
        static void *getParameter(int pTimeoutMilliseconds = 1000);
        // Have this thread sleep for specified milliseconds
        static void sleep(unsigned int pMilliseconds);

        Thread(const char *pName, void (*pFunction)(), void *pParameter = NULL);
        ~Thread();

        const char *name() { return mName; }

    private:

        String mName;
        std::thread mThread;

        static std::thread::id *sMainThreadID;
        static Mutex sThreadMutex;
        static std::map<std::thread::id, String> sThreadNames;
        static std::map<std::thread::id, void *> sThreadParameters;

    };
}

#endif
