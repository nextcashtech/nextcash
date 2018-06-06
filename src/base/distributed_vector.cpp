/**************************************************************************
 * Copyright 2017 NextCash, LLC                                           *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@nextcash.tech>                                  *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#include "distributed_vector.hpp"

#include "log.hpp"

#define NEXTCASH_DISTRIBUTED_VECTOR_LOG_NAME "Distributed Vector"


namespace NextCash
{
    bool testDistributedVector()
    {
        Log::add(Log::INFO, NEXTCASH_DISTRIBUTED_VECTOR_LOG_NAME,
          "------------- Starting Distributed Vector Tests -------------");

        bool success = true;

        /***********************************************************************************************
         * DistributedVector
         ***********************************************************************************************/
        DistributedVector<int> testVector(10);

        testVector.reserve(100);

        for(int i=5;i<=500;i+=5)
            testVector.push_back(i);

        /***********************************************************************************************
         * DistributedVector insert end of previous
         ***********************************************************************************************/
        int insertValue = 51;
        DistributedVector<int>::Iterator item;
        for(item=testVector.begin();item!=testVector.end();++item)
            if(*item > insertValue)
            {
                testVector.insert(item, insertValue);
                break;
            }

        item = testVector.begin() + 10;
        if(*item == 51)
            Log::addFormatted(Log::INFO, NEXTCASH_DISTRIBUTED_VECTOR_LOG_NAME, "Passed insert end of previous");
        else
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_DISTRIBUTED_VECTOR_LOG_NAME,
              "Failed insert end of previous : %d != 51", *item);
            success = false;
        }

        /***********************************************************************************************
         * DistributedVector insert in set
         ***********************************************************************************************/
        insertValue = 56;
        for(item=testVector.begin();item!=testVector.end();++item)
            if(*item > insertValue)
            {
                testVector.insert(item, insertValue);
                break;
            }

        item = testVector.begin() + 12;
        if(*item == 56)
            Log::addFormatted(Log::INFO, NEXTCASH_DISTRIBUTED_VECTOR_LOG_NAME, "Passed insert in set");
        else
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_DISTRIBUTED_VECTOR_LOG_NAME,
              "Failed insert in set : %d != 56", *item);
            success = false;
        }

        /***********************************************************************************************
         * DistributedVector push back end
         ***********************************************************************************************/
        insertValue = 501;
        testVector.push_back(insertValue);

        item = --testVector.end();
        if(*item == 501)
            Log::addFormatted(Log::INFO, NEXTCASH_DISTRIBUTED_VECTOR_LOG_NAME, "Passed push back end");
        else
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_DISTRIBUTED_VECTOR_LOG_NAME,
              "Failed push back end : %d != 501", *item);
            success = false;
        }

        /***********************************************************************************************
         * DistributedVector middle item
         ***********************************************************************************************/
        item = testVector.begin() + (testVector.size() / 2);
        if(*item == 250)
            Log::addFormatted(Log::INFO, NEXTCASH_DISTRIBUTED_VECTOR_LOG_NAME, "Passed middle item");
        else
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_DISTRIBUTED_VECTOR_LOG_NAME,
              "Failed middle item : %d != 250", *item);
            success = false;
        }

        /***********************************************************************************************
         * DistributedVector middle item from end
         ***********************************************************************************************/
        item = testVector.end() - (testVector.size() / 2);
        if(*item == 255)
            Log::addFormatted(Log::INFO, NEXTCASH_DISTRIBUTED_VECTOR_LOG_NAME, "Passed middle item from end");
        else
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_DISTRIBUTED_VECTOR_LOG_NAME,
              "Failed middle item from end : %d != 255", *item);
            success = false;
        }

        /***********************************************************************************************
         * DistributedVector 10 after begin
         ***********************************************************************************************/
        item = testVector.begin() + 10;
        if(*item == 51)
            Log::addFormatted(Log::INFO, NEXTCASH_DISTRIBUTED_VECTOR_LOG_NAME, "Passed 10 after begin");
        else
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_DISTRIBUTED_VECTOR_LOG_NAME,
              "Failed 10 after begin : %d != 51", *item);
            success = false;
        }

        /***********************************************************************************************
         * DistributedVector 11 after begin
         ***********************************************************************************************/
        item = testVector.begin() + 11;
        if(*item == 55)
            Log::addFormatted(Log::INFO, NEXTCASH_DISTRIBUTED_VECTOR_LOG_NAME, "Passed 11 after begin");
        else
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_DISTRIBUTED_VECTOR_LOG_NAME,
              "Failed 11 after begin : %d != 55", *item);
            success = false;
        }

        /***********************************************************************************************
         * DistributedVector 12 after begin
         ***********************************************************************************************/
        item = testVector.begin() + 12;
        if(*item == 56)
            Log::addFormatted(Log::INFO, NEXTCASH_DISTRIBUTED_VECTOR_LOG_NAME, "Passed 12 after begin");
        else
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_DISTRIBUTED_VECTOR_LOG_NAME,
              "Failed 12 after begin : %d != 56", *item);
            success = false;
        }

        /***********************************************************************************************
         * DistributedVector 11 before end
         ***********************************************************************************************/
        item = testVector.end() - 11;
        if(*item == 455)
            Log::addFormatted(Log::INFO, NEXTCASH_DISTRIBUTED_VECTOR_LOG_NAME, "Passed 11 before end");
        else
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_DISTRIBUTED_VECTOR_LOG_NAME,
              "Failed 11 before end : %d != 455", *item);
            success = false;
        }

        /***********************************************************************************************
         * DistributedVector 12 before end
         ***********************************************************************************************/
        item = testVector.end() - 12;
        if(*item == 450)
            Log::addFormatted(Log::INFO, NEXTCASH_DISTRIBUTED_VECTOR_LOG_NAME, "Passed 12 before end");
        else
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_DISTRIBUTED_VECTOR_LOG_NAME,
              "Failed 12 before end : %d != 450", *item);
            success = false;
        }

        // for(item=testVector.begin();item!=testVector.end();++item)
            // Log::addFormatted(Log::INFO, NEXTCASH_DISTRIBUTED_VECTOR_LOG_NAME, "Vector : %3d", *item);

        return success;
    }
}
