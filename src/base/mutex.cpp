#include "mutex.hpp"

#include "arcmist/base/log.hpp"

#include <unistd.h>


namespace ArcMist
{
    void Mutex::lock()
    {
        int sleeps = 0;
        while(!mMutex.try_lock())
        {
            usleep(100);
            if(++sleeps > 1000)
            {
                sleeps = 0;
                Log::addFormatted(Log::WARNING, "Mutex", "Waiting for lock on %s", mName);
            }
        }
    }

    void Mutex::unlock()
    {
        mMutex.unlock();
    }
}
