/**************************************************************************
 * Copyright 2018 NextCash, LLC                                           *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@nextcash.tech>                                  *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#include "hash_set.hpp"

#include "log.hpp"
#include "digest.hpp"

#ifdef PROFILER_ON
#include "profiler.hpp"
#endif

#define NEXTCASH_HASH_SET_LOG_NAME "HashSet"


namespace NextCash
{
    HashSet::Iterator &HashSet::Iterator::operator =(const Iterator &pRight)
    {
        mSet = pRight.mSet;
        mIter = pRight.mIter;
        return *this;
    }

    void HashSet::Iterator::increment()
    {
        ++mIter;
        if(mIter == mSet->end())
            gotoNextBegin();
    }

    void HashSet::Iterator::decrement()
    {
        if(mIter == mSet->begin() || (mIter == mSet->end() && mSet->size() == 0))
            gotoPreviousLast();
        else
            --mIter;
    }

    // Prefix increment
    HashSet::Iterator &HashSet::Iterator::operator ++()
    {
        increment();
        return *this;
    }

    // Postfix increment
    HashSet::Iterator HashSet::Iterator::operator ++(int)
    {
        Iterator result = *this;
        increment();
        return result;
    }

    // Prefix decrement
    HashSet::Iterator &HashSet::Iterator::operator --()
    {
        decrement();
        return *this;
    }

    // Postfix decrement
    HashSet::Iterator HashSet::Iterator::operator --(int)
    {
        Iterator result = *this;
        decrement();
        return result;
    }

    HashSet::Iterator HashSet::Iterator::erase()
    {
        SortedSet::Iterator resultIter = mSet->eraseNoDelete(mIter);
        if(resultIter == mSet->end())
        {
            Iterator result(mSet, mBeginSet, mEndSet, resultIter);
            result.gotoNextBegin();
            return result;
        }
        else
            return Iterator(mSet, mBeginSet, mEndSet, resultIter);
    }

    void HashSet::Iterator::gotoNextBegin()
    {
        if(mSet == mEndSet)
        {
            mIter = mSet->end();
            return;
        }

        ++mSet;
        while(true)
        {
            if(mSet->size() > 0)
            {
                mIter = mSet->begin();
                return;
            }
            else if(mSet == mEndSet)
            {
                mIter = mSet->end();
                return;
            }
            ++mSet;
        }
    }

    void HashSet::Iterator::gotoPreviousLast()
    {
        if(mSet == mBeginSet)
        {
            --mIter; // This is bad
            return;
        }

        --mSet;
        while(mSet->size() == 0 && mSet != mBeginSet)
            --mSet;
        mIter = --mSet->end();
    }

    HashSet::Iterator HashSet::begin()
    {
        Iterator result(mSets, mSets, mEndSet, mSets->begin());
        if(result.setIsEmpty())
            result.gotoNextBegin();
        return result;
    }

    HashSet::Iterator HashSet::end()
    {
        return Iterator(mEndSet, mSets, mEndSet, mEndSet->end());
    }

    // Return iterator to first matching item.
    HashSet::Iterator HashSet::find(const NextCash::Hash &pHash)
    {
        SortedSet *findSet = set(pHash);
        return Iterator(findSet, mSets, mEndSet, findSet->find(HashLookupObject(pHash)));
    }

    HashSet::Iterator HashSet::eraseDelete(Iterator &pIterator)
    {
        delete *pIterator;
        return pIterator.erase();
    }

    HashSet::Iterator HashSet::eraseNoDelete(Iterator &pIterator)
    {
        return pIterator.erase();
    }

    bool HashSet::test()
    {
        Log::add(Log::INFO, NEXTCASH_HASH_SET_LOG_NAME, "------------- Starting Hash Set Tests -------------");

        bool success = true;

        /***********************************************************************************************
         * Hash object set
         ***********************************************************************************************/
        class StringHash : public HashObject
        {
            Hash mHash;
            String mString;

            void calculateHash()
            {
                Digest digest(Digest::SHA256);
                digest.writeString(mString);
                digest.getResult(&mHash);
            }

        public:

            StringHash() {}
            ~StringHash() {}

            StringHash(const char *pText) : mString(pText)
            {
                calculateHash();
            }

            void setString(const char *pText)
            {
                mString = pText;
                calculateHash();
            }

            const String &getString() const { return mString; }

            // HashObject virtual functions
            const Hash &getHash() const { return mHash; }

            bool operator == (const HashObject *pRight) const
            {
                return mHash == pRight->getHash() &&
                  mString == ((StringHash *)pRight)->getString();
            }

            int compare(const HashObject *pRight) const
            {
                int result = mHash.compare(pRight->getHash());
                if(result != 0)
                    return result;

                return mString.compare(((StringHash *)pRight)->getString());
            }

        };

        HashSet set;

        StringHash *string1 = new StringHash("test1");
        StringHash *string2 = new StringHash("test2");
        StringHash *lookup;

        set.insert(string1);

        lookup = (StringHash *)set.get(string1->getHash());
        if(lookup == NULL)
        {
            Log::add(Log::ERROR, NEXTCASH_HASH_SET_LOG_NAME,
              "Failed hash string list 0 : not found");
            success = false;
        }
        else if(lookup->getString() != string1->getString())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_SET_LOG_NAME,
              "Failed hash string list 0 : %s", lookup->getString().text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_HASH_SET_LOG_NAME, "Passed hash string list 0");

        set.insert(string2);

        lookup = (StringHash *)set.get(string1->getHash());
        if(lookup == NULL)
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_SET_LOG_NAME,
              "Failed hash string list 1 : not found");
            success = false;
        }
        else if(lookup->getString() != string1->getString())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_SET_LOG_NAME,
              "Failed hash string list 1 : %s", lookup->getString().text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_HASH_SET_LOG_NAME, "Passed hash string list 1");

        lookup = (StringHash *)set.get(string2->getHash());
        if(lookup == NULL)
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_SET_LOG_NAME,
              "Failed hash string list 2 : not found");
            success = false;
        }
        else if(lookup->getString() != string2->getString())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_SET_LOG_NAME,
              "Failed hash string list 2 : %s", lookup->getString().text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_HASH_SET_LOG_NAME, "Passed hash string list 2");

        String newString;
        for(unsigned int i = 0; i < 500; ++i)
        {
            newString.writeFormatted("String %04d", i);
            set.insert(new StringHash(newString));

            // for(HashContainerList<String *>::Iterator item=set.begin();item!=set.end();++item)
            // {
                // Log::addFormatted(Log::DEBUG, NEXTCASH_HASH_SET_LOG_NAME,
                  // "Hash string list : %s <- %s", (*item)->text(), item.hash().hex().text());
            // }
        }

        lookup = (StringHash *)set.get(string1->getHash());
        if(lookup == NULL)
        {
            Log::add(Log::ERROR, NEXTCASH_HASH_SET_LOG_NAME,
              "Failed hash string list r1 : not found");
            success = false;
        }
        else if(lookup->getString() != string1->getString())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_SET_LOG_NAME,
              "Failed hash string list r1 : %s", lookup->getString().text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_HASH_SET_LOG_NAME, "Passed hash string list r1");

        lookup = (StringHash *)set.get(string2->getHash());
        if(lookup == NULL)
        {
            Log::add(Log::ERROR, NEXTCASH_HASH_SET_LOG_NAME,
              "Failed hash string list r2 : not found");
            success = false;
        }
        else if(lookup->getString() != string2->getString())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_SET_LOG_NAME,
              "Failed hash string list r2 : %s", lookup->getString().text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_HASH_SET_LOG_NAME, "Passed hash string list r2");

        // Find hash before first.
        StringHash *first = new StringHash();
        StringHash *actualFirst = (StringHash *)*set.begin();
        while(true)
        {
            newString.writeFormatted("String %d", Math::randomInt());
            first->setString(newString);
            if(first->getHash() < actualFirst->getHash())
            {
                set.insert(first);
                break;
            }
        }
        actualFirst = (StringHash *)*set.begin();

        // Find hash after last.
        StringHash *last = new StringHash();
        StringHash *actualLast = (StringHash *)*--set.end();
        while(true)
        {
            newString.writeFormatted("String %d", Math::randomInt());
            last->setString(newString);
            if(last->getHash() > actualLast->getHash())
            {
                set.insert(last);
                break;
            }
        }
        actualLast = (StringHash *)*--set.end();

        if(first->getHash() != actualFirst->getHash())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_SET_LOG_NAME,
              "Failed hash string list first : %s = %s", first->getHash().hex().text(),
              actualFirst->getHash().hex().text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_HASH_SET_LOG_NAME, "Passed hash string list first");

        if(first->getString() != actualFirst->getString())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_SET_LOG_NAME,
              "Failed hash string list first value : %s = %s", first->getString().text(),
              actualFirst->getString().text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_HASH_SET_LOG_NAME, "Passed hash string list first value");

        if(last->getHash() != actualLast->getHash())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_SET_LOG_NAME,
              "Failed hash string list last : %s = %s", last->getHash().hex().text(),
              actualLast->getHash().hex().text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_HASH_SET_LOG_NAME, "Passed hash string list last");

        if(last->getString() != actualLast->getString())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_SET_LOG_NAME,
              "Failed hash string list last value : %s = %s", last->getString().text(),
              actualLast->getString().text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_HASH_SET_LOG_NAME, "Passed hash string list last value");

        for(HashSet::Iterator iter = set.begin(); iter != set.end(); ++iter)
            Log::addFormatted(Log::INFO, NEXTCASH_HASH_SET_LOG_NAME, "%s : %s",
             (*iter)->getHash().hex().text(), ((StringHash *)*iter)->getString().text());

        return success;
    }
}
