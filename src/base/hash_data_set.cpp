/**************************************************************************
 * Copyright 2017 ArcMist, LLC                                            *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@arcmist.com>                                    *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#include "hash_data_set.hpp"

#include "arcmist/crypto/digest.hpp"


namespace ArcMist
{
    bool HashData::writeToDataFile(const Hash &pHash, OutputStream *pStream)
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

    bool HashData::readFromDataFile(unsigned int pHashSize, InputStream *pStream)
    {
        // Hash has already been read from the file so set the file offset to the current location minus hash size
        mDataOffset = pStream->readOffset() - pHashSize;
        bool result = read(pStream);
        clearFlags();
        return result;
    }

    class TestHashData : public HashData
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

        uint64_t size()
        {
            return value.length() + sizeof(char *) + 4 + 4;
        }

        int compareAge(HashData *pRight)
        {
            if(age < ((TestHashData *)pRight)->age)
                return -1;
            if(age > ((TestHashData *)pRight)->age)
                return 1;
            return 0;
        }

        bool valuesMatch(const HashData *pRight) const
        {
            return age == ((TestHashData *)pRight)->age && value == ((TestHashData *)pRight)->value;
        }

        int age;
        String value;

    private:
        TestHashData(const TestHashData &pCopy);
        const TestHashData &operator = (const TestHashData &pRight);
    };

    bool testHashDataSet()
    {
        Log::add(Log::INFO, ARCMIST_HASH_DATA_SET_LOG_NAME, "------------- Starting Hash Data Set Tests -------------");

        bool success = true;
        Hash hash(32);
        Hash checkHash("00007c6fe7ce5654952ea3311e554a0e965bcf82ce33e0856c4952c2af8b03c0");
        TestHashData *data;
        Digest digest(Digest::SHA256);
        HashDataSet<TestHashData, 32, 64, 64>::Iterator found;
        bool checkSuccess;
        unsigned int markedOldCount = 0;
        unsigned int removedSize;

        ArcMist::removeDirectory("test_hash_data_set");

        if(success)
        {
            HashDataSet<TestHashData, 32, 64, 64> hashDataSet;

            hashDataSet.load("Test", "test_hash_data_set");
            hashDataSet.setTargetCacheDataSize(1000000);

            TestHashData *lowest = NULL, *highest = NULL;
            Hash lowestHash, highestHash;

            for(unsigned int i=0;i<5000;++i)
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

                if(checkHash == hash)
                    removedSize = 0;

                // Add to set
                hashDataSet.insert(hash, data);
            }

            // Create duplicate value
            data = new TestHashData();
            data->age = 501;
            data->value.writeFormatted("Value %d", 501);

            // Calculate hash
            digest.initialize();
            data->write(&digest);
            digest.getResult(&hash);

            data->value += " second";

            // Add to set
            hashDataSet.insert(hash, data);

            if(hashDataSet.size() == 5001)
                Log::add(Log::INFO, ARCMIST_HASH_DATA_SET_LOG_NAME, "Pass hash data set size");
            else
            {
                Log::addFormatted(Log::ERROR, ARCMIST_HASH_DATA_SET_LOG_NAME, "Failed hash data set size : %d != 5001",
                  hashDataSet.size());
                success = false;
            }

            found = hashDataSet.get(lowestHash);
            if(!found)
            {
                Log::addFormatted(Log::ERROR, ARCMIST_HASH_DATA_SET_LOG_NAME, "Failed hash data set lowest : not found : %s",
                  lowestHash.hex().text());
                success = false;
            }
            else if(((TestHashData *)(*found))->value == lowest->value)
                Log::addFormatted(Log::INFO, ARCMIST_HASH_DATA_SET_LOG_NAME, "Pass hash data set lowest : %s - %s",
                  ((TestHashData *)(*found))->value.text(), found.hash().hex().text());
            else
            {
                Log::add(Log::ERROR, ARCMIST_HASH_DATA_SET_LOG_NAME, "Failed hash data set lowest : wrong entry");
                success = false;
            }

            found = hashDataSet.get(highestHash);
            if(!found)
            {
                Log::addFormatted(Log::ERROR, ARCMIST_HASH_DATA_SET_LOG_NAME, "Failed hash data set highest : not found : %s",
                  highestHash.hex().text());
                success = false;
            }
            else if(((TestHashData *)(*found))->value == highest->value)
                Log::addFormatted(Log::INFO, ARCMIST_HASH_DATA_SET_LOG_NAME, "Pass hash data set highest : %s - %s",
                  ((TestHashData *)(*found))->value.text(), found.hash().hex().text());
            else
            {
                Log::add(Log::ERROR, ARCMIST_HASH_DATA_SET_LOG_NAME, "Failed hash data set highest : wrong entry");
                success = false;
            }

            // Check duplicate values
            found = hashDataSet.get(hash);
            if(!found)
            {
                Log::add(Log::ERROR, ARCMIST_HASH_DATA_SET_LOG_NAME, "Failed hash data set duplicate : not found");
                success = false;
            }
            else
            {
                HashData *firstData = *found;
                if(found.hash() == hash)
                    Log::addFormatted(Log::INFO, ARCMIST_HASH_DATA_SET_LOG_NAME, "Pass hash data set duplicate first : %s - %s",
                      ((TestHashData *)(*found))->value.text(), found.hash().hex().text());
                else
                {
                    Log::add(Log::ERROR, ARCMIST_HASH_DATA_SET_LOG_NAME, "Failed hash data set duplicate first : wrong hash");
                    success = false;
                }

                ++found;
                if(*found != firstData)
                    Log::add(Log::INFO, ARCMIST_HASH_DATA_SET_LOG_NAME, "Pass hash data set duplicate second incremented");
                else
                {
                    Log::add(Log::ERROR, ARCMIST_HASH_DATA_SET_LOG_NAME, "Failed hash data set duplicate second not incremented");
                    success = false;
                }

                if(found.hash() == hash)
                    Log::addFormatted(Log::INFO, ARCMIST_HASH_DATA_SET_LOG_NAME, "Pass hash data set duplicate second : %s - %s",
                      ((TestHashData *)(*found))->value.text(), found.hash().hex().text());
                else
                {
                    Log::add(Log::ERROR, ARCMIST_HASH_DATA_SET_LOG_NAME, "Failed hash data set duplicate second : wrong hash");
                    success = false;
                }
            }

            hashDataSet.save();
        }

        if(success)
        {
            HashDataSet<TestHashData, 32, 64, 64> hashDataSet;

            hashDataSet.load("Test", "test_hash_data_set");
            hashDataSet.setTargetCacheDataSize(1000000);

            if(hashDataSet.size() == 5001)
                Log::add(Log::INFO, ARCMIST_HASH_DATA_SET_LOG_NAME, "Pass hash data set load size");
            else
            {
                Log::addFormatted(Log::ERROR, ARCMIST_HASH_DATA_SET_LOG_NAME, "Failed hash data set load size : %d != 5001",
                  hashDataSet.size());
                success = false;
            }

            checkSuccess = true;
            for(unsigned int i=0;i<5000;++i)
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
                    Log::addFormatted(Log::ERROR, ARCMIST_HASH_DATA_SET_LOG_NAME, "Failed hash data set load : %s not found",
                      data->value.text());
                    checkSuccess = false;
                    success = false;
                }
                else
                {
                    if(found.hash() != hash)
                    {
                        Log::add(Log::ERROR, ARCMIST_HASH_DATA_SET_LOG_NAME, "Failed hash data set load : wrong hash");
                        checkSuccess = false;
                        success = false;
                    }
                    // else
                        // Log::addFormatted(Log::INFO, ARCMIST_HASH_DATA_SET_LOG_NAME, "Pass hash data set load : %s - %s",
                          // ((TestHashData *)(*found))->value.text(), found.hash().hex().text());
                }

                delete data;
            }

            if(checkSuccess)
                Log::add(Log::INFO, ARCMIST_HASH_DATA_SET_LOG_NAME, "Pass hash data set load check 5000 lookups");

            for(unsigned int i=5000;i<7500;++i)
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
            for(unsigned int i=5000;i<7500;++i)
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
                    Log::addFormatted(Log::ERROR, ARCMIST_HASH_DATA_SET_LOG_NAME, "Failed hash data set load : %s not found",
                      data->value.text());
                    checkSuccess = false;
                    success = false;
                }
                else
                {
                    if(found.hash() != hash)
                    {
                        Log::add(Log::ERROR, ARCMIST_HASH_DATA_SET_LOG_NAME, "Failed hash data set load : wrong hash");
                        checkSuccess = false;
                        success = false;
                    }
                    // else
                        // Log::addFormatted(Log::INFO, ARCMIST_HASH_DATA_SET_LOG_NAME, "Pass hash data set load : %s - %s",
                          // ((TestHashData *)(*found))->value.text(), found.hash().hex().text());
                }

                delete data;
            }

            if(checkSuccess)
                Log::add(Log::INFO, ARCMIST_HASH_DATA_SET_LOG_NAME, "Pass hash data set load check 7500 lookups");

            // Check removing items
            removedSize = hashDataSet.size();
            for(unsigned int i=0;i<7500;i+=100)
            {
                // Create new value
                data = new TestHashData();
                data->age = i;
                data->value.writeFormatted("Value %d", i);

                // Calculate hash
                digest.initialize();
                data->write(&digest);
                digest.getResult(&hash);

                // Remove
                found = hashDataSet.get(hash);

                if(found)
                {
                    (*found)->setRemove();
                    --removedSize;
                }
            }

            // Check marking items as old
            for(unsigned int i=50;i<7500;i+=100)
            {
                // Create new value
                data = new TestHashData();
                data->age = i;
                data->value.writeFormatted("Value %d", i);

                // Calculate hash
                digest.initialize();
                data->write(&digest);
                digest.getResult(&hash);

                // Remove
                found = hashDataSet.get(hash);
                if(found)
                {
                    (*found)->setOld();
                    ++markedOldCount;
                }
            }

            // This applies the changes to the marked items
            hashDataSet.save();

            if(hashDataSet.size() == removedSize)
                Log::addFormatted(Log::INFO, ARCMIST_HASH_DATA_SET_LOG_NAME, "Pass hash data set remove size : %d", removedSize);
            else
            {
                Log::addFormatted(Log::ERROR, ARCMIST_HASH_DATA_SET_LOG_NAME, "Failed hash data set remove size : %d != %d",
                  hashDataSet.size(), removedSize);
                success = false;
            }

            if(hashDataSet.cacheSize() == hashDataSet.size() - markedOldCount)
                Log::addFormatted(Log::INFO, ARCMIST_HASH_DATA_SET_LOG_NAME, "Pass hash data set old cache size : %d",
                  hashDataSet.cacheSize());
            else
            {
                Log::addFormatted(Log::ERROR, ARCMIST_HASH_DATA_SET_LOG_NAME, "Failed hash data set old cache size : %d != %d",
                  hashDataSet.cacheSize(), hashDataSet.size() - markedOldCount);
                success = false;
            }
        }

        if(success)
        {
            HashDataSet<TestHashData, 32, 64, 64> hashDataSet;

            hashDataSet.load("Test", "test_hash_data_set");

            // Set max cache data size to 75% of current size
            uint64_t cacheMaxSize = (uint64_t)((double)hashDataSet.cacheDataSize() * 0.75);
            hashDataSet.setTargetCacheDataSize(cacheMaxSize);

            // Force cache to prune
            hashDataSet.save();

            if(hashDataSet.size() == removedSize)
                Log::addFormatted(Log::INFO, ARCMIST_HASH_DATA_SET_LOG_NAME, "Pass hash data set prune size : %d",
                  removedSize);
            else
            {
                Log::addFormatted(Log::ERROR, ARCMIST_HASH_DATA_SET_LOG_NAME, "Failed hash data set prune size : %d != %d",
                  hashDataSet.size(), removedSize);
                success = false;
            }

            // Pruning isn't exact so allow a 10% over amount
            uint64_t bufferDataSize = (uint64_t)((double)cacheMaxSize * 1.1);

            if(hashDataSet.cacheDataSize() < bufferDataSize)
                Log::addFormatted(Log::INFO, ARCMIST_HASH_DATA_SET_LOG_NAME, "Pass hash data set prune cache data size : %d < %d",
                  hashDataSet.cacheDataSize(), bufferDataSize);
            else
            {
                Log::addFormatted(Log::ERROR, ARCMIST_HASH_DATA_SET_LOG_NAME, "Failed hash data set prune cache data size : %d >= %d",
                  hashDataSet.cacheDataSize(), bufferDataSize);
                success = false;
            }

            checkSuccess = true;
            for(unsigned int i=0;i<5000;++i)
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

                if(i % 100 == 0)
                {
                    if(found)
                    {
                        Log::addFormatted(Log::ERROR, ARCMIST_HASH_DATA_SET_LOG_NAME, "Failed hash data set after prune : %s not removed : %s",
                          data->value.text(), hash.hex().text());
                        checkSuccess = false;
                        success = false;
                        break;
                    }
                }
                else
                {
                    if(!found)
                    {
                        Log::addFormatted(Log::ERROR, ARCMIST_HASH_DATA_SET_LOG_NAME, "Failed hash data set after prune : %s not found : %s",
                          data->value.text(), hash.hex().text());
                        checkSuccess = false;
                        success = false;
                        break;
                    }
                    else
                    {
                        if(found.hash() != hash)
                        {
                            Log::add(Log::ERROR, ARCMIST_HASH_DATA_SET_LOG_NAME, "Failed hash data set after prune : wrong hash");
                            checkSuccess = false;
                            success = false;
                        }
                        // else
                            // Log::addFormatted(Log::INFO, ARCMIST_HASH_DATA_SET_LOG_NAME, "Pass hash data set load : %s - %s",
                              // ((TestHashData *)(*found))->value.text(), found.hash().hex().text());
                    }
                }

                delete data;
            }

            if(checkSuccess)
                Log::add(Log::INFO, ARCMIST_HASH_DATA_SET_LOG_NAME, "Pass hash data set after prune check 5000 lookups");
        }

        return success;
    }
}
