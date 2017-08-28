#ifndef ARCMIST_MUTEX_HPP
#define ARCMIST_MUTEX_HPP

#include <mutex>


namespace ArcMist
{
    class Mutex
    {
    public:

        Mutex(const char *pName)
        {
            for(int i=0;i<32;i++)
                mName[i] = 0;
            for(int i=0;pName[i];i++)
                mName[i] = pName[i];
        }
        ~Mutex() {}

        void lock();
        void unlock();

    private:

        std::mutex mMutex;
        char mName[32];

    };
}

#endif
