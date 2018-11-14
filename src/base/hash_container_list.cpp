/**************************************************************************
 * Copyright 2018 NextCash, LLC                                           *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@nextcash.tech>                                  *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#include "hash_container_list.hpp"

#include "log.hpp"
#include "digest.hpp"

#define NEXTCASH_HASH_CONTAINER_LIST_LOG_NAME "HashContainerList"


namespace NextCash
{
    bool stringEqual(String *&pLeft, String *&pRight)
    {
        return *pLeft == *pRight;
    }

    bool testHashContainerList()
    {
        Log::add(Log::INFO, NEXTCASH_HASH_CONTAINER_LIST_LOG_NAME,
          "------------- Starting Hash Container List Tests -------------");

        /***********************************************************************************************
         * Hash container list
         ***********************************************************************************************/
        bool success = true;
        HashContainerList<String *> hashStringList;
        HashContainerList<String *>::Iterator retrieve;

        Hash l1(32);
        l1.randomize();
        Hash l2(32);
        l2.randomize();

        String *string1 = new String("test1");
        String *string2 = new String("test2");

        hashStringList.insertIfNotMatching(l1, string1, stringEqual);

        retrieve = hashStringList.get(l1);
        if(retrieve == hashStringList.end())
        {
            Log::add(Log::ERROR, NEXTCASH_HASH_LOG_NAME,
              "Failed hash string list 0 : not found");
            success = false;
        }
        else if(*retrieve != string1)
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Failed hash string list 0 : %s",
              (*retrieve)->text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed hash string list 0");

        hashStringList.insertIfNotMatching(l2, string2, stringEqual);

        retrieve = hashStringList.get(l1);
        if(retrieve == hashStringList.end())
        {
            Log::add(Log::ERROR, NEXTCASH_HASH_LOG_NAME,
              "Failed hash string list 1 : not found");
            success = false;
        }
        else if(*retrieve != string1)
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Failed hash string list 1 : %s",
              (*retrieve)->text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed hash string list 1");

        retrieve = hashStringList.get(l2);
        if(retrieve == hashStringList.end())
        {
            Log::add(Log::ERROR, NEXTCASH_HASH_LOG_NAME,
              "Failed hash string list 2 : not found");
            success = false;
        }
        else if(*retrieve != string2)
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME, "Failed hash string list 2 : %s",
              (*retrieve)->text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed hash string list 2");

        Hash lr(32);
        String *newString = NULL;
        for(unsigned int i=0;i<100;++i)
        {
            lr.randomize();
            newString = new String();
            newString->writeFormatted("String %04d", Math::randomInt() % 1000);
            //hashStringList.insert(lr, newString);
            hashStringList.insertIfNotMatching(lr, newString, stringEqual);

            // for(HashContainerList<String *>::Iterator item=hashStringList.begin();item!=hashStringList.end();++item)
            // {
                // Log::addFormatted(Log::DEBUG, NEXTCASH_HASH_LOG_NAME, "Hash string list : %s <- %s",
                  // (*item)->text(), item.hash().hex().text());
            // }
        }

        retrieve = hashStringList.get(l1);
        if(retrieve == hashStringList.end())
        {
            Log::add(Log::ERROR, NEXTCASH_HASH_LOG_NAME,
              "Failed hash string list r1 : not found");
            success = false;
        }
        else if(*retrieve != string1)
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME,
              "Failed hash string list r1 : %s", (*retrieve)->text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed hash string list r1");

        retrieve = hashStringList.get(l2);
        if(retrieve == hashStringList.end())
        {
            Log::add(Log::ERROR, NEXTCASH_HASH_LOG_NAME,
              "Failed hash string list r2 : not found");
            success = false;
        }
        else if(*retrieve != string2)
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME,
              "Failed hash string list r2 : %s", (*retrieve)->text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed hash string list r2");

        String *firstString = new String("first");
        String *lastString = new String("last");

        l1.zeroize();
        hashStringList.insertIfNotMatching(l1, firstString, stringEqual);

        l1.setMax();
        hashStringList.insertIfNotMatching(l1, lastString, stringEqual);

        String *firstTest = NULL;
        String *lastTest = NULL;
        for(HashContainerList<String *>::Iterator item = hashStringList.begin();
          item != hashStringList.end(); ++item)
        {
            if(firstTest == NULL)
                firstTest = *item;
            lastTest = *item;
            // if(*item == NULL)
                // Log::addFormatted(Log::DEBUG, NEXTCASH_HASH_LOG_NAME, "Hash string list : NULL <- %s",
                  // item.hash().hex().text());
            // else
                // Log::addFormatted(Log::DEBUG, NEXTCASH_HASH_LOG_NAME, "Hash string list : %s <- %s",
                  // (*item)->text(), item.hash().hex().text());
        }

        if(firstTest == NULL)
        {
            Log::add(Log::ERROR, NEXTCASH_HASH_LOG_NAME,
              "Failed hash string list iterate first : not found");
            success = false;
        }
        else if(firstTest != firstString)
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME,
              "Failed hash string list iterate first : %s", firstTest->text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed hash string list iterate first");

        if(lastTest == NULL)
        {
            Log::add(Log::ERROR, NEXTCASH_HASH_LOG_NAME,
              "Failed hash string list iterate last : not found");
            success = false;
        }
        else if(lastTest != lastString)
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME,
              "Failed hash string list iterate last : %s", lastTest->text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed hash string list iterate last");

        l1.zeroize();
        retrieve = hashStringList.get(l1);
        if(retrieve == hashStringList.end())
        {
            Log::add(Log::ERROR, NEXTCASH_HASH_LOG_NAME,
              "Failed hash string list get first : not found");
            success = false;
        }
        else if(*retrieve != firstString)
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME,
              "Failed hash string list get first : %s", (*retrieve)->text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed hash string list get first");

        l1.setMax();
        retrieve = hashStringList.get(l1);
        if(retrieve == hashStringList.end())
        {
            Log::add(Log::ERROR, NEXTCASH_HASH_LOG_NAME,
              "Failed hash string list get last : not found");
            success = false;
        }
        else if(*retrieve != lastString)
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_LOG_NAME,
              "Failed hash string list get last : %s", (*retrieve)->text());
            success = false;
        }
        else
            Log::add(Log::INFO, NEXTCASH_HASH_LOG_NAME, "Passed hash string list get last");

        for(HashContainerList<String *>::Iterator item = hashStringList.begin();
          item != hashStringList.end(); ++item)
            if(*item != NULL)
                delete *item;

        return success;
    }
}

