/**************************************************************************
 * Copyright 2017-2018 NextCash, LLC                                      *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@nextcash.tech>                                  *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#ifndef NEXTCASH_DISTRIBUTED_VECTOR_HPP
#define NEXTCASH_DISTRIBUTED_VECTOR_HPP

#include "log.hpp"

#include <cstring>
#include <vector>

#define NEXTCASH_DISTRIBUTED_VECTOR_LOG_NAME "DistributedVector"


namespace NextCash
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

        typedef typename std::vector<tType>::iterator SetIterator;

        class Iterator
        {
        public:
            Iterator()
            {
                enclosing = NULL;
                set = NULL;
            }
            Iterator(const Iterator &pCopy)
            {
                enclosing = pCopy.enclosing;
                set = pCopy.set;
                item = pCopy.item;
            }
            Iterator(DistributedVector *pEnclosing, std::vector<tType> *pSet, const SetIterator &pItem)
            {
                enclosing = pEnclosing;
                set = pSet;
                item = pItem;
            }

            tType &operator *() { return *item; }
            tType *operator ->() { return &(*item); }

            bool operator ==(const Iterator &pRight) const
            {
                return set == pRight.set && item == pRight.item;
            }

            bool operator !=(const Iterator &pRight)
            {
                return set != pRight.set || item != pRight.item;
            }

            Iterator &operator =(const Iterator &pRight)
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

            Iterator &operator +=(unsigned int pCount)
            {
                *this = *this + pCount;
                return *this;
            }

            // Prefix increment
            Iterator &operator ++()
            {
                increment();
                return *this;
            }

            // Postfix increment
            Iterator operator ++(int)
            {
                Iterator result = *this;
                increment();
                return result;
            }

            void decrement()
            {
                if(item == set->begin() || // First item of set
                  set->size() == 0) // Set is empty
                    *this = enclosing->previousLast(set); // Find last item in previous set
                else
                    --item;
            }

            Iterator &operator -=(unsigned int pCount)
            {
                *this = *this - pCount;
                return *this;
            }

            // Prefix decrement
            Iterator &operator --()
            {
                decrement();
                return *this;
            }

            // Postfix decrement
            Iterator operator --(int)
            {
                Iterator result = *this;
                decrement();
                return result;
            }

            Iterator operator +(unsigned int pCount) const;
            Iterator operator -(unsigned int pCount) const;

            DistributedVector *enclosing;
            std::vector<tType> *set;
            SetIterator item;
        };

        Iterator begin();
        Iterator end();

        tType &front();
        tType &back();
        tType &operator [](unsigned int pOffset);

        // Insert new item before specified item
        void insert(const Iterator &pBefore, const tType &pValue);

        // Add a new item to the end
        void push_back(const tType &pValue);

        // Remove specified item and return the item after it
        Iterator erase(const Iterator &pItem);

        std::vector<tType> *dataSet(unsigned int pOffset) { return mSets + pOffset; }

        void refresh();

    private:

        // Return the next Iterator after the specified set
        Iterator nextBegin(std::vector<tType> *pAfterSet);
        // Return the previous Iterator before the specified set
        Iterator previousLast(std::vector<tType> *pAfterSet);

        // Distribute items to other sets if one set is getting too large
        void distribute(std::vector<tType> *pSet, bool pFromPrevious, bool pFromNext);

        unsigned int mSetCount;
        unsigned int mSize;
        std::vector<tType> *mSets;
        std::vector<tType> *mEndSet;
        std::vector<tType> *mLastSet; // Last set that has items in it

    };

    template <class tType>
    DistributedVector<tType>::DistributedVector(unsigned int pSetCount)
    {
        mSetCount = pSetCount;
        mSets = new std::vector<tType>[mSetCount];
        mEndSet = mSets + mSetCount;
        mLastSet = mSets;
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
        mLastSet = mSets;
        mSize = 0;
    }

    template <class tType>
    typename DistributedVector<tType>::Iterator DistributedVector<tType>::Iterator::operator +(unsigned int pCount) const
    {
        unsigned int currentOffset = item - set->begin();
        unsigned int remainingInThisSet = set->size() - currentOffset;
        if(remainingInThisSet == pCount)
            return enclosing->nextBegin(set);
        else if(remainingInThisSet > pCount)
            return Iterator(enclosing, set, item + pCount);
        else if(set == enclosing->mEndSet - 1)
            return enclosing->end(); // Result would be past the end

        unsigned int remaining = pCount - remainingInThisSet;
        for(std::vector<tType> *newSet=set+1;newSet<enclosing->mEndSet;++newSet)
        {
            if(newSet->size() > remaining)
                return Iterator(enclosing, newSet, newSet->begin() + remaining);
            else
                remaining -= newSet->size();
        }

        return enclosing->end();
    }

    template <class tType>
    typename DistributedVector<tType>::Iterator DistributedVector<tType>::Iterator::operator -(unsigned int pCount) const
    {
        unsigned int currentOffset;
        if(item == set->end())
            currentOffset = set->size();
        else
            currentOffset = item - set->begin();

        if(currentOffset == pCount)
            return Iterator(enclosing, set, set->begin());
        else if(currentOffset > pCount)
            return Iterator(enclosing, set, item - pCount);
        else if(set == enclosing->mSets)
            return enclosing->begin(); // Result would be past the beginning

        unsigned int remaining = pCount - currentOffset;
        for(std::vector<tType> *newSet=set-1;newSet>=enclosing->mSets;--newSet)
        {
            if(newSet->size() > remaining)
                return Iterator(enclosing, newSet, newSet->begin() + (newSet->size() - remaining));
            else if(newSet->size() == remaining)
                return Iterator(enclosing, newSet, newSet->begin());
            else
                remaining -= newSet->size();
        }

        return enclosing->end();
    }

    template <class tType>
    typename DistributedVector<tType>::Iterator DistributedVector<tType>::nextBegin(std::vector<tType> *pAfterSet)
    {
        std::vector<tType> *set = pAfterSet + 1;
        for(;set<mEndSet;++set)
            if(set->size() > 0)
                return Iterator(this, set, set->begin());
        --set;
        return Iterator(this, set, set->end());
    }

    template <class tType>
    typename DistributedVector<tType>::Iterator DistributedVector<tType>::previousLast(std::vector<tType> *pBeforeSet)
    {
        if(pBeforeSet == mSets)
            return end();

        std::vector<tType> *set = pBeforeSet - 1;
        --set;
        for(;set>=mSets;--set)
            if(set->size() > 0)
                return Iterator(this, set, --set->end());

        return end();
    }

    template <class tType>
    typename DistributedVector<tType>::Iterator DistributedVector<tType>::begin()
    {
        std::vector<tType> *set = mSets;
        for(; set < mEndSet; ++set)
            if(set->size() > 0)
                return Iterator(this, set, set->begin());
        --set;
        return Iterator(this, set, set->begin());
    }

    template <class tType>
    typename DistributedVector<tType>::Iterator DistributedVector<tType>::end()
    {
        std::vector<tType> *set = mSets + mSetCount - 1;
        return Iterator(this, set, set->end());
    }

    template <class tType>
    tType &DistributedVector<tType>::front()
    {
        std::vector<tType> *set = mSets;
        for(; set < mEndSet; ++set)
            if(set->size() > 0)
                break;
        return set->front();
    }

    template <class tType>
    tType &DistributedVector<tType>::back()
    {
        return mLastSet->back();
    }

    template <class tType>
    tType &DistributedVector<tType>::operator [](unsigned int pOffset)
    {
        return *(begin() + pOffset);
    }

    template <class tType>
    void DistributedVector<tType>::distribute(std::vector<tType> *pSet, bool pFromPrevious, bool pFromNext)
    {
        // Check if set size is larger than it should be
        unsigned int maxSize = (mSize / mSetCount) * (mSetCount / 10);
        if(maxSize < 1024)
            maxSize = 1024;
        if(pSet->size() < maxSize)
            return;
        unsigned int minMove = maxSize / 4;

        int currentOffset = pSet - mSets;
        if(currentOffset < 0)
            return;

        // Get previous and next sets.
        unsigned int totalSize = pSet->size();
        unsigned int count = 1; // 1 for current set
        std::vector<tType> *previousSet, *nextSet;

        if(pFromPrevious || currentOffset == 0)
            previousSet = NULL;
        else
        {
            previousSet = pSet;
            --previousSet;
            totalSize += previousSet->size();
            ++count;
        }

        if(pFromNext || (unsigned int)currentOffset >= mSetCount - 1)
            nextSet = NULL;
        else
        {
            nextSet = pSet;
            ++nextSet;
            totalSize += nextSet->size();
            ++count;
        }

        if(previousSet == NULL && nextSet == NULL)
            return; // No place to distribute

        // Make these sets all the same size.
        unsigned int targetSize = totalSize / count;
        unsigned int addCount;
        unsigned int originalSize = pSet->size();

        if(nextSet != NULL && nextSet->size() >= targetSize)
        {
            totalSize -= nextSet->size();
            --count;
            nextSet = NULL;
            targetSize = totalSize / count;
        }

        if(previousSet != NULL && previousSet->size() >= targetSize)
        {
            totalSize -= previousSet->size();
            --count;
            previousSet = NULL;
            targetSize = totalSize / count;
        }

        if(nextSet != NULL && nextSet->size() < targetSize)
        {
            // Move end of current set first so removing items from the beginning is cheaper.
            addCount = targetSize - nextSet->size();
            if(addCount > minMove || addCount >= pSet->size() - minMove)
            {
                Log::addFormatted(Log::DEBUG, NEXTCASH_DISTRIBUTED_VECTOR_LOG_NAME,
                  "Distributing set %d : Moving %d/%d items to next of size %d", currentOffset,
                  addCount, originalSize, nextSet->size());

                // Insert items from end of current set at beginning of the next set.
                nextSet->insert(nextSet->begin(), pSet->end() - addCount, pSet->end());

                // Remove items from the end of the current set.
                pSet->erase(pSet->end() - addCount, pSet->end());
            }
        }

        if(previousSet != NULL && previousSet->size() < targetSize)
        {
            addCount = targetSize - previousSet->size();
            if(addCount > minMove || addCount >= pSet->size() - minMove)
            {
                Log::addFormatted(Log::DEBUG, NEXTCASH_DISTRIBUTED_VECTOR_LOG_NAME,
                  "Distributing set %d : Moving %d/%d items to previous of size %d", currentOffset,
                  addCount, originalSize, previousSet->size());

                // Append items from the beginning of the current set to the end or previous set.
                unsigned int previousSize = previousSet->size();
                previousSet->resize(previousSet->size() + addCount);
                std::move(pSet->begin(), pSet->begin() + addCount,
                  previousSet->begin() + previousSize);

                // Remove items from the beginning of the current set.
                pSet->erase(pSet->begin(), pSet->begin() + addCount);
            }
        }

        if(previousSet != NULL)
            distribute(previousSet, true, false);
        if(nextSet != NULL)
            distribute(nextSet, false, true);
    }

    template <class tType>
    void DistributedVector<tType>::insert(const Iterator &pBefore, const tType &pValue)
    {
        if(pBefore.set != mSets && // Not first set
          pBefore.item == pBefore.set->begin()) // Inserting before first item of set
        {
            // Add to end of previous set
            std::vector<tType> *set = pBefore.set;
            --set;
            set->push_back(pValue);
            ++mSize;
            distribute(set, false, false);
        }
        else if(pBefore == end()) // Inserting as last item
            push_back(pValue);
        else if(pBefore.item == pBefore.set->end())
        {
            // Append to this set
            pBefore.set->push_back(pValue);
            ++mSize;
            distribute(pBefore.set, false, false);
        }
        else
        {
            // Normal insert into this set
            pBefore.set->insert(pBefore.item, pValue);
            ++mSize;
            distribute(pBefore.set, false, false);
        }
    }

    template <class tType>
    void DistributedVector<tType>::push_back(const tType &pValue)
    {
        while(true)
        {
            if(mLastSet->size() < mLastSet->capacity() || // This set has more capacity available
              mLastSet == (mEndSet - 1)) // This is the last possible set
            {
                mLastSet->push_back(pValue);
                ++mSize;
                distribute(mLastSet, false, false);
                return;
            }
            else
                ++mLastSet;
        }
    }

    template <class tType>
    typename DistributedVector<tType>::Iterator DistributedVector<tType>::erase(const Iterator &pItem)
    {
        SetIterator nextItem = pItem.set->erase(pItem.item);
        --mSize;
        if(nextItem == pItem.set->end())
        {
            if(pItem.set == mLastSet)
            {
                // Update last set
                while(mLastSet->size() == 0 && mLastSet > mSets)
                    --mLastSet;
                return end();
            }
            else
                return nextBegin(pItem.set); // Return first item in next set
        }
        else
            return Iterator(this, pItem.set, nextItem); // Return next item in this set
    }

    template <class tType>
    void DistributedVector<tType>::refresh()
    {
        mLastSet = mSets;
        mSize = 0;
        for(std::vector<tType> *set = mSets; set < mEndSet; ++set)
            if(set->size() > 0)
            {
                mLastSet = set;
                mSize += set->size();
            }
    }

    bool testDistributedVector();
}

#endif
