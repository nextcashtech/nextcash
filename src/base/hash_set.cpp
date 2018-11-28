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
    void HashSet::Iterator::increment()
    {
        // Log::add(Log::DEBUG, NEXTCASH_HASH_SET_LOG_NAME, "Increment");
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

    HashSet::Iterator HashSet::Iterator::eraseNoDelete()
    {
        SortedSet::Iterator resultIter = mSet->eraseNoDelete(mIter);
        if(resultIter == mSet->end())
        {
            Iterator result(mSet, mBeginSet, mLastSet, resultIter);
            result.gotoNextBegin();
            return result;
        }
        else
            return Iterator(mSet, mBeginSet, mLastSet, resultIter);
    }

    void HashSet::Iterator::gotoNextBegin()
    {
        if(mSet == mLastSet)
        {
            // Log::add(Log::DEBUG, NEXTCASH_HASH_SET_LOG_NAME, "Already last set");
            mIter = mSet->end();
            return;
        }

        ++mSet;
        // Log::add(Log::DEBUG, NEXTCASH_HASH_SET_LOG_NAME, "Next set");
        while(true)
        {
            if(mSet->size() > 0)
            {
                // if(mSet == mLastSet)
                    // Log::add(Log::DEBUG, NEXTCASH_HASH_SET_LOG_NAME, "Non-empty end set");
                // else
                    // Log::add(Log::DEBUG, NEXTCASH_HASH_SET_LOG_NAME, "Non-empty set");
                mIter = mSet->begin();
                return;
            }
            else if(mSet == mLastSet)
            {
                // Log::add(Log::DEBUG, NEXTCASH_HASH_SET_LOG_NAME, "Last set");
                mIter = mSet->end();
                return;
            }
            else
            {
                ++mSet;
                // Log::add(Log::DEBUG, NEXTCASH_HASH_SET_LOG_NAME, "Next set");
            }
        }
    }

    void HashSet::Iterator::gotoPreviousLast()
    {
        if(mSet == mBeginSet)
        {
            --mIter; // This is bad
            Log::add(Log::WARNING, NEXTCASH_HASH_SET_LOG_NAME, "This is bad");
            return;
        }

        --mSet;
        while(mSet->size() == 0 && mSet != mBeginSet)
            --mSet;
        mIter = --mSet->end();
    }

    HashSet::Iterator HashSet::begin()
    {
        Iterator result(mSets, mSets, mLastSet, mSets->begin());
        if(result.setIsEmpty())
            result.gotoNextBegin();
        return result;
    }

    HashSet::Iterator HashSet::end()
    {
        return Iterator(mLastSet, mSets, mLastSet, mLastSet->end());
    }

    // Return iterator to first matching item.
    HashSet::Iterator HashSet::find(const NextCash::Hash &pHash)
    {
        SortedSet *findSet = set(pHash);
        SortedSet::Iterator result = findSet->find(HashLookupObject(pHash));
        if(result == findSet->end())
            return end();
        else
            return Iterator(findSet, mSets, mLastSet, result);
    }

    HashSet::Iterator HashSet::eraseDelete(Iterator &pIterator)
    {
        --mSize;
        delete *pIterator;
        return pIterator.eraseNoDelete();
    }

    HashSet::Iterator HashSet::eraseNoDelete(Iterator &pIterator)
    {
        --mSize;
        return pIterator.eraseNoDelete();
    }

    bool HashSet::test()
    {
        Log::add(Log::INFO, NEXTCASH_HASH_SET_LOG_NAME,
          "------------- Starting Hash Set Tests -------------");

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

            bool valueEquals(const HashObject *pRight) const
            {
                try
                {
                    return mString == dynamic_cast<const StringHash *>(pRight)->getString();
                }
                catch(...)
                {
                    return false;
                }
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
        // StringHash *first = new StringHash();
        // StringHash *actualFirst = (StringHash *)*set.begin();
        // while(true)
        // {
            // newString.writeFormatted("String %d", Math::randomInt());
            // first->setString(newString);
            // if(first->getHash() < actualFirst->getHash())
            // {
                // Log::addFormatted(Log::VERBOSE, NEXTCASH_HASH_SET_LOG_NAME,
                  // "Found item before first : %s", first->getString().text());
                // set.insert(first);
                // break;
            // }
        // }

        StringHash *first = new StringHash("String -1789157545");
        set.insert(first);
        StringHash *actualFirst = (StringHash *)*set.begin();

        // Find hash after last.
        // StringHash *last = new StringHash();
        // StringHash *actualLast = (StringHash *)*--set.end();
        // while(true)
        // {
            // newString.writeFormatted("String %d", Math::randomInt());
            // last->setString(newString);
            // if(last->getHash() > actualLast->getHash())
            // {
                // Log::addFormatted(Log::VERBOSE, NEXTCASH_HASH_SET_LOG_NAME,
                  // "Found item after last : %s", last->getString().text());
                // set.insert(last);
                // break;
            // }
        // }

        StringHash *last = new StringHash("String -67558938");
        set.insert(last);
        StringHash *actualLast = (StringHash *)*--set.end();

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

        unsigned int count = 0;
        for(HashSet::Iterator iter = set.begin(); iter != set.end(); ++iter, ++count)
            Log::addFormatted(Log::INFO, NEXTCASH_HASH_SET_LOG_NAME, "%s : %s",
             (*iter)->getHash().hex().text(), ((StringHash *)*iter)->getString().text());

        if(count != set.size())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_SET_LOG_NAME,
              "Failed hash set size : iterate count %d != size %d", count, set.size());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_HASH_SET_LOG_NAME, "Passed hash set size");

        return success;
    }
}
