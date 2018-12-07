/**************************************************************************
 * Copyright 2018 NextCash, LLC                                           *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@nextcash.tech>                                  *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#include "reference_sorted_set.hpp"


namespace NextCash
{

    bool testReferenceSortedSet()
    {
        Log::add(Log::INFO, NEXTCASH_SORTED_SET_LOG_NAME,
          "------------- Starting Reference Sorted Set Tests -------------");

        bool success = true;

        /******************************************************************************************
         * Sorted object set
         *****************************************************************************************/
        class SortedString
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

            bool operator == (SortedString &pRight)
            {
                return mString == pRight.getString() && mString == pRight.getString();
            }

            bool valueEquals(SortedString &pRight)
            {
                // Return false to test double inserts.
                return false; //mString == pRight.getString() && mString == pRight.getString();
            }

            int compare(SortedString &pRight)
            {
                int result = mString.compare(pRight.getString());
                if(result != 0)
                    return result;

                return mString.compare(pRight.getString());
            }

        };

        typedef ReferenceCounter<SortedString> SortedStringReference;
        ReferenceSortedSet<SortedString> set;

        SortedString *string1 = new SortedString("test1");
        SortedString *string2 = new SortedString("test2");
        SortedStringReference lookup;

        SortedStringReference string1Ref = string1;
        SortedStringReference string2Ref = string2;
        set.insert(string1Ref);

        lookup = set.get(string1Ref);
        if(!lookup)
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

        set.insert(string2Ref);

        lookup = set.get(string1Ref);
        if(!lookup)
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

        lookup = set.get(string2Ref);
        if(!lookup)
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
        SortedString *newSortedString;
        SortedStringReference newRef;
        for(unsigned int i = 0; i < 100; ++i)
        {
            newString.writeFormatted("String %04d", Math::randomInt() % 1000);
            newSortedString = new SortedString(newString);
            newRef = newSortedString;
            set.insert(newRef);

            // for(HashContainerList<String *>::Iterator item=set.begin();item!=set.end();++item)
            // {
                // Log::addFormatted(Log::DEBUG, NEXTCASH_SORTED_SET_LOG_NAME,
                  // "Hash string list : %s <- %s", (*item)->text(), item.hash().hex().text());
            // }
        }

        lookup = set.get(string1Ref);
        if(!lookup)
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

        lookup = set.get(string2Ref);
        if(!lookup)
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
        SortedStringReference firstRef(first);
        set.insert(firstRef);

        // Insert after last.
        SortedString *last = new SortedString("zString");
        SortedStringReference lastRef(last);
        set.insert(lastRef);

        if(first->getString() != (*set.begin())->getString())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_SORTED_SET_LOG_NAME,
              "Failed sorted string list first : %s = %s", first->getString().text(),
              (*set.begin())->getString().text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_SORTED_SET_LOG_NAME, "Passed sorted string list first");

        if(last->getString() != (*--set.end())->getString())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_SORTED_SET_LOG_NAME,
              "Failed sorted string list last : %s = %s", last->getString().text(),
              (*--set.end())->getString().text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_SORTED_SET_LOG_NAME, "Passed sorted string list last");

        // Insert first again
        first = new SortedString(*first);
        firstRef = first;
        set.insert(firstRef, true);
        ReferenceSortedSet<SortedString>::Iterator item = set.find(firstRef);

        if(first->getString() != (*item)->getString())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_SORTED_SET_LOG_NAME,
              "Failed sorted string list first twice(1) : %s = %s", first->getString().text(),
              (*item)->getString().text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_SORTED_SET_LOG_NAME,
              "Passed sorted string list first twice(1)");

        ++item;
        if(first->getString() != (*item)->getString())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_SORTED_SET_LOG_NAME,
              "Failed sorted string list first twice(2) : %s = %s", first->getString().text(),
              (*item)->getString().text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_SORTED_SET_LOG_NAME,
              "Passed sorted string list first twice(2)");

        // Insert last again
        last = new SortedString(*last);
        lastRef = last;
        set.insert(lastRef, true);
        item = set.find(lastRef);

        if(last->getString() != (*item)->getString())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_SORTED_SET_LOG_NAME,
              "Failed sorted string list last twice(1) : %s = %s", last->getString().text(),
              (*item)->getString().text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_SORTED_SET_LOG_NAME,
              "Passed sorted string list last twice(1)");

        ++item;
        if(last->getString() != (*item)->getString())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_SORTED_SET_LOG_NAME,
              "Failed sorted string list last twice(2) : %s = %s", last->getString().text(),
              (*item)->getString().text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_SORTED_SET_LOG_NAME,
              "Passed sorted string list last twice(2)");

        // Insert middle again
        SortedString *middle = new SortedString(**(set.begin() + (set.size() / 2)));
        SortedStringReference middleRef(middle);
        if(set.insert(middleRef))
        {
            Log::add(Log::ERROR, NEXTCASH_SORTED_SET_LOG_NAME,
              "Failed sorted string list middle twice(disabled)");
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_SORTED_SET_LOG_NAME,
              "Passed sorted string list middle twice(disabled)");

        middle = new SortedString(**(set.begin() + (set.size() / 2)));
        middleRef = middle;
        set.insert(middleRef, true);
        item = set.find(middleRef);

        if(middle->getString() != (*item)->getString())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_SORTED_SET_LOG_NAME,
              "Failed sorted string list middle twice(1) : %s = %s", middle->getString().text(),
              (*item)->getString().text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_SORTED_SET_LOG_NAME,
              "Passed sorted string list middle twice(1)");

        ++item;
        if(middle->getString() != (*item)->getString())
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_SORTED_SET_LOG_NAME,
              "Failed sorted string list middle twice(2) : %s = %s", middle->getString().text(),
              (*item)->getString().text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_SORTED_SET_LOG_NAME,
              "Passed sorted string list middle twice(2)");

        return success;
    }
}
