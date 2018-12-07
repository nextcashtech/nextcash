/**************************************************************************
 * Copyright 2017 NextCash, LLC                                           *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@nextcash.tech>                                  *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#ifndef NEXTCASH_REFERENCE_COUNTER_HPP
#define NEXTCASH_REFERENCE_COUNTER_HPP

#include "mutex.hpp"


namespace NextCash
{
    template <class tType>
    class ReferenceCounter
    {
    public:

        ReferenceCounter() { mData = new Data(NULL); }
        ReferenceCounter(tType *pObject) { mData = new Data(pObject); }
        ReferenceCounter(const ReferenceCounter &pCopy)
        {
            mData = pCopy.mData;

            mData->mutex.lock();
            ++mData->count;
            mData->mutex.unlock();
        }
        ~ReferenceCounter()
        {
            mData->mutex.lock();
            if(--mData->count == 0)
                delete mData;
            else
                mData->mutex.unlock();
        }

        ReferenceCounter &operator = (const ReferenceCounter &pRight)
        {
            mData->mutex.lock();
            if(--mData->count == 0)
                delete mData;
            else
                mData->mutex.unlock();

            mData = pRight.mData;

            mData->mutex.lock();
            ++mData->count;
            mData->mutex.unlock();

            return *this;
        }

        void operator = (tType *pObject)
        {
            mData->mutex.lock();
            if(mData->count == 1)
            {
                // Replace object
                if(mData->object != NULL)
                    delete mData->object;
                mData->object = pObject;
                mData->mutex.unlock();
            }
            else
            {
                --mData->count;
                mData->mutex.unlock();
                mData = new Data(pObject);
            }
        }

        void clear() { *this = NULL; }

        bool operator !() const { return mData->object == NULL; }
        operator bool() const { return mData->object != NULL; }

        tType &operator *() { return *mData->object; }
        tType *operator ->() { return mData->object; }
        const tType &operator *() const { return *mData->object; }
        const tType *operator ->() const { return mData->object; }

        tType *pointer() { return mData->object; }

    private:

        class Data
        {
        public:
            Data(tType *pObject) : mutex("RefCounter")
            {
                object = pObject;
                count = 1;
            }
            ~Data() { if(object != NULL) delete object; }

            MutexWithConstantName mutex;
            tType *object;
            unsigned int count;
        };

        Data *mData;

    };
}

#endif
