/**************************************************************************
 * Copyright 2017 ArcMist, LLC                                            *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@arcmist.com>                                    *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#ifndef ARCMIST_DISTRIBUTED_VECTOR_HPP
#define ARCMIST_DISTRIBUTED_VECTOR_HPP

#include <cstring>
#include <vector>


namespace ArcMist
{
    // This class reduces insert times on large data sets because an insert doesn't require moving
    //   all objects after the insert position, only those in the subset
    // It does increase time for iteration, moving iterators up and down the data set, because it
    //   has to traverse the multiple sets. Especially finding an iterator at an arbitrary offset.
    template <class tType>
    class DistributedVector
    {
    public:

        DistributedVector(unsigned int pSetCount);
        ~DistributedVector();

        unsigned int size() const { return mSize; }

        void clear();

        // Reserve total capacity. Capacity will be distributed amoung sets
        void reserve(unsigned int pCount);

        typedef typename std::vector<tType>::iterator set_iterator;

        class iterator
        {
        public:
            iterator()
            {
                enclosing = NULL;
                set = NULL;
            }
            iterator(const iterator &pCopy)
            {
                enclosing = pCopy.enclosing;
                set = pCopy.set;
                item = pCopy.item;
            }
            iterator(DistributedVector *pEnclosing, std::vector<tType> *pSet, set_iterator pItem)
            {
                enclosing = pEnclosing;
                set = pSet;
                item = pItem;
            }

            tType &operator *() { return *item; }
            tType *operator ->() { return &(*item); }

            bool operator ==(const iterator &pRight)
            {
                return set == pRight.set && item == pRight.item;
            }

            bool operator !=(const iterator &pRight)
            {
                return set != pRight.set || item != pRight.item;
            }

            iterator &operator =(const iterator &pRight)
            {
                enclosing = pRight.enclosing;
                set = pRight.set;
                item = pRight.item;
                return *this;
            }

            void increment()
            {
                ++item;
                if(item == set->end()) // Find first in next set
                    *this = enclosing->nextBegin(set);
            }

            iterator &operator +=(unsigned int pCount)
            {
                *this = *this + pCount;
                return *this;
            }

            // Prefix increment
            iterator &operator ++()
            {
                increment();
                return *this;
            }

            // Postfix increment
            iterator operator ++(int)
            {
                iterator result = *this;
                increment();
                return result;
            }

            void decrement()
            {
                if(item == set->begin()) // Find last in previous set
                    *this = enclosing->previousLast(set);
                else
                    --item;
            }

            iterator &operator -=(unsigned int pCount)
            {
                *this = *this - pCount;
                return *this;
            }

            // Prefix decrement
            iterator &operator --()
            {
                decrement();
                return *this;
            }

            // Postfix decrement
            iterator operator --(int)
            {
                iterator result = *this;
                decrement();
                return result;
            }

            iterator &operator +(unsigned int pCount);
            iterator &operator -(unsigned int pCount);

            DistributedVector *enclosing;
            std::vector<tType> *set;
            set_iterator item;
        };

        iterator begin();
        iterator end();

        tType &front();
        tType &back();
        tType &operator [](unsigned int pOffset);

        // Insert new item before specified item
        void insert(iterator pBefore, const tType &pValue);

        // Add a new item to the end
        void push_back(const tType &pValue);

        // Remove specified item and return the item after it
        iterator erase(iterator pItem);

        std::vector<tType> *dataSet(unsigned int pOffset)
        {
            return mSets + pOffset;
        }

        void refresh();

    private:

        // Return the next iterator after the specified set
        iterator nextBegin(std::vector<tType> *pAfterSet);
        // Return the previous iterator before the specified set
        iterator previousLast(std::vector<tType> *pAfterSet);

        unsigned int mSetCount;
        unsigned int mSize;
        std::vector<tType> *mSets;
        std::vector<tType> *mEndSet;
        std::vector<tType> *mLastPushSet; // Highest set that has items in it

    };

    template <class tType>
    DistributedVector<tType>::DistributedVector(unsigned int pSetCount)
    {
        mSetCount = pSetCount;
        mSets = new std::vector<tType>[mSetCount];
        mEndSet = mSets + mSetCount;
        mLastPushSet = mSets;
        mSize = 0;
    }

    template <class tType>
    DistributedVector<tType>::~DistributedVector()
    {
        delete[] mSets;
    }

    template <class tType>
    void DistributedVector<tType>::reserve(unsigned int pCount)
    {
        unsigned int setReserveSize = pCount / mSetCount;
        // if(setReserveSize < 64)
            // setReserveSize = 64;
        for(std::vector<tType> *set=mSets;set<mEndSet;++set)
            set->reserve(setReserveSize);
    }

    template <class tType>
    void DistributedVector<tType>::clear()
    {
        for(std::vector<tType> *set=mSets;set<mEndSet;++set)
            set->clear();
        mLastPushSet = mSets;
        mSize = 0;
    }

    template <class tType>
    typename DistributedVector<tType>::iterator &DistributedVector<tType>::iterator::operator +(unsigned int pCount)
    {
        unsigned int currentOffset = item - set->begin();
        unsigned int remainingInThisSet = set->size() - currentOffset;
        if(remainingInThisSet == pCount)
        {
            *this = enclosing->nextBegin(set);
            return *this;
        }
        else if(remainingInThisSet > pCount)
        {
            item += pCount;
            return *this;
        }
        else if(set == enclosing->mEndSet - 1)
        {
            // Result would be past the end
            *this = enclosing->end();
            return *this;
        }

        unsigned int remaining = pCount - remainingInThisSet;
        for(std::vector<tType> *newSet=set+1;newSet<enclosing->mEndSet;++newSet)
        {
            if(newSet->size() > remaining)
            {
                set = newSet;
                item = newSet->begin() + remaining;
                return *this;
            }
            else
                remaining -= newSet->size();
        }

        *this = enclosing->end();
        return *this;
    }

    template <class tType>
    typename DistributedVector<tType>::iterator &DistributedVector<tType>::iterator::operator -(unsigned int pCount)
    {
        unsigned int currentOffset;
        if(item == set->end())
            currentOffset = set->size();
        else
            currentOffset = item - set->begin();

        if(currentOffset == pCount)
        {
            item = set->begin();
            return *this;
        }
        else if(currentOffset > pCount)
        {
            item -= pCount;
            return *this;
        }
        else if(set == enclosing->mSets)
        {
            // Result would be past the beginning
            *this = enclosing->begin();
            return *this;
        }

        unsigned int remaining = pCount - currentOffset;
        for(std::vector<tType> *newSet=set-1;newSet>=enclosing->mSets;--newSet)
        {
            if(newSet->size() > remaining)
            {
                set = newSet;
                item = newSet->begin() + (newSet->size() - remaining);
                return *this;
            }
            else if(newSet->size() == remaining)
            {
                set = newSet;
                item = newSet->begin();
                return *this;
            }
            else
                remaining -= newSet->size();
        }

        *this = enclosing->end();
        return *this;
    }

    template <class tType>
    typename DistributedVector<tType>::iterator DistributedVector<tType>::nextBegin(std::vector<tType> *pAfterSet)
    {
        std::vector<tType> *set = pAfterSet + 1;
        for(;set<mEndSet;++set)
            if(set->size() > 0)
                return iterator(this, set, set->begin());
        --set;
        return iterator(this, set, set->end());
    }

    template <class tType>
    typename DistributedVector<tType>::iterator DistributedVector<tType>::previousLast(std::vector<tType> *pBeforeSet)
    {
        if(pBeforeSet == mSets)
            return end();

        std::vector<tType> *set = pBeforeSet - 1;
        for(;set>=mSets;--set)
            if(set->size() > 0)
                return iterator(this, set, --set->end());

        return end();
    }

    template <class tType>
    typename DistributedVector<tType>::iterator DistributedVector<tType>::begin()
    {
        std::vector<tType> *set = mSets;
        for(;set<mEndSet;++set)
            if(set->size() > 0)
                return iterator(this, set, set->begin());
        --set;
        return iterator(this, set, set->begin());
    }

    template <class tType>
    typename DistributedVector<tType>::iterator DistributedVector<tType>::end()
    {
        std::vector<tType> *set = mSets + (mSetCount - 1);
        return iterator(this, set, set->end());
    }

    template <class tType>
    tType &DistributedVector<tType>::front()
    {
        std::vector<tType> *set = mSets;
        for(;set<mEndSet;++set)
            if(set->size() > 0)
                break;
        return set->front();
    }

    template <class tType>
    tType &DistributedVector<tType>::back()
    {
        return mLastPushSet->back();
    }

    template <class tType>
    tType &DistributedVector<tType>::operator [](unsigned int pOffset)
    {
        return *(begin() + pOffset);
    }

    template <class tType>
    void DistributedVector<tType>::insert(iterator pBefore, const tType &pValue)
    {
        if(pBefore.set != mSets && pBefore.item == pBefore.set->begin())
        {
            // Add to end of previous set
            std::vector<tType> *set = pBefore.set - 1;
            set->push_back(pValue);
            ++mSize;
        }
        else
        {
            pBefore.set->insert(pBefore.item, pValue);
            ++mSize;
        }
    }

    template <class tType>
    void DistributedVector<tType>::push_back(const tType &pValue)
    {
        while(true)
        {
            if(mLastPushSet->size() < mLastPushSet->capacity() || // This set has more capacity available
              mLastPushSet == (mEndSet - 1)) // This is the last set
            {
                mLastPushSet->push_back(pValue);
                ++mSize;
                return;
            }
            else
                ++mLastPushSet;
        }
    }

    template <class tType>
    typename DistributedVector<tType>::iterator DistributedVector<tType>::erase(iterator pItem)
    {
        set_iterator nextItem = pItem.set->erase(pItem.item);
        --mSize;
        if(nextItem == pItem.set->end())
            return nextBegin(pItem.set); // Return first item in next set
        else
            return iterator(this, pItem.set, nextItem); // Return next item in this set
    }

    template <class tType>
    void DistributedVector<tType>::refresh()
    {
        mLastPushSet = mSets;
        mSize = 0;
        for(std::vector<tType> *set=mSets;set<mEndSet;++set)
            if(set->size() > 0)
            {
                mLastPushSet = set;
                mSize += set->size();
            }
    }

    bool testDistributedVector();
}

#endif
