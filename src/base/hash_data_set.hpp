/**************************************************************************
 * Copyright 2017-2018 NextCash, LLC                                      *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@nextcash.tech>                                  *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#ifndef NEXTCASH_HASH_DATA_SET_HPP
#define NEXTCASH_HASH_DATA_SET_HPP

#include "mutex.hpp"
#include "thread.hpp"
#include "math.hpp"
#include "string.hpp"
#include "hash.hpp"
#include "distributed_vector.hpp"
#include "log.hpp"
#include "stream.hpp"
#include "file_stream.hpp"

#ifdef PROFILER_ON
#include "profiler.hpp"
#endif

#define NEXTCASH_HASH_DATA_SET_LOG_NAME "HashDataSet"


namespace NextCash
{
    // A data class that can be used within a HashDataSet.
    // NOTE: The objects read/write size can't increase after being initially added to the hash
    //   data set or it will overwrite the next item in the file.
    class HashData
    {
    public:

        HashData() { mFlags = 0; mDataOffset = INVALID_STREAM_SIZE; }
        virtual ~HashData() {}

        // Flags
        bool markedRemove() const { return mFlags & REMOVE_FLAG; }
        bool isModified() const { return mFlags & MODIFIED_FLAG; }
        bool isNew() const { return mFlags & NEW_FLAG; }
        bool isOld() const { return mFlags & OLD_FLAG; }

        void setRemove() { mFlags |= REMOVE_FLAG; }
        void setModified() { mFlags |= MODIFIED_FLAG; }
        void setNew() { mFlags |= NEW_FLAG; }
        void setOld() { mFlags |= OLD_FLAG; }

        void clearRemove() { mFlags &= ~REMOVE_FLAG; }
        void clearModified() { mFlags &= ~MODIFIED_FLAG; }
        void clearNew() { mFlags &= ~NEW_FLAG; }
        void clearOld() { mFlags &= ~OLD_FLAG; }
        void clearFlags() { mFlags = 0; }

        bool wasWritten() const { return mDataOffset != INVALID_STREAM_SIZE; }
        stream_size dataOffset() const { return mDataOffset; }
        void setDataOffset(stream_size pDataOffset) { mDataOffset = pDataOffset; }
        void clearDataOffset() { mDataOffset = INVALID_STREAM_SIZE; }

        // Reads object data from a stream
        virtual bool read(InputStream *pStream) = 0;

        // Writes object data to a stream
        virtual bool write(OutputStream *pStream) = 0;

        // Returns the size(bytes) in memory of the object
        virtual stream_size size() const = 0;

        // Evaluates the relative age of two objects.
        // Used to determine which objects to drop from cache
        // Negative means this object is older than pRight.
        // Zero means both objects are the same age.
        // Positive means this object is newer than pRight.
        virtual int compareAge(HashData *pRight) = 0;

        // Returns true if the value of this object matches the value pRight references
        virtual bool valuesMatch(const HashData *pRight) const = 0;

        bool readFromDataFile(unsigned int pHashSize, InputStream *pStream);
        bool writeToDataFile(const Hash &pHash, OutputStream *pStream);

    private:

        static const uint8_t NEW_FLAG           = 0x01; // Hasn't been added to the index yet
        static const uint8_t MODIFIED_FLAG      = 0x02; // Modified since last save
        static const uint8_t REMOVE_FLAG        = 0x04; // Needs removed from index and cache
        static const uint8_t OLD_FLAG           = 0x08; // Needs to be dropped from cache

        uint8_t mFlags;

        // The offset in the data file of the hash value, followed by the specific data for the
        //   virtual read/write functions.
        stream_size mDataOffset;

        HashData(const HashData &pCopy);
        const HashData &operator = (const HashData &pRight);
    };

    // Returns true if the values pointed to by both HashData pointers match
    inline bool hashDataValuesMatch(HashData *&pLeft, HashData *&pRight)
    {
        return pLeft->valuesMatch(pRight);
    }

    /* A data set that is looked up by a hash, is divided into subsets, and is stored in and used
     *   directly from files/streams.
     *
     * It implements a cache in memory for recently used items. And allows control of which items
     *   stay in the cache after a save.
     *
     * tHashDataType - A class that is a non abstract child class of HashData.
     * tHashSize - The number of bytes in the hashes used to reference data.
     * tSampleSize - The number of samples loaded in memory to reduce file reads during lookups.
     * tSetCount - The number of sets to break the data into. The more there are the faster lookups
     *   will be, but they add overhead.
     *
     * These are the file types.
     *   Data - A list of hashes with hash data immediately after each hash.
     *   Index - A list of stream_size objects that are offsets into the data file. They are sorted
     *     by the hash they reference.
     *   Cache - The same format as the data file, except it only contains items that should be in
     *     the cache.
     */
    template <class tHashDataType, uint8_t tHashSize, uint16_t tSampleSize, uint16_t tSetCount>
    class HashDataSet
    {
    public:
        class Iterator;
    private:

        // Returns the offset to the data subset. Must return a distributed number from zero to tSetCount - 1
        virtual unsigned int subSetOffset(const Hash &pLookupValue)
        {
            if(tSetCount <= 0x100)
                return pLookupValue.lookup8() % tSetCount;
            else if(tSetCount <= 0x10000)
                return pLookupValue.lookup16() % tSetCount;
        }

        class SampleEntry
        {
        public:
            Hash hash;
            stream_size offset;

            bool load(InputStream *pIndexFile, InputStream *pDataFile)
            {
                if(hash.isEmpty())
                {
                    pIndexFile->setReadOffset(offset);
                    stream_size dataOffset;
                    pIndexFile->read(&dataOffset, sizeof(stream_size));

                    pDataFile->setReadOffset(dataOffset);
                    if(!hash.read(pDataFile, tHashSize))
                    {
                        Log::addFormatted(Log::ERROR, NEXTCASH_HASH_DATA_SET_LOG_NAME,
                          "Failed to read sample index hash at offset %llu", offset);
                        return false;
                    }
                }
                return true;
            }
        };

        typedef typename HashContainerList<HashData *>::Iterator SubSetIterator;

        class SubSet
        {
        public:

            SubSet();
            ~SubSet();

            unsigned int id() const { return mID; }

            stream_size size() const { return mFileSize + mNewSize; }
            stream_size cacheSize() const { return mCache.size(); }
            static const stream_size staticCacheItemSize =
              Hash::memorySize(tHashSize) + // Hash in cache.
              sizeof(void *); // Data pointer in cache.
            stream_size cacheDataSize() // 12 = 8 byte memory pointer + 4 byte size count
              { return mCacheRawDataSize + (mCache.size() * staticCacheItemSize); }

            // Inserts a new item corresponding to the lookup.
            bool insert(const Hash &pLookupValue, HashData *pValue, bool pRejectMatching);

            bool removeIfMatching(const Hash &pLookupValue, HashData *pValue);

            // Returns an iterator referencing the first matching item.
            SubSetIterator get(const Hash &pLookupValue, bool pForcePull = false);
            HashData *getData(const Hash &pLookupValue, bool pForcePull = false);

            SubSetIterator end() { return mCache.end(); }

            // Pull all items with matching hashes from the file and put them in the cache.
            //   Returns true if any items were added to the cache.
            // If pPullMatchingFunction then only items that return true will be pulled.
            bool pull(const Hash &pLookupValue, HashData *pMatching = NULL);

            bool load(const char *pName, const char *pFilePath, unsigned int pID);
            bool save(const char *pName, uint64_t pMaxCacheDataSize);

            // Rewrite data file filling in gaps from removed data.
            bool defragment();

        private:

            bool pullHash(InputStream *pDataFile, stream_size pFileOffset, Hash &pHash)
            {
#ifdef PROFILER_ON
                ProfilerReference profiler(getProfiler(PROFILER_SET, PROFILER_HASH_SET_PULL_ID,
                  PROFILER_HASH_SET_PULL_NAME), true);
#endif
                if(!pDataFile->setReadOffset(pFileOffset))
                {
                    Log::addFormatted(Log::ERROR, NEXTCASH_HASH_DATA_SET_LOG_NAME,
                      "Failed to pull hash at index offset %d/%d", pFileOffset,
                      pDataFile->length());
                    return false;
                }

                if(!pHash.read(pDataFile, tHashSize))
                {
                    Log::addFormatted(Log::ERROR, NEXTCASH_HASH_DATA_SET_LOG_NAME,
                      "Failed to pull hash at index offset %d/%d", pFileOffset,
                      pDataFile->length());
                    return false;
                }
                return true;
            }

            void loadSamples(InputStream *pIndexFile);

            // Find offsets into indices that contain the specified hash, based on samples
            bool findSample(const Hash &pHash, InputStream *pIndexFile, InputStream *pDataFile,
              stream_size &pBegin, stream_size &pEnd);

            bool loadCache();
            bool saveCache();

            // Mark items in the cache as old until it is under the specified data size.
            // Only called by trimeCache.
            void markOld(stream_size pDataSize);

            // Remove items from cache based on "age" and data size specified.
            // Only called by save.
            bool trimCache(uint64_t pMaxCacheDataSize);

            MutexWithConstantName mLock;
            const char *mFilePath;
            stream_size mFileSize, mNewSize, mCacheRawDataSize;
            unsigned int mID;
            HashContainerList<HashData *> mCache;
            SampleEntry *mSamples;

        };

    protected:

        ReadersLock mLock;
        String mFilePath;
        String mName;
        SubSet mSubSets[tSetCount];
        stream_size mTargetCacheDataSize;
        bool mIsValid;

    public:

        HashDataSet(const char *pName) : mLock(String(pName) + "Lock")
          { mName = pName; mTargetCacheDataSize = 0; mIsValid = false; }
        ~HashDataSet() {}

        bool isValid() const { return mIsValid; }

        const String &name() const { return mName; }
        const String &path() const { return mFilePath; }

        // Iterators only allow iteration through the end of the current sub set. They are only
        //   designed to allow iterating through matching hashes.
        class Iterator
        {
        public:

            Iterator() { mSubSet = NULL; }
            Iterator(SubSet *pSubSet, SubSetIterator &pIterator)
            {
                mSubSet = pSubSet;
                mIterator = pIterator;
            }

            HashData *operator *() { return *mIterator; }
            HashData *operator ->() { return *mIterator; }

            const Hash &hash() const { return mIterator.hash(); }

            operator bool() const { return mSubSet != NULL && mIterator != mSubSet->end(); }
            bool operator !() const { return mSubSet == NULL || mIterator == mSubSet->end(); }

            bool operator ==(const Iterator &pRight) { return mIterator == pRight.mIterator; }
            bool operator !=(const Iterator &pRight) { return mIterator != pRight.mIterator; }

            Iterator &operator =(const Iterator &pRight)
            {
                mSubSet = pRight.mSubSet;
                mIterator = pRight.mIterator;
                return *this;
            }

            // Prefix increment
            Iterator &operator ++()
            {
                ++mIterator;
                return *this;
            }

            // Postfix increment
            Iterator operator ++(int)
            {
                Iterator result = *this;
                ++result;
                return result;
            }

            // Prefix decrement
            Iterator &operator --()
            {
                --mIterator;
                return *this;
            }

            // Postfix decrement
            Iterator operator --(int)
            {
                Iterator result = *this;
                --result;
                return result;
            }

        private:
            SubSet *mSubSet;
            SubSetIterator mIterator;
        };

        stream_size size() const
        {
            stream_size result = 0;
            const SubSet *subSet = mSubSets;
            for(unsigned int i = 0;i < tSetCount; ++i)
            {
                result += subSet->size();
                ++subSet;
            }
            return result;
        }

        stream_size cacheSize() const
        {
            stream_size result = 0;
            const SubSet *subSet = mSubSets;
            for(unsigned int i = 0; i < tSetCount; ++i)
            {
                result += subSet->cacheSize();
                ++subSet;
            }
            return result;
        }

        stream_size cacheDataSize()
        {
            stream_size result = 0;
            SubSet *subSet = mSubSets;
            for(unsigned int i = 0; i < tSetCount; ++i)
            {
                result += subSet->cacheDataSize();
                ++subSet;
            }
            return result;
        }

        // Set max cache data size in bytes
        stream_size targetCacheDataSize() const { return mTargetCacheDataSize; }
        void setTargetCacheDataSize(stream_size pSize) { mTargetCacheDataSize = pSize; }

        // Inserts a new item corresponding to the lookup.
        // Returns false if the pValue matches an existing value under the same hash according to
        //   the HashData::valuesMatch function.
        bool insert(const Hash &pLookupValue, HashData *pValue, bool pRejectMatching = false);

        bool removeIfMatching(const Hash &pLookupValue, HashData *pValue);

        // Returns an iterator referencing the first matching item.
        // Set pForcePull to true if you want to ensure you get all matching values even if they
        //   are no longer cached.
        Iterator get(const Hash &pLookupValue, bool pForcePull = false);
        HashData *getData(const Hash &pLookupValue, bool pForcePull = false);

        Iterator begin();
        Iterator end();

        virtual bool load(const char *pFilePath);
        virtual bool save();
        virtual bool saveMultiThreaded(unsigned int pThreadCount = 4);

        class SaveThreadData
        {
        public:

            SaveThreadData(const char *pName, SubSet *pFirstSubSet, stream_size pMaxSetCacheDataSize) :
              mutex("SaveThreadData")
            {
                name = pName;
                nextSubSet = pFirstSubSet;
                maxSetCacheDataSize = pMaxSetCacheDataSize;
                offset = 0;
                success = true;
                for(unsigned int i = 0; i < tSetCount; ++i)
                {
                    setComplete[i] = false;
                    setSuccess[i] = true;
                }
            }

            Mutex mutex;
            const char *name;
            SubSet *nextSubSet;
            stream_size maxSetCacheDataSize;
            unsigned int offset;
            bool success;
            bool setComplete[tSetCount];
            bool setSuccess[tSetCount];

            SubSet *getNext()
            {
                mutex.lock();
                SubSet *result = nextSubSet;
                if(nextSubSet != NULL)
                {
                    if(++offset == tSetCount)
                        nextSubSet = NULL;
                    else
                        ++nextSubSet;
                }
                mutex.unlock();
                return result;
            }

            void markComplete(unsigned int pOffset, bool pSuccess)
            {
                setComplete[pOffset] = true;
                setSuccess[pOffset] = pSuccess;
                if(!pSuccess)
                    success = false;
            }

        };

        static void saveThreadRun(); // Thread to process save tasks

        //TODO void defragment(); // Re-sort and rewrite data file using index file.
    };

    // template <class tHashDataType, uint8_t tHashSize, uint16_t tSampleSize, uint16_t tSetCount>
    // static unsigned int HashDataSet<tSampleSize, 256>::subSetOffset(const Hash &pLookupValue)
      // { return pLookupValue.lookup8(); }

    // template <class tHashDataType, uint8_t tHashSize, uint16_t tSampleSize, uint16_t tSetCount>
    // static unsigned int HashDataSet<tSampleSize, 2048>::subSetOffset(const Hash &pLookupValue)
      // { return pLookupValue.lookup16() >> 5; }

    // template <class tHashDataType, uint8_t tHashSize, uint16_t tSampleSize, uint16_t tSetCount>
    // HashDataSet<tHashDataType, tHashSize, tSampleSize, tSetCount>::HashDataSet() : mLock("HashDataSet")
    // {

    // }

    // template <class tHashDataType, uint8_t tHashSize, uint16_t tSampleSize, uint16_t tSetCount>
    // HashDataSet<tHashDataType, tHashSize, tSampleSize, tSetCount>::~HashDataSet()
    // {

    // }

    template <class tHashDataType, uint8_t tHashSize, uint16_t tSampleSize, uint16_t tSetCount>
    bool HashDataSet<tHashDataType, tHashSize, tSampleSize, tSetCount>::insert(const Hash &pLookupValue,
      HashData *pValue, bool pRejectMatching)
    {
#ifdef PROFILER_ON
        ProfilerReference profiler(getProfiler(PROFILER_SET, PROFILER_HASH_SET_INSERT_ID,
          PROFILER_HASH_SET_INSERT_NAME), true);
#endif
        mLock.writeLock("Insert");
        bool result = mSubSets[subSetOffset(pLookupValue)].insert(pLookupValue, pValue,
          pRejectMatching);
        mLock.writeUnlock();
        return result;
    }

    template <class tHashDataType, uint8_t tHashSize, uint16_t tSampleSize, uint16_t tSetCount>
    bool HashDataSet<tHashDataType, tHashSize, tSampleSize, tSetCount>::removeIfMatching(const Hash &pLookupValue,
      HashData *pValue)
    {
        mLock.writeLock("Remove");
        bool result = mSubSets[subSetOffset(pLookupValue)].removeIfMatching(pLookupValue, pValue);
        mLock.writeUnlock();
        return result;
    }

    template <class tHashDataType, uint8_t tHashSize, uint16_t tSampleSize, uint16_t tSetCount>
    typename HashDataSet<tHashDataType, tHashSize, tSampleSize, tSetCount>::Iterator
      HashDataSet<tHashDataType, tHashSize, tSampleSize, tSetCount>::get(const Hash &pLookupValue,
      bool pForcePull)
    {
        mLock.readLock();
        SubSet *subSet = mSubSets + subSetOffset(pLookupValue);
        SubSetIterator result = subSet->get(pLookupValue, pForcePull);
        mLock.readUnlock();
        return Iterator(subSet, result);
    }

    template <class tHashDataType, uint8_t tHashSize, uint16_t tSampleSize, uint16_t tSetCount>
    HashData *HashDataSet<tHashDataType, tHashSize, tSampleSize, tSetCount>::getData(const Hash &pLookupValue,
      bool pForcePull)
    {
        mLock.readLock();
        SubSet *subSet = mSubSets + subSetOffset(pLookupValue);
        HashData *result = subSet->getData(pLookupValue, pForcePull);
        mLock.readUnlock();
        return result;
    }

    template <class tHashDataType, uint8_t tHashSize, uint16_t tSampleSize, uint16_t tSetCount>
    bool HashDataSet<tHashDataType, tHashSize, tSampleSize, tSetCount>::load(const char *pFilePath)
    {
        mLock.writeLock("Load");
        mIsValid = true;
        mFilePath = pFilePath;
        if(!createDirectory(mFilePath))
        {
            Log::addFormatted(Log::ERROR, mName.text(), "Failed to create directory : %s",
              mFilePath.text());
            mIsValid = false;
            mLock.writeUnlock();
            return false;
        }
        SubSet *subSet = mSubSets;
        uint32_t lastReport = getTime();
        for(unsigned int i = 0; i < tSetCount; ++i)
        {
            if(getTime() - lastReport > 10)
            {
                Log::addFormatted(Log::INFO, mName.text(), "Load is %2d%% Complete",
                  (int)(((float)i / (float)tSetCount) * 100.0f));
                lastReport = getTime();
            }
            if(!subSet->load(mName.text(), mFilePath, i))
                mIsValid = false;
            ++subSet;
        }
        mLock.writeUnlock();
        return true;
    }

    template <class tHashDataType, uint8_t tHashSize, uint16_t tSampleSize, uint16_t tSetCount>
    bool HashDataSet<tHashDataType, tHashSize, tSampleSize, tSetCount>::save()
    {
        mLock.writeLock("Save");

        if(!mIsValid)
        {
            Log::add(Log::ERROR, mName.text(), "Can't save invalid data set");
            mLock.writeUnlock();
            return false;
        }

        SubSet *subSet = mSubSets;
        uint32_t lastReport = getTime();
        uint64_t maxSetCacheDataSize = 0;
        bool success = true;
        if(mTargetCacheDataSize > 0)
            maxSetCacheDataSize = mTargetCacheDataSize / tSetCount;
        for(unsigned int i = 0; i < tSetCount; ++i)
        {
            if(getTime() - lastReport > 10)
            {
                Log::addFormatted(Log::INFO, mName.text(), "Save is %2d%% Complete",
                  (int)(((float)i / (float)tSetCount) * 100.0f));
                lastReport = getTime();
            }

            if(!subSet->save(mName.text(), maxSetCacheDataSize))
            {
                Log::addFormatted(Log::WARNING, mName.text(), "Failed set %d save",
                  subSet->id());
                success = false;
            }

            ++subSet;
        }
        mLock.writeUnlock();
        return success;
    }

    template <class tHashDataType, uint8_t tHashSize, uint16_t tSampleSize, uint16_t tSetCount>
    void HashDataSet<tHashDataType, tHashSize, tSampleSize, tSetCount>::saveThreadRun()
    {
        SaveThreadData *data = (SaveThreadData *)Thread::getParameter();
        if(data == NULL)
        {
            Log::add(Log::WARNING, data->name, "Thread parameter is null. Stopping");
            return;
        }

        SubSet *subSet;
        while(true)
        {
            subSet = data->getNext();
            if(subSet == NULL)
            {
                Log::add(Log::DEBUG, data->name, "No more save tasks remaining");
                break;
            }

            if(subSet->save(data->name, data->maxSetCacheDataSize))
                data->markComplete(subSet->id(), true);
            else
            {
                Log::addFormatted(Log::WARNING, data->name, "Failed save of set %d", subSet->id());
                data->markComplete(subSet->id(), false);
            }
        }
    }

    template <class tHashDataType, uint8_t tHashSize, uint16_t tSampleSize, uint16_t tSetCount>
    bool HashDataSet<tHashDataType, tHashSize, tSampleSize, tSetCount>::saveMultiThreaded(unsigned int pThreadCount)
    {
        mLock.writeLock("Save");

        if(!mIsValid)
        {
            Log::add(Log::ERROR, mName.text(), "Can't save invalid data set");
            mLock.writeUnlock();
            return false;
        }

        uint64_t maxSetCacheDataSize = 0;
        if(mTargetCacheDataSize > 0)
            maxSetCacheDataSize = mTargetCacheDataSize / tSetCount;
        SaveThreadData threadData(mName.text(), mSubSets, maxSetCacheDataSize);
        Thread *threads[pThreadCount];
        int32_t lastReport = getTime();
        unsigned int i;
        String threadName;

        // Start threads
        for(i = 0; i < pThreadCount; ++i)
        {
            threadName.writeFormatted("%s Save %d", mName.text(), i);
            threads[i] = new Thread(threadName, saveThreadRun, &threadData);
        }

        // Monitor threads
        unsigned int completedCount;
        bool report;
        while(true)
        {
            if(threadData.offset == tSetCount)
            {
                report = getTime() - lastReport > 10;
                completedCount = 0;
                for(i = 0; i < tSetCount; ++i)
                    if(threadData.setComplete[i])
                        ++completedCount;
                    else if(report)
                        Log::addFormatted(Log::INFO, mName.text(), "Save waiting for set %d", i);

                if(report)
                    lastReport = getTime();

                if(completedCount == tSetCount)
                    break;
            }
            else if(getTime() - lastReport > 10)
            {
                completedCount = 0;
                for(i = 0; i < tSetCount; ++i)
                    if(threadData.setComplete[i])
                        ++completedCount;

                Log::addFormatted(Log::INFO, mName.text(), "Save is %2d%% Complete",
                  (int)(((float)completedCount / (float)tSetCount) * 100.0f));

                lastReport = getTime();
            }

            Thread::sleep(500);
        }

        // Delete threads
        Log::add(Log::DEBUG, mName.text(), "Deleting save threads");
        for(i = 0; i < pThreadCount; ++i)
            delete threads[i];

        mLock.writeUnlock();
        return threadData.success;
    }

    template <class tHashDataType, uint8_t tHashSize, uint16_t tSampleSize, uint16_t tSetCount>
    HashDataSet<tHashDataType, tHashSize, tSampleSize, tSetCount>::SubSet::SubSet() :
      mLock("HashDataSubSet")
    {
        mSamples = NULL;
        mFileSize = 0;
        mNewSize = 0;
        mCacheRawDataSize = 0;
    }

    template <class tHashDataType, uint8_t tHashSize, uint16_t tSampleSize, uint16_t tSetCount>
    HashDataSet<tHashDataType, tHashSize, tSampleSize, tSetCount>::SubSet::~SubSet()
    {
        for(HashContainerList<HashData *>::Iterator item=mCache.begin();item!=mCache.end();++item)
            delete *item;
        if(mSamples != NULL)
            delete[] mSamples;
    }

    template <class tHashDataType, uint8_t tHashSize, uint16_t tSampleSize, uint16_t tSetCount>
    bool HashDataSet<tHashDataType, tHashSize, tSampleSize, tSetCount>::SubSet::insert(
      const Hash &pLookupValue, HashData *pValue, bool pRejectMatching)
    {
        bool result = false;
        mLock.lock();
        if(pRejectMatching)
        {
            if(mCache.insertIfNotMatching(pLookupValue, pValue, hashDataValuesMatch))
            {
                ++mNewSize;
                mCacheRawDataSize += pValue->size();
                pValue->clearDataOffset();
                pValue->setNew();
                result = true;
            }
        }
        else
        {
            mCache.insert(pLookupValue, pValue);
            ++mNewSize;
            mCacheRawDataSize += pValue->size();
            pValue->clearDataOffset();
            pValue->setNew();
            result = true;
        }
        mLock.unlock();
        return result;
    }

    template <class tHashDataType, uint8_t tHashSize, uint16_t tSampleSize, uint16_t tSetCount>
    bool HashDataSet<tHashDataType, tHashSize, tSampleSize, tSetCount>::SubSet::removeIfMatching(
      const Hash &pLookupValue, HashData *pValue)
    {
        mLock.lock();

        bool result = false;
        SubSetIterator item = mCache.get(pLookupValue);
        while(item != mCache.end() && item.hash() == pLookupValue)
        {
            if(pValue->valuesMatch(*item) && !(*item)->markedRemove())
            {
                (*item)->setRemove();
                result = true;
            }
            ++item;
        }

        // Pull only matching items
        if(!result && pull(pLookupValue, pValue))
        {
            item = mCache.get(pLookupValue);
            while(item != mCache.end() && item.hash() == pLookupValue)
            {
                if(pValue->valuesMatch(*item) && !(*item)->markedRemove())
                {
                    (*item)->setRemove();
                    result = true;
                }
                ++item;
            }
        }

        mLock.unlock();
        return result;
    }

    template <class tHashDataType, uint8_t tHashSize, uint16_t tSampleSize, uint16_t tSetCount>
    typename HashDataSet<tHashDataType, tHashSize, tSampleSize, tSetCount>::SubSetIterator
      HashDataSet<tHashDataType, tHashSize, tSampleSize, tSetCount>::SubSet::get(
      const Hash &pLookupValue, bool pForcePull)
    {
        mLock.lock();

        if(pForcePull)
            pull(pLookupValue);

        SubSetIterator result = mCache.get(pLookupValue);
        if(result == mCache.end() && !pForcePull && pull(pLookupValue))
            result = mCache.get(pLookupValue);

        mLock.unlock();
        return result;
    }

    template <class tHashDataType, uint8_t tHashSize, uint16_t tSampleSize, uint16_t tSetCount>
    HashData *HashDataSet<tHashDataType, tHashSize, tSampleSize, tSetCount>::SubSet::getData(
      const Hash &pLookupValue, bool pForcePull)
    {
        mLock.lock();

        HashData *result = NULL;

        if(pForcePull)
            pull(pLookupValue);

        SubSetIterator item = mCache.get(pLookupValue);
        if(item == mCache.end() && !pForcePull && pull(pLookupValue))
            item = mCache.get(pLookupValue);

        while(item != mCache.end() && item.hash() == pLookupValue)
        {
            if(!(*item)->markedRemove())
            {
                result = *item;
                break;
            }

            ++item;
        }

        mLock.unlock();
        return result;
    }

    template <class tHashDataType, uint8_t tHashSize, uint16_t tSampleSize, uint16_t tSetCount>
    bool HashDataSet<tHashDataType, tHashSize, tSampleSize, tSetCount>::SubSet::pull(
      const Hash &pLookupValue, HashData *pMatching)
    {
        if(mFileSize == 0)
            return false;

        int compare;
        stream_size dataOffset;
        Hash hash(tHashSize);
        stream_size first = 0, last = (mFileSize - 1) * sizeof(stream_size), begin, end, current;
        String filePathName;
        filePathName.writeFormatted("%s%s%04x.index", mFilePath, PATH_SEPARATOR, mID);
        FileInputStream indexFile(filePathName);
        filePathName.writeFormatted("%s%s%04x.data", mFilePath, PATH_SEPARATOR, mID);
        FileInputStream dataFile(filePathName);

        if(!indexFile.isValid())
        {
            Log::add(Log::ERROR, NEXTCASH_HASH_DATA_SET_LOG_NAME,
              "Failed to open index file in pull");
            return false;
        }

        if(!dataFile.isValid())
        {
            Log::add(Log::ERROR, NEXTCASH_HASH_DATA_SET_LOG_NAME,
              "Failed to open index file in pull");
            return false;
        }

        if(mSamples != NULL)
        {
            if(!findSample(pLookupValue, &indexFile, &dataFile, begin, end))
                return false; // Failed

            if(begin == INVALID_STREAM_SIZE)
                return false; // Not within subset
        }
        else // Not enough items for a full sample set
        {
            // Setup index binary search on all indices
            begin = first;
            end = last;

            // Check first item
            indexFile.setReadOffset(begin);
            indexFile.read(&dataOffset, sizeof(stream_size));
            dataFile.setReadOffset(dataOffset);
            if(!hash.read(&dataFile))
                return false;

            compare = pLookupValue.compare(hash);
            if(compare < 0)
                return false; // Lookup is before first item
            else if(compare == 0)
                end = begin;
            else if(mFileSize > 1)
            {
                // Check last item
                indexFile.setReadOffset(end);
                indexFile.read(&dataOffset, sizeof(stream_size));
                dataFile.setReadOffset(dataOffset);
                if(!hash.read(&dataFile))
                    return false;

                compare = pLookupValue.compare(hash);
                if(compare > 0)
                    return false; // Lookup is after last item
                else if(compare == 0)
                    begin = end;
            }
            else
                return false; // Not within subset
        }

        if(begin == end)
            current = begin; // Lookup matches a sample
        else
        {
            // Binary search the file indices
            while(true)
            {
                // Break the set in two halves (set current to the middle)
                current = (end - begin) / 2;
                current -= current % sizeof(stream_size);
                if(current == 0) // Begin and end are next to each other and have already been checked
                    return false;
                current += begin;

                // Read the middle item
                indexFile.setReadOffset(current);
                indexFile.read(&dataOffset, sizeof(stream_size));
                dataFile.setReadOffset(dataOffset);
                if(!hash.read(&dataFile))
                    return false;

                // Determine which half the desired item is in
                compare = pLookupValue.compare(hash);
                if(compare > 0)
                    begin = current;
                else if(compare < 0)
                    end = current;
                else
                    break;
            }
        }

        // Match likely found
        // Loop backwards to find the first matching
        while(current > first)
        {
            current -= sizeof(stream_size);
            indexFile.setReadOffset(current);
            indexFile.read(&dataOffset, sizeof(stream_size));
            dataFile.setReadOffset(dataOffset);
            if(!hash.read(&dataFile))
                return false;

            if(pLookupValue != hash)
            {
                current += sizeof(stream_size);
                break;
            }
        }

        // Read in all matching
        bool result = false;
        HashData *next;
        while(current <= last)
        {
            indexFile.setReadOffset(current);
            indexFile.read(&dataOffset, sizeof(stream_size));
            dataFile.setReadOffset(dataOffset);
            if(!hash.read(&dataFile))
                return result;

            if(pLookupValue != hash)
                break;

            next = new tHashDataType();
            if(!next->readFromDataFile(tHashSize, &dataFile))
            {
                delete next;
                break;
            }

            if((pMatching == NULL || pMatching->valuesMatch(next)) &&
              mCache.insertIfNotMatching(pLookupValue, next, hashDataValuesMatch))
            {
                mCacheRawDataSize += next->size();
                result = true;
            }
            else
                delete next;

            current += sizeof(stream_size);
        }

        return result;
    }

    template <class tHashDataType, uint8_t tHashSize, uint16_t tSampleSize, uint16_t tSetCount>
    void HashDataSet<tHashDataType, tHashSize, tSampleSize, tSetCount>::SubSet::loadSamples(InputStream *pIndexFile)
    {
        if(mSamples != NULL)
            delete[] mSamples;
        mSamples = NULL;

        stream_size delta = mFileSize / tSampleSize;
        if(delta < 4)
            return;

        mSamples = new SampleEntry[tSampleSize];

        // Load samples
        stream_size offset = 0;
        SampleEntry *sample = mSamples;
        for(unsigned int i=0;i<tSampleSize-1;++i)
        {
            sample->hash.clear();
            sample->offset = offset;
            offset += (delta * sizeof(stream_size));
            ++sample;
        }

        // Load last sample
        sample->hash.clear();
        sample->offset = pIndexFile->length() - sizeof(stream_size);
    }

    template <class tHashDataType, uint8_t tHashSize, uint16_t tSampleSize, uint16_t tSetCount>
    bool HashDataSet<tHashDataType, tHashSize, tSampleSize, tSetCount>::SubSet::findSample(const Hash &pHash,
      InputStream *pIndexFile, InputStream *pDataFile, stream_size &pBegin, stream_size &pEnd)
    {
        // Check first entry
        SampleEntry *sample = mSamples;
        if(!sample->load(pIndexFile, pDataFile))
            return false;
        int compare = sample->hash.compare(pHash);
        if(compare > 0)
        {
            // Hash is before the first entry of subset
            pBegin = INVALID_STREAM_SIZE;
            pEnd   = INVALID_STREAM_SIZE;
            return true;
        }
        else if(compare == 0)
        {
            // Hash is the first entry of subset
            pBegin = sample->offset;
            pEnd   = sample->offset;
            return true;
        }
        // Log::addFormatted(Log::VERBOSE, NEXTCASH_HASH_DATA_SET_LOG_NAME,
          // "First : %s", mSamples[0].hash.hex().text());

        // Check last entry
        sample = mSamples + (tSampleSize - 1);
        if(!sample->load(pIndexFile, pDataFile))
            return false;
        compare = sample->hash.compare(pHash);
        if(compare < 0)
        {
            // Hash is after the last entry of subset
            pBegin = INVALID_STREAM_SIZE;
            pEnd   = INVALID_STREAM_SIZE;
            return true;
        }
        else if(compare == 0)
        {
            // Hash is the after last entry of subset
            pBegin = sample->offset;
            pEnd   = sample->offset;
            return true;
        }
        // Log::addFormatted(Log::VERBOSE, NEXTCASH_HASH_DATA_SET_LOG_NAME,
          // "Last : %s", mSamples[tSampleSize - 1].hash.hex().text());

        // Binary search the samples
        unsigned int sampleBegin = 0;
        unsigned int sampleEnd = tSampleSize - 1;
        unsigned int sampleCurrent;
        bool done = false;

        while(!done)
        {
            sampleCurrent = (sampleBegin + sampleEnd) / 2;
            // Log::addFormatted(Log::VERBOSE, NEXTCASH_HASH_DATA_SET_LOG_NAME,
              // "Sample : %s", mSamples[sampleCurrent].hash.hex().text());

            if(sampleCurrent == sampleBegin || sampleCurrent == sampleEnd)
                done = true;

            sample = mSamples + sampleCurrent;
            if(!sample->load(pIndexFile, pDataFile))
                return false;

            // Determine which half the desired item is in
            compare = sample->hash.compare(pHash);
            if(compare < 0)
                sampleBegin = sampleCurrent;
            else if(compare > 0)
                sampleEnd = sampleCurrent;
            else
            {
                sampleBegin = sampleCurrent;
                sampleEnd = sampleCurrent;
                break;
            }
        }

        // Setup index binary search on sample subset of indices
        pBegin = mSamples[sampleBegin].offset;
        pEnd = mSamples[sampleEnd].offset;
        return true;
    }

    template <class tHashDataType, uint8_t tHashSize, uint16_t tSampleSize, uint16_t tSetCount>
    bool HashDataSet<tHashDataType, tHashSize, tSampleSize, tSetCount>::SubSet::loadCache()
    {
        for(HashContainerList<HashData *>::Iterator item = mCache.begin();
          item != mCache.end(); ++item)
            delete *item;
        mCache.clear();
        mCacheRawDataSize = 0;

        // Open cache file
        String filePathName;
        filePathName.writeFormatted("%s%s%04x.cache", mFilePath, PATH_SEPARATOR, mID);
        FileInputStream *cacheFile = new FileInputStream(filePathName);
        HashData *next;
        Hash hash(tHashSize);

        if(!cacheFile->isValid())
        {
            delete cacheFile;
            return false;
        }

        bool success = true;
        stream_size dataOffset;
        cacheFile->setReadOffset(0);
        while(cacheFile->remaining())
        {
            // Read data offset from cache file
            cacheFile->read(&dataOffset, sizeof(stream_size));

            // Read hash from cache file
            if(!hash.read(cacheFile))
            {
                success = false;
                break;
            }

            // Read data from cache file
            next = new tHashDataType();
            if(!next->read(cacheFile))
            {
                delete next;
                success = false;
                break;
            }

            next->setDataOffset(dataOffset);

            mCache.insert(hash, next);
            mCacheRawDataSize += next->size();
        }

        delete cacheFile;
        return success;
    }

    template <class tHashDataType, uint8_t tHashSize, uint16_t tSampleSize, uint16_t tSetCount>
    bool HashDataSet<tHashDataType, tHashSize, tSampleSize, tSetCount>::SubSet::saveCache()
    {
        // Open cache file
        String filePathName;
        filePathName.writeFormatted("%s%s%04x.cache", mFilePath, PATH_SEPARATOR, mID);
        FileOutputStream *cacheFile = new FileOutputStream(filePathName, true);

        if(!cacheFile->isValid())
        {
            Log::addFormatted(Log::WARNING, NEXTCASH_HASH_DATA_SET_LOG_NAME,
              "Failed to open subset cache file %04x for writing : %s", mID, filePathName.text());
            delete cacheFile;
            return false;
        }

        stream_size dataOffset;
        for(HashContainerList<HashData *>::Iterator item = mCache.begin(); item != mCache.end();
          ++item)
        {
            dataOffset = (*item)->dataOffset();
            cacheFile->write(&dataOffset, sizeof(stream_size));
            item.hash().write(cacheFile);
            (*item)->write(cacheFile);
        }

        delete cacheFile;
        return true;
    }

    // Insert sorted, oldest first
    inline void insertOldest(HashData *pItem, std::vector<HashData *> &pList,
      unsigned int pMaxCount)
    {
        if(pList.size() == 0)
        {
            // Add as first item
            pList.push_back(pItem);
            return;
        }

        if((*--pList.end())->compareAge(pItem) < 0)
        {
            if(pList.size() < pMaxCount)
                pList.push_back(pItem); // Add as last item
            return;
        }

        // Insert sorted
        bool inserted = false;
        for(std::vector<HashData *>::iterator item = pList.begin(); item != pList.end(); ++item)
            if((*item)->compareAge(pItem) > 0)
            {
                inserted = true;
                pList.insert(item, pItem);
                break;
            }

        if(!inserted)
            pList.push_back(pItem);

        if(pList.size() > pMaxCount)
            pList.pop_back();
    }

    template <class tHashDataType, uint8_t tHashSize, uint16_t tSampleSize, uint16_t tSetCount>
    bool HashDataSet<tHashDataType, tHashSize, tSampleSize, tSetCount>::SubSet::load(const char *pName,
      const char *pFilePath, unsigned int pID)
    {
        mLock.lock();

        for(HashContainerList<HashData *>::Iterator item = mCache.begin();
          item != mCache.end(); ++item)
            delete *item;
        mCache.clear();
        mCacheRawDataSize = 0;

        String filePathName;
        bool created = false;

        mFilePath = pFilePath;
        mID = pID;

        // Open index file
        filePathName.writeFormatted("%s%s%04x.index", mFilePath, PATH_SEPARATOR, mID);
        if(!fileExists(filePathName))
        {
            // Create index file
            FileOutputStream indexOutFile(filePathName, true);
            created = true;
        }
        FileInputStream indexFile(filePathName);
        indexFile.setReadOffset(0);

        if(!indexFile.isValid())
        {
            Log::addFormatted(Log::ERROR, pName,
              "Failed to open index file : %s", filePathName.text());
            mLock.unlock();
            return false;
        }

        mFileSize = indexFile.length() / sizeof(stream_size);
        mNewSize = 0;

        // Open data file
        if(created)
        {
            filePathName.writeFormatted("%s%s%04x.data", mFilePath, PATH_SEPARATOR, mID);
            FileOutputStream dataOutFile(filePathName, true); // Create data file
        }

        loadSamples(&indexFile);
        loadCache();

        mLock.unlock();
        return true;
    }

    //TODO This operation is expensive. Try to find a better method
    template <class tHashDataType, uint8_t tHashSize, uint16_t tSampleSize, uint16_t tSetCount>
    void HashDataSet<tHashDataType, tHashSize, tSampleSize, tSetCount>::SubSet::markOld(stream_size pDataSize)
    {
        if(pDataSize == 0)
        {
            for(HashContainerList<HashData *>::Iterator item = mCache.begin();
              item != mCache.end(); ++item)
                (*item)->setOld();
            return;
        }

        uint64_t currentSize = cacheDataSize();
        if(currentSize <= pDataSize)
        {
            // Log::addFormatted(Log::VERBOSE, NEXTCASH_HASH_DATA_SET_LOG_NAME,
              // "Set %d is not big enough to mark old", mID);
            return;
        }

        double markPercent = ((double)(currentSize - pDataSize) / (double)currentSize) * 1.25;
        unsigned int markCount = (unsigned int)((double)mCache.size() * markPercent);
        std::vector<HashData *> oldestList;
        if(markCount == 0)
        {
            Log::addFormatted(Log::VERBOSE, NEXTCASH_HASH_DATA_SET_LOG_NAME,
              "Set %d has no items to mark old", mID);
            return;
        }

        // Build list of oldest items
        for(HashContainerList<HashData *>::Iterator item = mCache.begin(); item != mCache.end();
          ++item)
            insertOldest(*item, oldestList, markCount);

        // Remove all items below age of newest item in old list
        if(oldestList.size() == 0)
        {
            Log::addFormatted(Log::VERBOSE, NEXTCASH_HASH_DATA_SET_LOG_NAME,
              "Set %d has mark old list is empty", mID);
            return;
        }

        unsigned int markedCount = 0;
        HashData *cutoff = oldestList.back();
        stream_size markedSize = 0;
        for(HashContainerList<HashData *>::Iterator item = mCache.begin();
          item != mCache.end(); ++item)
            if((*item)->isOld())
            {
                ++markedCount;
                markedSize += (*item)->size() + staticCacheItemSize;
            }
            else if((*item)->compareAge(cutoff) < 0)
            {
                (*item)->setOld();
                ++markedCount;
                markedSize += (*item)->size() + staticCacheItemSize;
            }

        if(currentSize - markedSize > pDataSize)
        {
            // Mark every other item as old.
            bool markThisOld = false;
            for(HashContainerList<HashData *>::Iterator item = mCache.begin();
              item != mCache.end(); ++item)
            {
                if(markThisOld && !(*item)->isOld())
                {
                    (*item)->setOld();
                    ++markedCount;
                    markedSize += (*item)->size() + staticCacheItemSize;
                    if(currentSize - markedSize < pDataSize)
                        break;
                    markThisOld = false;
                }
                else
                    markThisOld = true;
            }
        }

        if(currentSize - markedSize > pDataSize)
        {
            // Mark every other item as old.
            bool markThisOld = false;
            for(HashContainerList<HashData *>::Iterator item = mCache.begin();
              item != mCache.end(); ++item)
            {
                if(markThisOld && !(*item)->isOld())
                {
                    (*item)->setOld();
                    ++markedCount;
                    markedSize += (*item)->size() + staticCacheItemSize;
                    if(currentSize - markedSize < pDataSize)
                        break;
                    markThisOld = false;
                }
                else
                    markThisOld = true;
            }
        }

        if(currentSize - markedSize > pDataSize)
            Log::addFormatted(Log::WARNING, NEXTCASH_HASH_DATA_SET_LOG_NAME,
              "Set %d failed to mark enough old. Marked %d/%d items (%d/%d)", mID, markedCount,
              mCache.size(), markedSize, currentSize);
    }

    template <class tHashDataType, uint8_t tHashSize, uint16_t tSampleSize, uint16_t tSetCount>
    bool HashDataSet<tHashDataType, tHashSize, tSampleSize, tSetCount>::SubSet::trimCache(uint64_t pMaxCacheDataSize)
    {
        // Mark items as old to keep cache data size under max
        markOld(pMaxCacheDataSize);

        // Remove old and removed items from the cache
        for(HashContainerList<HashData *>::Iterator item = mCache.begin(); item != mCache.end();)
        {
            if((*item)->isOld())
            {
                mCacheRawDataSize -= (*item)->size();
                delete *item;
                item = mCache.erase(item);
            }
            else
                ++item;
        }

        return saveCache();
    }

    template <class tHashDataType, uint8_t tHashSize, uint16_t tSampleSize, uint16_t tSetCount>
    bool HashDataSet<tHashDataType, tHashSize, tSampleSize, tSetCount>::SubSet::save(
      const char *pName, uint64_t pMaxCacheDataSize)
    {
#ifdef PROFILER_ON
        ProfilerReference profiler(getProfiler(PROFILER_SET, PROFILER_HASH_SET_SUB_SAVE_ID,
          PROFILER_HASH_SET_SUB_SAVE_NAME), true);
#endif
        mLock.lock();

        if(mCache.size() == 0)
        {
            // Log::addFormatted(Log::VERBOSE, pName,
              // "Set %d save has no cache", mID);
            mLock.unlock();
            return true;
        }

        String filePathName;

        // Reopen data file as an output stream
        filePathName.writeFormatted("%s%s%04x.data", mFilePath, PATH_SEPARATOR, mID);
        FileOutputStream *dataOutFile = new FileOutputStream(filePathName);
        HashContainerList<HashData *>::Iterator item;
        uint64_t newCount = 0;
        bool indexNeedsUpdated = false;

        // Write all cached data to file, update or append, so they all have file offsets
        for(item = mCache.begin(); item != mCache.end();)
        {
            if((*item)->markedRemove())
            {
                if(!(*item)->isNew())
                {
                    indexNeedsUpdated = true;
                    ++item;
                }
                else
                {
                    mCacheRawDataSize -= (*item)->size();
                    delete *item;
                    item = mCache.erase(item);
                }
            }
            else
            {
                if((*item)->isModified() || !(*item)->wasWritten())
                    (*item)->writeToDataFile(item.hash(), dataOutFile);
                if((*item)->isNew())
                {
                    ++newCount;
                    indexNeedsUpdated = true;
                }
                ++item;
            }
        }

        delete dataOutFile;

        if(!indexNeedsUpdated)
        {
            trimCache(pMaxCacheDataSize);

            // Log::addFormatted(Log::VERBOSE, pName,
              // "Set %d save index not updated", mID);
            mLock.unlock();
            return true;
        }

        // Read entire index file
        filePathName.writeFormatted("%s%s%04x.index", mFilePath, PATH_SEPARATOR, mID);
        FileInputStream *indexFile = new FileInputStream(filePathName);
        uint64_t previousSize = indexFile->length() / sizeof(stream_size);
        DistributedVector<stream_size> indices(tSetCount);
        DistributedVector<Hash> hashes(tSetCount);
        unsigned int indicesPerSet = (previousSize / tSetCount) + 1;
        unsigned int readIndices = 0;
        std::vector<stream_size> *indiceSet;
        std::vector<Hash> *hashSet;
        unsigned int setOffset = 0;
        uint64_t reserveSize = previousSize + mCache.size();

        if(reserveSize < tSetCount * 32)
            reserveSize = tSetCount * 32;

        indices.reserve(reserveSize);
        hashes.reserve(reserveSize);
        indexFile->setReadOffset(0);
        while(indexFile->remaining())
        {
            if(previousSize - readIndices < indicesPerSet)
                indicesPerSet = previousSize - readIndices;

            // Read set of indices
            indiceSet = indices.dataSet(setOffset);
            indiceSet->resize(indicesPerSet);
            indexFile->read(indiceSet->data(), indicesPerSet * sizeof(stream_size));

            // Allocate empty hashes
            hashSet = hashes.dataSet(setOffset);
            hashSet->resize(indicesPerSet);

            readIndices += indicesPerSet;
            ++setOffset;
        }

        delete indexFile;
        indices.refresh();
        hashes.refresh();

        // Update indices
        DistributedVector<Hash>::Iterator hash;
        DistributedVector<stream_size>::Iterator index;
        int compare;
        bool found;
        int32_t lastReport = getTime();
        unsigned int cacheOffset = 0;
        unsigned int begin, end, current;
        unsigned int readHeadersCount = 0;//, previousIndices = indices.size();
        bool success = true;

        filePathName.writeFormatted("%s%s%04x.data", mFilePath, PATH_SEPARATOR, mID);
        FileInputStream dataFile(filePathName);

        for(item = mCache.begin(); item != mCache.end() && success; ++cacheOffset)
        {
            if(getTime() - lastReport > 10)
            {
                Log::addFormatted(Log::INFO, pName,
                  "Set %d save index update is %2d%% Complete", mID,
                  (int)(((float)cacheOffset / (float)mCache.size()) * 100.0f));

                lastReport = getTime();
            }

            if((*item)->markedRemove())
            {
                // Check that it was previously added to the index and data file.
                // Otherwise it isn't in current indices and doesn't need removed.
                if(!(*item)->isNew())
                {
                    // Remove from indices.
                    // They aren't sorted by file offset so in this scenario a linear search is
                    //   required since not all hashes are read and reading them for a binary
                    //   search would presumably be more expensive since it requires reading hashes.
                    found = false;
                    hash = hashes.begin();
                    for(index = indices.begin();index != indices.end(); ++index, ++hash)
                        if(*index == (*item)->dataOffset())
                        {
                            indices.erase(index);
                            hashes.erase(hash);
                            found = true;
                            break;
                        }

                    if(!found)
                    {
                        Log::addFormatted(Log::ERROR, pName,
                          "Failed to find index to remove for file offset %d : %s",
                          (*item)->dataOffset(), item.hash().hex().text());
                        success = false;
                        break;
                    }
                }

                mCacheRawDataSize -= (*item)->size();
                delete *item;
                item = mCache.erase(item);
            }
            else if((*item)->isNew())
            {
                // For new items perform insert sort into existing indices.
                // This costs more processor time to do the insert for every new item.
                // This saves file reads by not requiring a read of every existing index like a
                //   merge sort would.
                if(indices.size () == 0)
                {
                    // Add as only item
                    indices.push_back((*item)->dataOffset());
                    hashes.push_back(item.hash());
                    (*item)->clearNew();
                    ++item;
                    continue;
                }

                // Check first entry
                hash = hashes.begin();
                if(hash->isEmpty())
                {
                    // Fetch data
                    if(!pullHash(&dataFile, indices.front(), *hash))
                    {
                        success = false;
                        break;
                    }
                    ++readHeadersCount;
                }

                compare = item.hash().compare(*hash);
                if(compare <= 0)
                {

                    // Insert as first
                    indices.insert(indices.begin(), (*item)->dataOffset());
                    hashes.insert(hashes.begin(), item.hash());
                    (*item)->clearNew();

                    ++item;
                    continue;
                }

                // Check last entry
                hash = hashes.end() - 1;
                if(hash->isEmpty())
                {
                    // Fetch data
                    if(!pullHash(&dataFile, indices.back(), *hash))
                    {
                        success = false;
                        break;
                    }
                    ++readHeadersCount;
                }

                compare = item.hash().compare(*hash);
                if(compare >= 0)
                {
                    // Add to end
                    indices.push_back((*item)->dataOffset());
                    hashes.push_back(item.hash());
                    (*item)->clearNew();
                    ++item;
                    continue;
                }

                // Binary insert sort
                begin = 0;
                end = indices.size() - 1;
                while(true)
                {
                    // Divide data set in half
                    current = (begin + end) / 2;

                    // Pull "current" entry (if it isn't already)
                    hash = hashes.begin() + current;
                    index = indices.begin() + current;
                    if(hash->isEmpty())
                    {
                        // Fetch data
                        if(!pullHash(&dataFile, *index, *hash))
                        {
                            success = false;
                            break;
                        }
                        ++readHeadersCount;
                    }

                    compare = item.hash().compare(*hash);
                    if(current == begin || compare == 0)
                    {
                        if(current != begin && compare < 0)
                        {
                            // Insert before current
                            indices.insert(index, (*item)->dataOffset());
                            hashes.insert(hash, item.hash());
                            (*item)->clearNew();
                        }
                        else //if(compare >= 0)
                        {
                            // Insert after current
                            ++index;
                            ++hash;
                            indices.insert(index, (*item)->dataOffset());
                            hashes.insert(hash, item.hash());
                            (*item)->clearNew();
                        }
                        break;
                    }

                    if(compare > 0)
                        begin = current;
                    else //if(compare < 0)
                        end = current;
                }
                ++item;
            }
            else
                ++item;
        }

        if(success)
        {
            // Open index file as an output stream
            filePathName.writeFormatted("%s%s%04x.index", mFilePath, PATH_SEPARATOR, mID);
            FileOutputStream *indexOutFile = new FileOutputStream(filePathName, true);

            // Write the new index
            for(setOffset = 0; setOffset < tSetCount; ++setOffset)
            {
                // Write set of indices
                indiceSet = indices.dataSet(setOffset);
                indexOutFile->write(indiceSet->data(), indiceSet->size() * sizeof(stream_size));
            }

            // Update size
            mFileSize = indexOutFile->length() / sizeof(stream_size);
            mNewSize = 0;

            delete indexOutFile;

            // Open index file
            filePathName.writeFormatted("%s%s%04x.index", mFilePath, PATH_SEPARATOR, mID);
            FileInputStream indexFile(filePathName);

            // Reload samples
            loadSamples(&indexFile);

            trimCache(pMaxCacheDataSize);
        }

        mLock.unlock();
        return true;
    }

    template <class tHashDataType, uint8_t tHashSize, uint16_t tSampleSize, uint16_t tSetCount>
    bool HashDataSet<tHashDataType, tHashSize, tSampleSize, tSetCount>::SubSet::defragment()
    {
        // Open current index file.
        // Create new temp index file.
        // Open current data file.
        // Create new temp data file.
        // Parse through index file.
        //   Pull each associated item from the current data file and append it to the temp data
        //     file.
        //   Append new data offset to the temp index file.
        // Remove current files and replace with temp files.
        return false;
    }

    bool testHashDataSet();
}

#endif
