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

            bool operator ==(const Iterator &pRight)
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
        void insert(Iterator pBefore, const tType &pValue);

        // Add a new item to the end
        void push_back(const tType &pValue);

        // Remove specified item and return the item after it
        Iterator erase(Iterator pItem);

        std::vector<tType> *dataSet(unsigned int pOffset) { return mSets + pOffset; }

        void refresh();

    private:

        // Return the next Iterator after the specified set
        Iterator nextBegin(std::vector<tType> *pAfterSet);
        // Return the previous Iterator before the specified set
        Iterator previousLast(std::vector<tType> *pAfterSet);

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
        for(;set>=mSets;--set)
            if(set->size() > 0)
                return Iterator(this, set, --set->end());

        return end();
    }

    template <class tType>
    typename DistributedVector<tType>::Iterator DistributedVector<tType>::begin()
    {
        std::vector<tType> *set = mSets;
        for(;set<mEndSet;++set)
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
        for(;set<mEndSet;++set)
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
    void DistributedVector<tType>::insert(Iterator pBefore, const tType &pValue)
    {
        if(pBefore.set != mSets && // Not first set
          pBefore.item == pBefore.set->begin()) // Inserting before first item of set
        {
            // Add to end of previous set
            std::vector<tType> *set = pBefore.set - 1;
            set->push_back(pValue);
            ++mSize;
        }
        else if(pBefore == end()) // Inserting as last item
            push_back(pValue);
        else
        {
            // Normal insert into this set
            pBefore.set->insert(pBefore.item, pValue);
            ++mSize;
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
                return;
            }
            else
                ++mLastSet;
        }
    }

    template <class tType>
    typename DistributedVector<tType>::Iterator DistributedVector<tType>::erase(Iterator pItem)
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
        for(std::vector<tType> *set=mSets;set<mEndSet;++set)
            if(set->size() > 0)
            {
                mLastSet = set;
                mSize += set->size();
            }
    }

    bool testDistributedVector();
}

#endif
