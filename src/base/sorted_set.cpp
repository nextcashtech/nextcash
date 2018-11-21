/**************************************************************************
 * Copyright 2018 NextCash, LLC                                           *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@nextcash.tech>                                  *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#include "sorted_set.hpp"

#include "log.hpp"
#include "digest.hpp"
#include "hash.hpp"

#ifdef PROFILER_ON
#include "profiler.hpp"
#endif

#define NEXTCASH_SORTED_SET_LOG_NAME "SortedSet"


namespace NextCash
{
    SortedSet::~SortedSet()
    {
        for(Iterator item = mItems.begin(); item != mItems.end(); ++item)
            delete *item;
    }

    bool SortedSet::contains(const SortedObject &pMatching)
    {
        uint8_t flags = 0x00;
        binaryFind(pMatching, flags);
        return flags & WAS_FOUND;
    }

    bool SortedSet::insert(SortedObject *pObject, bool pAllowDuplicates)
    {
        uint8_t flags = RETURN_INSERT_POSITION;
        Iterator item = binaryFind(*pObject, flags);

        if(flags & WAS_FOUND && !pAllowDuplicates)
            return false; // Already in list

        mItems.insert(item, pObject); // Insert in list
        return true;
    }

    bool SortedSet::remove(const SortedObject &pMatching)
    {
        SortedObject *object = getAndRemove(pMatching);
        if(object != NULL)
        {
            delete object;
            return true;
        }
        return false;
    }

    SortedObject *SortedSet::get(const SortedObject &pMatching)
    {
        uint8_t flags = 0x00;
        Iterator item = binaryFind(pMatching, flags);
        if(item != mItems.end())
            return *item;
        else
            return NULL;
    }

    SortedObject *SortedSet::getAndRemove(const SortedObject &pMatching)
    {
        uint8_t flags = 0x00;
        Iterator item = binaryFind(pMatching, flags);
        if(item != mItems.end())
        {
            SortedObject *result = *item;
            mItems.erase(item);
            return result;
        }
        else
            return NULL;
    }

    SortedSet::Iterator SortedSet::find(const SortedObject &pMatching)
    {
        uint8_t flags = 0x00;
        return binaryFind(pMatching, flags);
    }

    void SortedSet::clear()
    {
        for(Iterator item = mItems.begin(); item != mItems.end(); ++item)
            delete *item;
        mItems.clear();
    }

    void SortedSet::clearNoDelete()
    {
        mItems.clear();
    }

    SortedSet::Iterator SortedSet::moveForward(Iterator pIterator, const SortedObject &pMatching,
      uint8_t &pFlags)
    {
        ++pIterator;
        while(pIterator != end() && pMatching.compare(*pIterator) == 0)
            ++pIterator;

        if(!(pFlags & RETURN_INSERT_POSITION))
            --pIterator;

        return pIterator;
    }

    SortedSet::Iterator SortedSet::moveBackward(Iterator pIterator, const SortedObject &pMatching)
    {
        if(pIterator == begin())
            return pIterator;

        --pIterator;
        while(pMatching.compare(*pIterator) == 0)
        {
            if(pIterator == begin())
                return pIterator;
            --pIterator;
        }

        return ++pIterator;
    }

    SortedSet::Iterator SortedSet::binaryFind(const SortedObject &pMatching, uint8_t &pFlags)
    {
#ifdef PROFILER_ON
        ProfilerReference profiler(getProfiler(PROFILER_SET, PROFILER_SORTED_SET_FIND_ID,
          PROFILER_SORTED_SET_FIND_NAME), true);
#endif

        if(mItems.size() == 0) // Item would be only item in list.
            return mItems.end();

        int compare = pMatching.compare(mItems.back());
        if(compare > 0) // Item would be after end of list.
            return mItems.end();
        else if(compare == 0)
        {
            // Item matches last item.
            pFlags |= WAS_FOUND;
            if(pFlags & RETURN_INSERT_POSITION)
                return mItems.end();
            else
                return moveBackward(--mItems.end(), pMatching);
        }

        compare = pMatching.compare(mItems.front());
        if(compare < 0)
        {
            // Item would be before beginning of list.
            if(pFlags & RETURN_INSERT_POSITION)
                return mItems.begin(); // Return first item to insert before.
            else
                return mItems.end();
        }
        else if(compare == 0)
        {
            // Item matches first item.
            pFlags |= WAS_FOUND;
            if(pFlags & RETURN_INSERT_POSITION)
                return moveForward(mItems.begin(), pMatching, pFlags);
            else
                return mItems.begin();
        }

        Iterator bottom = mItems.begin();
        Iterator top    = --mItems.end();
        Iterator current;

        while(true)
        {
            // Break the set in two halves
            current = bottom + ((top - bottom) / 2);
            compare = pMatching.compare((*current));

            if(compare == 0) // Matching item found
            {
                pFlags |= WAS_FOUND;
                if(pFlags & RETURN_INSERT_POSITION)
                    return moveForward(current, pMatching, pFlags);
                else
                    return moveBackward(current, pMatching);
            }

            if(current == bottom) // Matching item not found
            {
                if(pFlags & RETURN_INSERT_POSITION)
                    return top;
                else
                    return mItems.end();
            }

            // Determine which half contains the desired item
            if(compare > 0)
                bottom = current;
            else //if(compare < 0)
                top = current;
        }
    }

    bool SortedSet::test()
    {
        Log::add(Log::INFO, NEXTCASH_SORTED_SET_LOG_NAME, "------------- Starting Sorted Set Tests -------------");

        bool success = true;

        /***********************************************************************************************
         * Sorted object set
         ***********************************************************************************************/
        class SortedString : public SortedObject
        {
            String mString;

        public:

            SortedString() {}
            SortedString(const SortedString &pCopy)
            {
                mString = pCopy.mString;
            }
            ~SortedString() {}

            SortedString(const char *pText) : mString(pText) {}

            void setString(const char *pText) { mString = pText; }

            const String &getString() const { return mString; }

            bool operator == (const SortedObject *pRight) const
            {
                return mString == ((SortedString *)pRight)->getString() &&
                  mString == ((SortedString *)pRight)->getString();
            }

            int compare(const SortedObject *pRight) const
            {
                int result = mString.compare(((SortedString *)pRight)->getString());
                if(result != 0)
                    return result;

                return mString.compare(((SortedString *)pRight)->getString());
            }

        };

        SortedSet set;

        SortedString *string1 = new SortedString("test1");
        SortedString *string2 = new SortedString("test2");
        SortedString *lookup;

        set.insert(string1);

        lookup = (SortedString *)set.get(*string1);
        if(lookup == NULL)
        {
            Log::add(Log::ERROR, NEXTCASH_SORTED_SET_LOG_NAME,
              "Failed sorted string list 0 : not found");
            success = false;
        }
        else if(lookup->getString() != string1->getString())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_SORTED_SET_LOG_NAME,
              "Failed sorted string list 0 : %s", lookup->getString().text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_SORTED_SET_LOG_NAME, "Passed sorted string list 0");

        set.insert(string2);

        lookup = (SortedString *)set.get(*string1);
        if(lookup == NULL)
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_SORTED_SET_LOG_NAME,
              "Failed sorted string list 1 : not found");
            success = false;
        }
        else if(lookup->getString() != string1->getString())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_SORTED_SET_LOG_NAME,
              "Failed sorted string list 1 : %s", lookup->getString().text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_SORTED_SET_LOG_NAME, "Passed sorted string list 1");

        lookup = (SortedString *)set.get(*string2);
        if(lookup == NULL)
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_SORTED_SET_LOG_NAME,
              "Failed sorted string list 2 : not found");
            success = false;
        }
        else if(lookup->getString() != string2->getString())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_SORTED_SET_LOG_NAME,
              "Failed sorted string list 2 : %s", lookup->getString().text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_SORTED_SET_LOG_NAME, "Passed sorted string list 2");

        String newString;
        for(unsigned int i = 0; i < 100; ++i)
        {
            newString.writeFormatted("String %04d", Math::randomInt() % 1000);
            set.insert(new SortedString(newString));

            // for(HashContainerList<String *>::Iterator item=set.begin();item!=set.end();++item)
            // {
                // Log::addFormatted(Log::DEBUG, NEXTCASH_SORTED_SET_LOG_NAME,
                  // "Hash string list : %s <- %s", (*item)->text(), item.hash().hex().text());
            // }
        }

        lookup = (SortedString *)set.get(*string1);
        if(lookup == NULL)
        {
            Log::add(Log::ERROR, NEXTCASH_SORTED_SET_LOG_NAME,
              "Failed sorted string list r1 : not found");
            success = false;
        }
        else if(lookup->getString() != string1->getString())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_SORTED_SET_LOG_NAME,
              "Failed sorted string list r1 : %s", lookup->getString().text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_SORTED_SET_LOG_NAME, "Passed sorted string list r1");

        lookup = (SortedString *)set.get(*string2);
        if(lookup == NULL)
        {
            Log::add(Log::ERROR, NEXTCASH_SORTED_SET_LOG_NAME,
              "Failed sorted string list r2 : not found");
            success = false;
        }
        else if(lookup->getString() != string2->getString())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_SORTED_SET_LOG_NAME,
              "Failed sorted string list r2 : %s", lookup->getString().text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_SORTED_SET_LOG_NAME, "Passed sorted string list r2");

        // Insert before first.
        SortedString *first = new SortedString("AString");
        set.insert(first);

        // Insert after last.
        SortedString *last = new SortedString("zString");
        set.insert(last);

        if(first->getString() != ((SortedString *)*set.begin())->getString())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_SORTED_SET_LOG_NAME,
              "Failed sorted string list first : %s = %s", first->getString().text(),
              ((SortedString *)*set.begin())->getString().text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_SORTED_SET_LOG_NAME, "Passed sorted string list first");

        if(last->getString() != ((SortedString *)*--set.end())->getString())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_SORTED_SET_LOG_NAME,
              "Failed sorted string list last : %s = %s", last->getString().text(),
              ((SortedString *)*--set.end())->getString().text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_SORTED_SET_LOG_NAME, "Passed sorted string list last");

        // Insert first again
        first = new SortedString(*first);
        set.insert(first, true);
        SortedSet::Iterator item = set.find(*first);

        if(first->getString() != ((SortedString *)*item)->getString())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_SORTED_SET_LOG_NAME,
              "Failed sorted string list first twice(1) : %s = %s", first->getString().text(),
              ((SortedString *)*item)->getString().text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_SORTED_SET_LOG_NAME,
              "Passed sorted string list first twice(1)");

        ++item;
        if(first->getString() != ((SortedString *)*item)->getString())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_SORTED_SET_LOG_NAME,
              "Failed sorted string list first twice(2) : %s = %s", first->getString().text(),
              ((SortedString *)*item)->getString().text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_SORTED_SET_LOG_NAME,
              "Passed sorted string list first twice(2)");

        // Insert last again
        last = new SortedString(*last);
        set.insert(last, true);
        item = set.find(*last);

        if(last->getString() != ((SortedString *)*item)->getString())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_SORTED_SET_LOG_NAME,
              "Failed sorted string list last twice(1) : %s = %s", last->getString().text(),
              ((SortedString *)*item)->getString().text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_SORTED_SET_LOG_NAME,
              "Passed sorted string list last twice(1)");

        ++item;
        if(last->getString() != ((SortedString *)*item)->getString())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_SORTED_SET_LOG_NAME,
              "Failed sorted string list last twice(2) : %s = %s", last->getString().text(),
              ((SortedString *)*item)->getString().text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_SORTED_SET_LOG_NAME,
              "Passed sorted string list last twice(2)");

        // Insert middle again
        SortedString *middle =
          new SortedString(*(SortedString *)*(set.begin() + (set.size() / 2)));
        if(set.insert(middle))
        {
            Log::add(Log::ERROR, NEXTCASH_SORTED_SET_LOG_NAME,
              "Failed sorted string list middle twice(disabled)");
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_SORTED_SET_LOG_NAME,
              "Passed sorted string list middle twice(disabled)");

        set.insert(middle, true);
        item = set.find(*middle);

        if(middle->getString() != ((SortedString *)*item)->getString())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_SORTED_SET_LOG_NAME,
              "Failed sorted string list middle twice(1) : %s = %s", middle->getString().text(),
              ((SortedString *)*item)->getString().text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_SORTED_SET_LOG_NAME,
              "Passed sorted string list middle twice(1)");

        ++item;
        if(middle->getString() != ((SortedString *)*item)->getString())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_SORTED_SET_LOG_NAME,
              "Failed sorted string list middle twice(2) : %s = %s", middle->getString().text(),
              ((SortedString *)*item)->getString().text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_SORTED_SET_LOG_NAME,
              "Passed sorted string list middle twice(2)");

        return success;
    }
}
