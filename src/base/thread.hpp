#ifndef ARCMIST_THREAD_HPP
#define ARCMIST_THREAD_HPP

#include <thread>
#include <map>


namespace ArcMist
{
    class Thread
    {
    public:

        // Current thread name
        static const char *currentName();
        static void sleep(unsigned int pMilliseconds);

        Thread(const char *pName, void (*pFunction)());
        ~Thread();

        const char *name() { return mName; }

    private:

        char *mName;
        std::thread mThread;

        static std::thread::id *mMainThreadID;
        static std::map<std::thread::id, char *> mThreadNames;

    };
}

#endif
