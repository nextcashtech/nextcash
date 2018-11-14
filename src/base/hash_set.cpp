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
    HashSet::HashSet()
    {

    }

    HashSet::~HashSet()
    {
        for(Iterator item = mItems.begin(); item != mItems.end(); ++item)
            delete *item;
    }

    void HashSet::getHashList(HashList &pList) const
    {
        pList.clear();
        pList.reserve(mItems.size());
        for(ConstIterator item = mItems.begin(); item != mItems.end(); ++item)
            pList.emplace_back((*item)->getHash());
    }

    bool HashSet::contains(const Hash &pHash)
    {
        uint8_t flags = 0x00;
        binaryFind(pHash, flags);
        return flags & WAS_FOUND;
    }

    bool HashSet::insert(HashObject *pObject)
    {
        uint8_t flags = RETURN_ITEM_AFTER;
        Iterator item = binaryFind(pObject->getHash(), flags);

        if(flags & WAS_FOUND)
            return false; // Already in list

        if(flags & BELONGS_AT_END)
            mItems.push_back(pObject); // Add to end of list
        else
            mItems.insert(item, pObject); // Insert in list

        return true;
    }

    bool HashSet::remove(const Hash &pHash)
    {
        HashObject *object = getAndRemove(pHash);
        if(object != NULL)
        {
            delete object;
            return true;
        }
        return false;
    }

    bool HashSet::removeAt(unsigned int pOffset)
    {
        if(mItems.size() <= pOffset)
            return false;

        Iterator item = mItems.begin() + pOffset;
        delete *item;
        mItems.erase(item);
        return true;
    }

    HashObject *HashSet::get(const NextCash::Hash &pHash)
    {
        uint8_t flags = 0x00;
        Iterator item = binaryFind(pHash, flags);
        if(item != mItems.end())
            return *item;
        else
            return NULL;
    }

    HashObject *HashSet::getAndRemove(const NextCash::Hash &pHash)
    {
        uint8_t flags = 0x00;
        Iterator item = binaryFind(pHash, flags);
        if(item != mItems.end())
        {
            HashObject *result = *item;
            mItems.erase(item);
            return result;
        }
        else
            return NULL;
    }

    HashObject *HashSet::getAndRemoveAt(unsigned int pOffset)
    {
        if(mItems.size() <= pOffset)
            return NULL;

        Iterator item = mItems.begin() + pOffset;
        HashObject *result = *item;
        mItems.erase(item);
        return result;
    }

    void HashSet::clear()
    {
        for(Iterator item = mItems.begin(); item != mItems.end(); ++item)
            delete *item;
        mItems.clear();
    }

    void HashSet::clearNoDelete()
    {
        mItems.clear();
    }

    HashSet::Iterator HashSet::binaryFind(const Hash &pHash, uint8_t &pFlags)
    {
#ifdef PROFILER_ON
        ProfilerReference profiler(getProfiler(PROFILER_SET, PROFILER_HASH_SET_FIND_ID,
          PROFILER_HASH_SET_FIND_NAME), true);
#endif

        if(mItems.size() == 0)
        {
            // Item would be only item in list.
            pFlags |= BELONGS_AT_END;
            return mItems.end();
        }

        int compare = pHash.compare(mItems.back()->getHash());
        if(compare > 0)
        {
            // Item would be after end of list.
            pFlags |= BELONGS_AT_END;
            return mItems.end();
        }
        else if(compare == 0)
        {
            pFlags |= WAS_FOUND;
            return --mItems.end();
        }

        compare = pHash.compare(mItems.front()->getHash());
        if(compare < 0)
        {
            // Item would be before beginning of list.
            if(pFlags & RETURN_ITEM_AFTER)
                return mItems.begin(); // Return first item to insert before.
            else
                return mItems.end();
        }
        else if(compare == 0)
        {
            pFlags |= WAS_FOUND;
            return mItems.begin();
        }

        Iterator bottom = mItems.begin();
        Iterator top    = --mItems.end();
        Iterator current;

        while(true)
        {
            // Break the set in two halves
            current = bottom + ((top - bottom) / 2);
            compare = pHash.compare((*current)->getHash());

            if(compare == 0) // Item found
            {
                pFlags |= WAS_FOUND;
                return current;
            }

            if(current == bottom) // Item not found
            {
                if(pFlags & RETURN_ITEM_AFTER)
                    return top;
                else
                    return mItems.end();
            }

            // Determine which half the desired item is in
            if(compare > 0)
                bottom = current;
            else //if(compare < 0)
                top = current;
        }
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
        for(unsigned int i = 0; i < 100; ++i)
        {
            newString.writeFormatted("String %04d", Math::randomInt() % 1000);
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

        HashList list;
        set.getHashList(list);

        // Find hash before first.
        StringHash *first = new StringHash();
        while(true)
        {
            newString.writeFormatted("String %d", Math::randomInt());
            first->setString(newString);
            if(first->getHash() < list.front())
            {
                set.insert(first);
                break;
            }
        }

        // Find hash after last.
        StringHash *last = new StringHash();
        while(true)
        {
            newString.writeFormatted("String %d", Math::randomInt());
            last->setString(newString);
            if(last->getHash() > list.back())
            {
                set.insert(last);
                break;
            }
        }

        set.getHashList(list);

        if(first->getHash() != list.front())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_SET_LOG_NAME,
              "Failed hash string list first : %s = %s", first->getHash().hex().text(),
              list.front().hex().text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_HASH_SET_LOG_NAME, "Passed hash string list first");

        if(first->getString() != ((StringHash *)set.get(list.front()))->getString())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_SET_LOG_NAME,
              "Failed hash string list first value : %s = %s", first->getString().text(),
              ((StringHash *)set.get(list.front()))->getString().text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_HASH_SET_LOG_NAME, "Passed hash string list first value");

        if(last->getHash() != list.back())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_SET_LOG_NAME,
              "Failed hash string list last : %s = %s", last->getString().text(),
              ((StringHash *)set.get(list.back()))->getString().text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_HASH_SET_LOG_NAME, "Passed hash string list last");

        if(last->getString() != ((StringHash *)set.get(list.back()))->getString())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_SET_LOG_NAME,
              "Failed hash string list last value : %s = %s", last->getString().text(),
              ((StringHash *)set.get(list.back()))->getString().text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_HASH_SET_LOG_NAME, "Passed hash string list last value");

        return success;
    }
}
