/**************************************************************************
 * Copyright 2017-2018 NextCash, LLC                                      *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@nextcash.tech>                                  *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#include "hash_data_file_set.hpp"

#include "digest.hpp"


namespace NextCash
{
    bool HashDataFileSetObject::writeToDataFile(const Hash &pHash, OutputStream *pStream)
    {
        if(mDataOffset == INVALID_STREAM_SIZE)
        {
            // Not written to file yet
            pStream->setWriteOffset(pStream->length());
            mDataOffset = pStream->writeOffset();
            pHash.write(pStream);
            bool result = write(pStream);
            if(result)
                clearModified();
            setNew();
            return result;
        }

        if(!isModified())
            return true;

        if(pStream->writeOffset() != mDataOffset + pHash.size())
            pStream->setWriteOffset(mDataOffset + pHash.size());

        bool result = write(pStream);
        if(result)
            clearModified();
        return result;
    }

    bool HashDataFileSetObject::readFromDataFile(unsigned int pHashSize, InputStream *pStream)
    {
        // Hash has already been read from the file so set the file offset to the current location
        //   minus hash size
        mDataOffset = pStream->readOffset() - pHashSize;
        bool result = read(pStream);
        clearFlags();
        return result;
    }

    class TestHashData : public HashDataFileSetObject
    {
    public:

        TestHashData() { age = 0; }

        bool read(InputStream *pStream)
        {
            age = pStream->readInt();
            unsigned int length = pStream->readUnsignedInt();
            if(length == 0)
                value.clear();
            else
                value = pStream->readString(length);
            return true;
        }

        bool write(OutputStream *pStream)
        {
            pStream->writeInt(age);
            pStream->writeUnsignedInt(value.length());
            pStream->writeString(value.text());
            return true;
        }

        stream_size size() const
        {
            return value.length() + sizeof(char *) + 4 + 4;
        }

        int compareAge(HashDataFileSetObject *pRight)
        {
            if(age < ((TestHashData *)pRight)->age)
                return -1;
            if(age > ((TestHashData *)pRight)->age)
                return 1;
            return 0;
        }

        bool valuesMatch(const HashDataFileSetObject *pRight) const
        {
            return age == ((TestHashData *)pRight)->age &&
              value == ((TestHashData *)pRight)->value;
        }

        int age;
        String value;

    private:
        TestHashData(const TestHashData &pCopy);
        const TestHashData &operator = (const TestHashData &pRight);
    };

    bool testHashDataFileSet()
    {
        Log::add(Log::INFO, NEXTCASH_HASH_DATA_FILE_SET_LOG_NAME,
          "------------- Starting Hash Data File Set Tests -------------");

        bool success = true;
        Hash hash(32);
        TestHashData *data;
        Digest digest(Digest::SHA256);
        HashDataFileSet<TestHashData, 32, 64, 64>::Iterator found;
        bool checkSuccess;
        unsigned int markedOldCount = 0;
        unsigned int removedSize = 0;
        const unsigned int testSize = 5000;
        const unsigned int testSizeLarger = 7500;
        String dupValue, nonDupValue;

        NextCash::removeDirectory("test_hash_data_set");

        if(success)
        {
            HashDataFileSet<TestHashData, 32, 64, 64> hashDataSet("TestSet");

            hashDataSet.load("test_hash_data_set");
            hashDataSet.setTargetCacheDataSize(1000000);

            TestHashData *lowest = NULL, *highest = NULL;
            Hash lowestHash, highestHash;

            for(unsigned int i=0;i<testSize;++i)
            {
                // Create new value
                data = new TestHashData();
                data->age = i;
                data->value.writeFormatted("Value %d", i);

                // Calculate hash
                digest.initialize();
                data->write(&digest);
                digest.getResult(&hash);

                if(lowest == NULL || lowestHash > hash)
                {
                    lowestHash = hash;
                    lowest = data;
                }

                if(highest == NULL || highestHash < hash)
                {
                    highestHash = hash;
                    highest = data;
                }

                // Add to set
                hashDataSet.insert(hash, data);
            }

            // Create duplicate value
            data = new TestHashData();
            data->age = (testSize / 2) + 2;
            dupValue.writeFormatted("Value %d", data->age);
            nonDupValue.writeFormatted("Value %d", data->age);
            data->value.writeFormatted("Value %d", data->age);

            // Calculate hash
            digest.initialize();
            data->write(&digest);
            digest.getResult(&hash);

            data->value += " second";
            dupValue += " second";

            // Add to set
            hashDataSet.insert(hash, data);

            if(hashDataSet.size() == testSize + 1)
                Log::add(Log::INFO, NEXTCASH_HASH_DATA_FILE_SET_LOG_NAME, "Pass hash data set size");
            else
            {
                Log::addFormatted(Log::ERROR, NEXTCASH_HASH_DATA_FILE_SET_LOG_NAME,
                  "Failed hash data set size : %d != %d",
                  hashDataSet.size(), testSize + 1);
                success = false;
            }

            found = hashDataSet.get(lowestHash);
            if(!found)
            {
                Log::addFormatted(Log::ERROR, NEXTCASH_HASH_DATA_FILE_SET_LOG_NAME,
                  "Failed hash data set lowest : not found : %s",
                  lowestHash.hex().text());
                success = false;
            }
            else if(((TestHashData *)(*found))->value == lowest->value)
                Log::addFormatted(Log::INFO, NEXTCASH_HASH_DATA_FILE_SET_LOG_NAME,
                  "Pass hash data set lowest : %s - %s",
                  ((TestHashData *)(*found))->value.text(), found.hash().hex().text());
            else
            {
                Log::addFormatted(Log::ERROR, NEXTCASH_HASH_DATA_FILE_SET_LOG_NAME,
                  "Failed hash data set lowest : wrong entry : %s - %s",
                  ((TestHashData *)(*found))->value.text(), found.hash().hex().text());
                success = false;
            }

            found = hashDataSet.get(highestHash);
            if(!found)
            {
                Log::addFormatted(Log::ERROR, NEXTCASH_HASH_DATA_FILE_SET_LOG_NAME,
                  "Failed hash data set highest : not found : %s",
                  highestHash.hex().text());
                success = false;
            }
            else if(((TestHashData *)(*found))->value == highest->value)
                Log::addFormatted(Log::INFO, NEXTCASH_HASH_DATA_FILE_SET_LOG_NAME,
                  "Pass hash data set highest : %s - %s",
                  ((TestHashData *)(*found))->value.text(), found.hash().hex().text());
            else
            {
                Log::addFormatted(Log::ERROR, NEXTCASH_HASH_DATA_FILE_SET_LOG_NAME,
                  "Failed hash data set highest : wrong entry : %s - %s",
                  ((TestHashData *)(*found))->value.text(), found.hash().hex().text());
                success = false;
            }

            // Check duplicate values
            found = hashDataSet.get(hash);
            if(!found)
            {
                Log::add(Log::ERROR, NEXTCASH_HASH_DATA_FILE_SET_LOG_NAME,
                  "Failed hash data set duplicate : not found");
                success = false;
            }
            else
            {
                HashDataFileSetObject  *firstData = *found;
                if(found.hash() == hash)
                {
                    if(((TestHashData *)(*found))->value == dupValue ||
                      ((TestHashData *)(*found))->value == nonDupValue ||
                      ((TestHashData *)(*found))->value == data->value)
                        Log::addFormatted(Log::INFO, NEXTCASH_HASH_DATA_FILE_SET_LOG_NAME,
                          "Pass hash data set duplicate first : %s - %s",
                          ((TestHashData *)(*found))->value.text(), found.hash().hex().text());
                    else
                    {
                        Log::addFormatted(Log::ERROR, NEXTCASH_HASH_DATA_FILE_SET_LOG_NAME,
                          "Failed hash data set duplicate first : wrong entry : %s - %s",
                          ((TestHashData *)(*found))->value.text(), found.hash().hex().text());
                        success = false;
                    }
                }
                else
                {
                    Log::addFormatted(Log::ERROR, NEXTCASH_HASH_DATA_FILE_SET_LOG_NAME,
                      "Failed hash data set duplicate first : wrong hash",
                      found.hash().hex().text());
                    success = false;
                }

                ++found;
                if(*found != firstData)
                    Log::add(Log::INFO, NEXTCASH_HASH_DATA_FILE_SET_LOG_NAME,
                      "Pass hash data set duplicate second incremented");
                else
                {
                    Log::add(Log::ERROR, NEXTCASH_HASH_DATA_FILE_SET_LOG_NAME,
                      "Failed hash data set duplicate second not incremented");
                    success = false;
                }

                if(found.hash() == hash)
                {
                    if(((TestHashData *)(*found))->value == dupValue ||
                      ((TestHashData *)(*found))->value == nonDupValue ||
                      ((TestHashData *)(*found))->value == data->value)
                        Log::addFormatted(Log::INFO, NEXTCASH_HASH_DATA_FILE_SET_LOG_NAME,
                          "Pass hash data set duplicate second : %s - %s",
                          ((TestHashData *)(*found))->value.text(), found.hash().hex().text());
                    else
                    {
                        Log::addFormatted(Log::ERROR, NEXTCASH_HASH_DATA_FILE_SET_LOG_NAME,
                          "Failed hash data set duplicate second : wrong entry : %s - %s",
                          ((TestHashData *)(*found))->value.text(), found.hash().hex().text());
                        success = false;
                    }
                }
                else
                {
                    Log::addFormatted(Log::ERROR, NEXTCASH_HASH_DATA_FILE_SET_LOG_NAME,
                      "Failed hash data set duplicate second : wrong hash",
                      found.hash().hex().text());
                    success = false;
                }
            }

            if(!hashDataSet.saveMultiThreaded(4))
            {
                Log::addFormatted(Log::ERROR, NEXTCASH_HASH_DATA_FILE_SET_LOG_NAME,
                  "Failed hash data set multi-threaded save");
                success = false;
            }
        }

        if(success)
        {
            HashDataFileSet<TestHashData, 32, 64, 64> hashDataSet("TestSet");

            hashDataSet.load("test_hash_data_set");
            hashDataSet.setTargetCacheDataSize(1000000);

            if(hashDataSet.size() == testSize + 1)
                Log::addFormatted(Log::INFO, NEXTCASH_HASH_DATA_FILE_SET_LOG_NAME,
                  "Pass hash data set load size : %d", testSize + 1);
            else
            {
                Log::addFormatted(Log::ERROR, NEXTCASH_HASH_DATA_FILE_SET_LOG_NAME,
                  "Failed hash data set load size : %d != %d",
                  hashDataSet.size(), testSize + 1);
                success = false;
            }

            checkSuccess = true;
            for(unsigned int i=0;i<testSize;++i)
            {
                // Create new value
                data = new TestHashData();
                data->age = i;
                data->value.writeFormatted("Value %d", i);

                // Calculate hash
                digest.initialize();
                data->write(&digest);
                digest.getResult(&hash);

                found = hashDataSet.get(hash);
                if(!found)
                {
                    Log::addFormatted(Log::ERROR, NEXTCASH_HASH_DATA_FILE_SET_LOG_NAME,
                      "Failed hash data set load : %s not found", data->value.text());
                    checkSuccess = false;
                    success = false;
                }
                else
                {
                    if(found.hash() != hash)
                    {
                        Log::addFormatted(Log::ERROR, NEXTCASH_HASH_DATA_FILE_SET_LOG_NAME,
                          "Failed hash data set load : wrong hash : %s",
                          found.hash().hex().text());
                        checkSuccess = false;
                        success = false;
                    }
                    else if(((TestHashData *)(*found))->value != data->value &&
                      ((TestHashData *)(*found))->value != dupValue &&
                      ((TestHashData *)(*found))->value != nonDupValue)
                    {
                        Log::addFormatted(Log::ERROR, NEXTCASH_HASH_DATA_FILE_SET_LOG_NAME,
                          "Failed hash data set load : wrong value : %s - %s",
                          ((TestHashData *)(*found))->value.text(), found.hash().hex().text());
                        checkSuccess = false;
                        success = false;
                    }
                    // else
                        // Log::addFormatted(Log::INFO, NEXTCASH_HASH_DATA_FILE_SET_LOG_NAME,
                            // "Pass hash data set load : %s - %s",
                            // ((TestHashData *)(*found))->value.text(),
                            // found.hash().hex().text());
                }

                delete data;
            }

            if(checkSuccess)
                Log::addFormatted(Log::INFO, NEXTCASH_HASH_DATA_FILE_SET_LOG_NAME,
                  "Pass hash data set load check %d lookups", testSize);

            for(unsigned int i=testSize;i<testSizeLarger;++i)
            {
                // Create new value
                data = new TestHashData();
                data->age = i;
                data->value.writeFormatted("Value %d", i);

                // Calculate hash
                digest.initialize();
                data->write(&digest);
                digest.getResult(&hash);

                // Add to set
                hashDataSet.insert(hash, data);
            }

            checkSuccess = true;
            for(unsigned int i = testSize; i < testSizeLarger; ++i)
            {
                // Create new value
                data = new TestHashData();
                data->age = i;
                data->value.writeFormatted("Value %d", i);

                // Calculate hash
                digest.initialize();
                data->write(&digest);
                digest.getResult(&hash);

                found = hashDataSet.get(hash);
                if(!found)
                {
                    Log::addFormatted(Log::ERROR, NEXTCASH_HASH_DATA_FILE_SET_LOG_NAME,
                      "Failed hash data set load : %s not found",
                      data->value.text());
                    checkSuccess = false;
                    success = false;
                }
                else
                {
                    if(found.hash() != hash)
                    {
                        Log::addFormatted(Log::ERROR, NEXTCASH_HASH_DATA_FILE_SET_LOG_NAME,
                          "Failed hash data set load : wrong hash : %s",
                          found.hash().hex().text());
                        checkSuccess = false;
                        success = false;
                    }
                    else if(((TestHashData *)(*found))->value != data->value &&
                      ((TestHashData *)(*found))->value != dupValue &&
                      ((TestHashData *)(*found))->value != nonDupValue)
                    {
                        Log::addFormatted(Log::ERROR, NEXTCASH_HASH_DATA_FILE_SET_LOG_NAME,
                          "Failed hash data set load : wrong value : %s - %s",
                          ((TestHashData *)(*found))->value.text(), found.hash().hex().text());
                        checkSuccess = false;
                        success = false;
                    }
                    // else
                        // Log::addFormatted(Log::INFO, NEXTCASH_HASH_DATA_FILE_SET_LOG_NAME,
                            // "Pass hash data set load : %s - %s",
                            // ((TestHashData *)(*found))->value.text(),
                            // found.hash().hex().text());
                }

                delete data;
            }

            if(checkSuccess)
                Log::addFormatted(Log::INFO, NEXTCASH_HASH_DATA_FILE_SET_LOG_NAME,
                  "Pass hash data set check %d lookups", testSizeLarger);

            // Check removing items
            removedSize = hashDataSet.size();
            for(unsigned int i = 0; i < testSizeLarger; i += (testSize / 10))
            {
                // Create new value
                data = new TestHashData();
                data->age = i;
                data->value.writeFormatted("Value %d", i);

                // Calculate hash
                digest.initialize();
                data->write(&digest);
                digest.getResult(&hash);

                delete data;

                // Remove
                found = hashDataSet.get(hash);

                if(found)
                {
                    (*found)->setRemove();
                    --removedSize;
                    Log::addFormatted(Log::DEBUG, NEXTCASH_HASH_DATA_FILE_SET_LOG_NAME,
                      "Marked item for removal : %s", ((TestHashData *)(*found))->value.text());
                }
            }

            // Check marking items as old
            for(unsigned int i = 50; i < testSizeLarger; i += (testSize / 10))
            {
                // Create new value
                data = new TestHashData();
                data->age = i;
                data->value.writeFormatted("Value %d", i);

                // Calculate hash
                digest.initialize();
                data->write(&digest);
                digest.getResult(&hash);

                delete data;

                // Remove
                found = hashDataSet.get(hash);
                if(found)
                {
                    (*found)->setOld();
                    ++markedOldCount;
                    Log::addFormatted(Log::DEBUG, NEXTCASH_HASH_DATA_FILE_SET_LOG_NAME,
                      "Marked item as old : %s", ((TestHashData *)(*found))->value.text());
                }
            }

            // This applies the changes to the marked items
            hashDataSet.save();

            if(hashDataSet.size() == removedSize)
                Log::addFormatted(Log::INFO, NEXTCASH_HASH_DATA_FILE_SET_LOG_NAME,
                  "Pass hash data set remove size : %d", removedSize);
            else
            {
                Log::addFormatted(Log::ERROR, NEXTCASH_HASH_DATA_FILE_SET_LOG_NAME,
                  "Failed hash data set remove size : %d != %d", hashDataSet.size(), removedSize);
                success = false;
            }

            if(hashDataSet.cacheSize() == hashDataSet.size() - markedOldCount)
                Log::addFormatted(Log::INFO, NEXTCASH_HASH_DATA_FILE_SET_LOG_NAME,
                  "Pass hash data set old cache size : %d", hashDataSet.cacheSize());
            else
            {
                Log::addFormatted(Log::ERROR, NEXTCASH_HASH_DATA_FILE_SET_LOG_NAME,
                  "Failed hash data set old cache size : %d != %d",
                  hashDataSet.cacheSize(), hashDataSet.size() - markedOldCount);
                success = false;
            }
        }

        if(success)
        {
            HashDataFileSet<TestHashData, 32, 64, 64> hashDataSet("TestSet");

            hashDataSet.load("test_hash_data_set");

            // Set max cache data size to 75% of current size
            uint64_t cacheMaxSize = (uint64_t)((double)hashDataSet.cacheDataSize() * 0.75);
            hashDataSet.setTargetCacheDataSize(cacheMaxSize);

            // Force cache to trim
            hashDataSet.save();

            if(hashDataSet.size() == removedSize)
                Log::addFormatted(Log::INFO, NEXTCASH_HASH_DATA_FILE_SET_LOG_NAME,
                  "Pass hash data set trim size : %d",
                  removedSize);
            else
            {
                Log::addFormatted(Log::ERROR, NEXTCASH_HASH_DATA_FILE_SET_LOG_NAME,
                  "Failed hash data set trim size : %d != %d",
                  hashDataSet.size(), removedSize);
                success = false;
            }

            // Pruning isn't exact so allow a 10% over amount
            uint64_t bufferDataSize = (uint64_t)((double)cacheMaxSize * 1.1);

            if(hashDataSet.cacheDataSize() < bufferDataSize)
                Log::addFormatted(Log::INFO, NEXTCASH_HASH_DATA_FILE_SET_LOG_NAME,
                  "Pass hash data set trim cache data size : %d < %d",
                  hashDataSet.cacheDataSize(), bufferDataSize);
            else
            {
                Log::addFormatted(Log::ERROR, NEXTCASH_HASH_DATA_FILE_SET_LOG_NAME,
                  "Failed hash data set trim cache data size : %d >= %d",
                  hashDataSet.cacheDataSize(), bufferDataSize);
                success = false;
            }

            checkSuccess = true;
            for(unsigned int i = 0; i < testSize; ++i)
            {
                // Create new value
                data = new TestHashData();
                data->age = i;
                data->value.writeFormatted("Value %d", i);

                // Calculate hash
                digest.initialize();
                data->write(&digest);
                digest.getResult(&hash);

                found = hashDataSet.get(hash);

                if(i % (testSize / 10) == 0)
                {
                    if(found)
                    {
                        Log::addFormatted(Log::ERROR, NEXTCASH_HASH_DATA_FILE_SET_LOG_NAME,
                          "Failed hash data set after trim : %s not removed : %s",
                          data->value.text(), hash.hex().text());
                        checkSuccess = false;
                        success = false;
                    }
                }
                else
                {
                    if(!found)
                    {
                        Log::addFormatted(Log::ERROR, NEXTCASH_HASH_DATA_FILE_SET_LOG_NAME,
                          "Failed hash data set after trim : %s not found : %s",
                          data->value.text(), hash.hex().text());
                        checkSuccess = false;
                        success = false;
                    }
                    else
                    {
                        if(found.hash() != hash)
                        {
                            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_DATA_FILE_SET_LOG_NAME,
                              "Failed hash data set after trim : wrong hash : %s",
                              found.hash().hex().text());
                            checkSuccess = false;
                            success = false;
                        }
                        else if(((TestHashData *)(*found))->value != data->value &&
                          ((TestHashData *)(*found))->value != dupValue &&
                          ((TestHashData *)(*found))->value != nonDupValue)
                        {
                            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_DATA_FILE_SET_LOG_NAME,
                              "Failed hash data set load : wrong value : %s - %s",
                              ((TestHashData *)(*found))->value.text(), found.hash().hex().text());
                            checkSuccess = false;
                            success = false;
                        }
                    }
                }

                delete data;
            }

            if(checkSuccess)
                Log::addFormatted(Log::INFO, NEXTCASH_HASH_DATA_FILE_SET_LOG_NAME,
                  "Pass hash data set after trim check %d lookups", testSize);
        }

        return success;
    }
}
