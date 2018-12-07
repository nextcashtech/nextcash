/**************************************************************************
 * Copyright 2018 NextCash, LLC                                           *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@nextcash.tech>                                  *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#include "reference_hash_set.hpp"

#include "log.hpp"
#include "digest.hpp"

#ifdef PROFILER_ON
#include "profiler.hpp"
#endif


namespace NextCash
{
    bool testReferenceHashSet()
    {
        Log::add(Log::INFO, NEXTCASH_REF_HASH_SET_LOG_NAME,
          "------------- Starting Reference Hash Set Tests -------------");

        class StringHash
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

            const Hash &getHash() { return mHash; }

            bool valueEquals(StringHash &pRight)
            {
                return mString == pRight.mString;
            }

            int compare(StringHash &pRight)
            {
                int result = mHash.compare(pRight.mHash);
                if(result != 0)
                    return result;

                return mString.compare(pRight.mString);
            }

        };

        bool success = true;

        typedef ReferenceCounter<StringHash> StringHashReference;
        ReferenceHashSet<StringHash> set;

        StringHash *string1 = new StringHash("test1");
        StringHashReference string1Ref(string1);
        StringHash *string2 = new StringHash("test2");
        StringHashReference string2Ref(string2);
        StringHashReference lookup;

        set.insert(string1Ref);

        lookup = set.get(string1->getHash());
        if(!lookup)
        {
            Log::add(Log::ERROR, NEXTCASH_REF_HASH_SET_LOG_NAME,
              "Failed ref hash string list 0 : not found");
            success = false;
        }
        else if(lookup->getString() != string1->getString())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_REF_HASH_SET_LOG_NAME,
              "Failed ref hash string list 0 : %s", lookup->getString().text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_REF_HASH_SET_LOG_NAME, "Passed ref hash string list 0");

        set.insert(string2Ref);

        lookup = set.get(string1->getHash());
        if(!lookup)
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_REF_HASH_SET_LOG_NAME,
              "Failed ref hash string list 1 : not found");
            success = false;
        }
        else if(lookup->getString() != string1->getString())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_REF_HASH_SET_LOG_NAME,
              "Failed ref hash string list 1 : %s", lookup->getString().text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_REF_HASH_SET_LOG_NAME, "Passed ref hash string list 1");

        lookup = set.get(string2->getHash());
        if(!lookup)
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_REF_HASH_SET_LOG_NAME,
              "Failed ref hash string list 2 : not found");
            success = false;
        }
        else if(lookup->getString() != string2->getString())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_REF_HASH_SET_LOG_NAME,
              "Failed ref hash string list 2 : %s", lookup->getString().text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_REF_HASH_SET_LOG_NAME, "Passed ref hash string list 2");

        String newString;
        StringHashReference newStringHashReference;
        for(unsigned int i = 0; i < 500; ++i)
        {
            newString.writeFormatted("String %04d", i);
            newStringHashReference = new StringHash(newString);
            set.insert(newStringHashReference);

            // for(HashContainerList<String *>::Iterator item=set.begin();item!=set.end();++item)
            // {
                // Log::addFormatted(Log::DEBUG, NEXTCASH_REF_HASH_SET_LOG_NAME,
                  // "Hash string list : %s <- %s", (*item)->text(), item.hash().hex().text());
            // }
        }

        lookup = set.get(string1->getHash());
        if(!lookup)
        {
            Log::add(Log::ERROR, NEXTCASH_REF_HASH_SET_LOG_NAME,
              "Failed ref hash string list r1 : not found");
            success = false;
        }
        else if(lookup->getString() != string1->getString())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_REF_HASH_SET_LOG_NAME,
              "Failed ref hash string list r1 : %s", lookup->getString().text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_REF_HASH_SET_LOG_NAME, "Passed ref hash string list r1");

        lookup = set.get(string2->getHash());
        if(!lookup)
        {
            Log::add(Log::ERROR, NEXTCASH_REF_HASH_SET_LOG_NAME,
              "Failed ref hash string list r2 : not found");
            success = false;
        }
        else if(lookup->getString() != string2->getString())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_REF_HASH_SET_LOG_NAME,
              "Failed ref hash string list r2 : %s", lookup->getString().text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_REF_HASH_SET_LOG_NAME, "Passed ref hash string list r2");

        // Find hash before first.
        // StringHash *first = new StringHash();
        // StringHash *actualFirst = (StringHash *)*set.begin();
        // while(true)
        // {
            // newString.writeFormatted("String %d", Math::randomInt());
            // first->setString(newString);
            // if(first->getHash() < actualFirst->getHash())
            // {
                // Log::addFormatted(Log::VERBOSE, NEXTCASH_REF_HASH_SET_LOG_NAME,
                  // "Found item before first : %s", first->getString().text());
                // set.insert(first);
                // break;
            // }
        // }

        StringHash *first = new StringHash("String -1789157545");
        StringHashReference firstRef = first;
        set.insert(firstRef);
        StringHashReference actualFirst = set.front();

        // Find hash after last.
        // StringHash *last = new StringHash();
        // StringHash *actualLast = (StringHash *)*--set.end();
        // while(true)
        // {
            // newString.writeFormatted("String %d", Math::randomInt());
            // last->setString(newString);
            // if(last->getHash() > actualLast->getHash())
            // {
                // Log::addFormatted(Log::VERBOSE, NEXTCASH_REF_HASH_SET_LOG_NAME,
                  // "Found item after last : %s", last->getString().text());
                // set.insert(last);
                // break;
            // }
        // }

        StringHash *last = new StringHash("String -67558938");
        StringHashReference lastRef = last;
        set.insert(lastRef);
        StringHashReference actualLast = set.back();

        if(first->getHash() != actualFirst->getHash())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_REF_HASH_SET_LOG_NAME,
              "Failed ref hash string list first : %s = %s", first->getHash().hex().text(),
              actualFirst->getHash().hex().text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_REF_HASH_SET_LOG_NAME,
              "Passed ref hash string list first");

        if(first->getString() != actualFirst->getString())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_REF_HASH_SET_LOG_NAME,
              "Failed ref hash string list first value : %s = %s", first->getString().text(),
              actualFirst->getString().text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_REF_HASH_SET_LOG_NAME,
              "Passed ref hash string list first value");

        if(last->getHash() != actualLast->getHash())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_REF_HASH_SET_LOG_NAME,
              "Failed ref hash string list last : %s = %s", last->getHash().hex().text(),
              actualLast->getHash().hex().text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_REF_HASH_SET_LOG_NAME,
              "Passed ref hash string list last");

        if(last->getString() != actualLast->getString())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_REF_HASH_SET_LOG_NAME,
              "Failed ref hash string list last value : %s = %s", last->getString().text(),
              actualLast->getString().text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_REF_HASH_SET_LOG_NAME,
              "Passed ref hash string list last value");

        unsigned int count = 0;
        for(ReferenceHashSet<StringHash>::Iterator iter = set.begin(); iter != set.end();
          ++iter, ++count)
        {
            if(count == 0 || count == set.size() - 1)
                Log::addFormatted(Log::INFO, NEXTCASH_REF_HASH_SET_LOG_NAME, "%s : %s",
                 (*iter)->getHash().hex().text(), (*iter)->getString().text());
        }

        if(count != set.size())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_REF_HASH_SET_LOG_NAME,
              "Failed ref hash set size : iterate count %d != size %d", count, set.size());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_REF_HASH_SET_LOG_NAME, "Passed ref hash set size");

        return success;
    }
}
