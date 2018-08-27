/**************************************************************************
 * Copyright 2017 NextCash, LLC                                           *
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
    // NOTE: The objects size can't increase after being initially added to the hash data set or it
    //   will overwrite the next item in the file
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
        virtual uint64_t size() = 0;

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
            stream_size cacheDataSize() { return mCacheRawDataSize + (mCache.size() * (tHashSize + 12)); } // 12 = 8 byte memory pointer + 4 byte size count

            // Inserts a new item corresponding to the lookup.
            bool insert(const Hash &pLookupValue, HashData *pValue, bool pRejectMatching);

            bool removeIfMatching(const Hash &pLookupValue, HashData *pValue);

            // Returns an iterator referencing the first matching item.
            SubSetIterator get(const Hash &pLookupValue, bool pForcePull = false);

            SubSetIterator end() { return mCache.end(); }

            // Pull all items with matching hashes from the file and put them in the cache.
            //   Returns true if any items were added to the cache.
            // If pPullMatchingFunction then only items that return true will be pulled.
            bool pull(const Hash &pLookupValue, HashData *pMatching = NULL);

            bool load(const char *pFilePath, unsigned int pID);
            bool save();
            bool cleanup(uint64_t pMaxCacheDataSize);

        private:

            Hash pullHash(InputStream *pDataFile, stream_size pFileOffset)
            {
#ifdef PROFILER_ON
                NextCash::Profiler profiler("Hash SubSet Save Pull Hash");
#endif
                if(!pDataFile->setReadOffset(pFileOffset))
                {
                    Log::addFormatted(Log::ERROR, NEXTCASH_HASH_DATA_SET_LOG_NAME,
                      "Failed to pull hash at index offset %d/%d", pFileOffset, pDataFile->length());
                    return Hash();
                }

                Hash result(tHashSize);
                if(!result.read(pDataFile))
                {
                    Log::addFormatted(Log::ERROR, NEXTCASH_HASH_DATA_SET_LOG_NAME,
                      "Failed to pull hash at index offset %d/%d", pFileOffset, pDataFile->length());
                    return Hash();
                }
                return result;
            }

            void loadSamples(InputStream *pIndexFile);

            // Find offsets into indices that contain the specified hash, based on samples
            bool findSample(const Hash &pHash, InputStream *pIndexFile, InputStream *pDataFile,
              stream_size &pBegin, stream_size &pEnd);

            bool loadCache();
            bool saveCache();

            // Prune old items from the cache until it is under the specified data size
            void pruneCache(stream_size pDataSize);

            ReadersLock mLock;
            String mFilePath;
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

        HashDataSet() : mLock("HashDataSet") { mTargetCacheDataSize = 0; mIsValid = false; }
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

        Iterator begin();
        Iterator end();

        virtual bool load(const char *pName, const char *pFilePath);
        virtual bool save();
        virtual bool saveMultiThreaded(unsigned int pThreadCount = 4);

        class SaveThreadData
        {
        public:

            SaveThreadData(SubSet *pFirstSubSet, stream_size pMaxSetCacheDataSize) :
              mutex("SaveThreadData")
            {
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
                mutex.lock();
                setComplete[pOffset] = true;
                setSuccess[pOffset] = pSuccess;
                if(!pSuccess)
                    success = false;
                mutex.unlock();
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
        NextCash::Profiler profiler("Hash Set Insert");
#endif
        mLock.readLock();
        bool result = mSubSets[subSetOffset(pLookupValue)].insert(pLookupValue, pValue, pRejectMatching);
        mLock.readUnlock();
        return result;
    }

    template <class tHashDataType, uint8_t tHashSize, uint16_t tSampleSize, uint16_t tSetCount>
    bool HashDataSet<tHashDataType, tHashSize, tSampleSize, tSetCount>::removeIfMatching(const Hash &pLookupValue,
      HashData *pValue)
    {
#ifdef PROFILER_ON
        NextCash::Profiler profiler("Hash Set Remove If Matching");
#endif
        mLock.readLock();
        bool result = mSubSets[subSetOffset(pLookupValue)].removeIfMatching(pLookupValue, pValue);
        mLock.readUnlock();
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
    bool HashDataSet<tHashDataType, tHashSize, tSampleSize, tSetCount>::load(const char *pName,
      const char *pFilePath)
    {
        mLock.writeLock("Load");
        mIsValid = true;
        mName = pName;
        mFilePath = pFilePath;
        if(!createDirectory(mFilePath))
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_DATA_SET_LOG_NAME,
              "%s Failed to create directory : %s", mName.text(), mFilePath.text());
            mIsValid = false;
            mLock.writeUnlock();
            return false;
        }
        SubSet *subSet = mSubSets;
        uint32_t lastReport = getTime();
        for(unsigned int i=0;i<tSetCount;++i)
        {
            if(getTime() - lastReport > 10)
            {
                Log::addFormatted(Log::INFO, NEXTCASH_HASH_DATA_SET_LOG_NAME,
                  "%s load is %2d%% Complete", mName.text(), (int)(((float)i / (float)tSetCount) * 100.0f));
                lastReport = getTime();
            }
            if(!subSet->load(mFilePath, i))
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
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_DATA_SET_LOG_NAME,
              "%s : can't save invalid data set", mName.text());
            mLock.writeUnlock();
            return false;
        }

        SubSet *subSet = mSubSets;
        uint32_t lastReport = getTime();
        uint64_t maxSetCacheDataSize = 0;
        bool success = true;
        if(mTargetCacheDataSize > 0)
            maxSetCacheDataSize = mTargetCacheDataSize / tSetCount;
        for(unsigned int i=0;i<tSetCount;++i)
        {
            if(getTime() - lastReport > 10)
            {
                Log::addFormatted(Log::INFO, NEXTCASH_HASH_DATA_SET_LOG_NAME,
                  "%s save is %2d%% Complete", mName.text(),
                  (int)(((float)i / (float)tSetCount) * 100.0f));
                lastReport = getTime();
            }

            if(!subSet->save())
            {
                Log::addFormatted(Log::ERROR, NEXTCASH_HASH_DATA_SET_LOG_NAME,
                  "Failed %s set %d save", mName.text(), subSet->id());
                success = false;
            }

            if(!subSet->cleanup(maxSetCacheDataSize))
            {
                Log::addFormatted(Log::ERROR, NEXTCASH_HASH_DATA_SET_LOG_NAME,
                  "Failed %s set %d save cleanup", mName.text(), subSet->id());
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
            Log::add(NextCash::Log::WARNING, NEXTCASH_HASH_DATA_SET_LOG_NAME,
              "Thread parameter is null. Stopping");
            return;
        }

        SubSet *subSet;
        while(true)
        {
            subSet = data->getNext();
            if(subSet == NULL)
                break;

            if(subSet->save() && subSet->cleanup(data->maxSetCacheDataSize))
                data->markComplete(subSet->id(), true);
            else
                data->markComplete(subSet->id(), false);
        }
    }

    template <class tHashDataType, uint8_t tHashSize, uint16_t tSampleSize, uint16_t tSetCount>
    bool HashDataSet<tHashDataType, tHashSize, tSampleSize, tSetCount>::saveMultiThreaded(unsigned int pThreadCount)
    {
        mLock.writeLock("Save");

        if(!mIsValid)
        {
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_DATA_SET_LOG_NAME,
              "%s : can't save invalid data set", mName.text());
            mLock.writeUnlock();
            return false;
        }

        uint64_t maxSetCacheDataSize = 0;
        if(mTargetCacheDataSize > 0)
            maxSetCacheDataSize = mTargetCacheDataSize / tSetCount;
        SaveThreadData threadData(mSubSets, maxSetCacheDataSize);
        Thread *threads[pThreadCount];
        int32_t lastReport = getTime();
        unsigned int i;
        String threadName;

        // Start threads
        for(i = 0; i < pThreadCount; ++i)
        {
            threadName.writeFormatted("%s %d", mName.text(), i);
            threads[i] = new Thread(threadName, saveThreadRun, &threadData);
        }

        // Monitor threads
        while(threadData.offset < tSetCount)
        {
            if(getTime() - lastReport > 10)
            {
                Log::addFormatted(Log::INFO, NEXTCASH_HASH_DATA_SET_LOG_NAME,
                  "%s save is %2d%% Complete", mName.text(),
                  (int)(((float)threadData.offset / (float)tSetCount) * 100.0f));
                lastReport = getTime();
            }

            Thread::sleep(500);
        }

        // Delete threads
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
#ifdef PROFILER_ON
        NextCash::Profiler profiler("Hash SubSet Insert");
#endif
        bool result = false;
        mLock.writeLock();
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
        mLock.writeUnlock();
        return result;
    }

    template <class tHashDataType, uint8_t tHashSize, uint16_t tSampleSize, uint16_t tSetCount>
    bool HashDataSet<tHashDataType, tHashSize, tSampleSize, tSetCount>::SubSet::removeIfMatching(const Hash &pLookupValue,
      HashData *pValue)
    {
#ifdef PROFILER_ON
        NextCash::Profiler profiler("Hash SubSet Remove If Matching");
#endif
        mLock.writeLock();

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

        mLock.writeUnlock();
        return result;
    }

    template <class tHashDataType, uint8_t tHashSize, uint16_t tSampleSize, uint16_t tSetCount>
    typename HashDataSet<tHashDataType, tHashSize, tSampleSize, tSetCount>::SubSetIterator
      HashDataSet<tHashDataType, tHashSize, tSampleSize, tSetCount>::SubSet::get(const Hash &pLookupValue,
      bool pForcePull)
    {
        mLock.readLock();
        if(pForcePull)
            pull(pLookupValue);
        SubSetIterator result = mCache.get(pLookupValue);
        if(!pForcePull && result == mCache.end() && pull(pLookupValue))
            result = mCache.get(pLookupValue);
        mLock.readUnlock();
        return result;
    }

    template <class tHashDataType, uint8_t tHashSize, uint16_t tSampleSize, uint16_t tSetCount>
    bool HashDataSet<tHashDataType, tHashSize, tSampleSize, tSetCount>::SubSet::pull(const Hash &pLookupValue,
      HashData *pMatching)
    {
        if(mFileSize == 0)
            return false;

        int compare;
        stream_size dataOffset;
        Hash hash(tHashSize);
        stream_size first = 0, last = (mFileSize - 1) * sizeof(stream_size), begin, end, current;
        String filePathName;
        filePathName.writeFormatted("%s%s%04x.index", mFilePath.text(), PATH_SEPARATOR, mID);
        FileInputStream indexFile(filePathName);
        filePathName.writeFormatted("%s%s%04x.data", mFilePath.text(), PATH_SEPARATOR, mID);
        FileInputStream dataFile(filePathName);

        if(!indexFile.isValid())
        {
            Log::add(Log::ERROR, NEXTCASH_HASH_DATA_SET_LOG_NAME, "Failed to open index file in pull");
            return false;
        }

        if(!dataFile.isValid())
        {
            Log::add(Log::ERROR, NEXTCASH_HASH_DATA_SET_LOG_NAME, "Failed to open index file in pull");
            return false;
        }

        // Log::addFormatted(Log::VERBOSE, NEXTCASH_HASH_DATA_SET_LOG_NAME,
          // "Pull : %s", pLookupValue.hex().text());
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
                // Log::addFormatted(Log::VERBOSE, NEXTCASH_HASH_DATA_SET_LOG_NAME,
                  // "Binary : %s", hash.hex().text());

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
        mCache.clear();
        mCacheRawDataSize = 0;

        // Open cache file
        String filePathName;
        filePathName.writeFormatted("%s%s%04x.cache", mFilePath.text(), PATH_SEPARATOR, mID);
        FileInputStream *cacheFile = new FileInputStream(filePathName);
        HashData *next;
        Hash hash(tHashSize);

        if(!cacheFile->isValid())
        {
            // Log::addFormatted(Log::DEBUG, NEXTCASH_HASH_DATA_SET_LOG_NAME,
              // "Failed to open subset cache file %04x for reading : %s", mID, filePathName.text());
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
        filePathName.writeFormatted("%s%s%04x.cache", mFilePath.text(), PATH_SEPARATOR, mID);
        FileOutputStream *cacheFile = new FileOutputStream(filePathName, true);

        if(!cacheFile->isValid())
        {
            Log::addFormatted(Log::WARNING, NEXTCASH_HASH_DATA_SET_LOG_NAME,
              "Failed to open subset cache file %04x for writing : %s", mID, filePathName.text());
            delete cacheFile;
            return false;
        }

        stream_size dataOffset;
        for(HashContainerList<HashData *>::Iterator item=mCache.begin();item!=mCache.end();++item)
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
    inline void insertOldest(HashData *pItem, std::vector<HashData *> &pList, unsigned int pMaxCount)
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
        for(std::vector<HashData *>::iterator item=pList.begin();item!=pList.end();++item)
            if((*item)->compareAge(pItem) > 0)
            {
                inserted = true;
                pList.insert(item, pItem);
                break;
            }

        if(!inserted)
            pList.push_back(pItem);

        if(pList.size() > pMaxCount)
            pList.erase(--pList.end());
    }

    //TODO This operation is expensive. Try to find a better method
    template <class tHashDataType, uint8_t tHashSize, uint16_t tSampleSize, uint16_t tSetCount>
    void HashDataSet<tHashDataType, tHashSize, tSampleSize, tSetCount>::SubSet::pruneCache(stream_size pDataSize)
    {
#ifdef PROFILER_ON
        NextCash::Profiler profiler("Hash SubSet Prune");
#endif
        if(pDataSize == 0)
        {
            for(HashContainerList<HashData *>::Iterator item=mCache.begin();item!=mCache.end();++item)
                (*item)->setOld();
            return;
        }

        uint64_t currentSize = cacheDataSize();
        if(currentSize <= pDataSize)
            return;

        double prunePercent = (double)(currentSize - pDataSize) / (double)currentSize;
        unsigned int pruneCount = (unsigned int)((double)mCache.size() * prunePercent);
        unsigned int prunedCount = 0;
        std::vector<HashData *> oldestList;
        if(pruneCount <= 0)
            return;

        // Build list of oldest items
        for(HashContainerList<HashData *>::Iterator item=mCache.begin();item!=mCache.end();++item)
            insertOldest(*item, oldestList, pruneCount);

        // Remove all items below age of newest item in old list
        if(oldestList.size() > 0)
        {
            HashData *cutoff = oldestList.back();
            for(HashContainerList<HashData *>::Iterator item=mCache.begin();item!=mCache.end();++item)
                if((*item)->compareAge(cutoff) < 0)
                {
                    ++prunedCount;
                    (*item)->setOld();
                }
        }

        // Log::addFormatted(Log::DEBUG, NEXTCASH_HASH_DATA_SET_LOG_NAME,
          // "Marked %d/%d items as old during pruning", prunedCount, pruneCount);
    }

    template <class tHashDataType, uint8_t tHashSize, uint16_t tSampleSize, uint16_t tSetCount>
    bool HashDataSet<tHashDataType, tHashSize, tSampleSize, tSetCount>::SubSet::load(const char *pFilePath, unsigned int pID)
    {
        mLock.writeLock("Load");

        mCache.clear();
        mCacheRawDataSize = 0;

        String filePathName;
        bool created = false;

        mFilePath = pFilePath;
        mID = pID;

        // Open index file
        filePathName.writeFormatted("%s%s%04x.index", mFilePath.text(), PATH_SEPARATOR, mID);
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
            Log::addFormatted(Log::ERROR, NEXTCASH_HASH_DATA_SET_LOG_NAME,
              "Failed to open index file : %s", filePathName.text());
            mLock.writeUnlock();
            return false;
        }

        mFileSize = indexFile.length() / sizeof(stream_size);
        mNewSize = 0;

        // Open data file
        filePathName.writeFormatted("%s%s%04x.data", mFilePath.text(), PATH_SEPARATOR, mID);
        if(created)
            FileOutputStream dataOutFile(filePathName, true); // Create data file

        loadSamples(&indexFile);
        loadCache();

        mLock.writeUnlock();
        return true;
    }

    template <class tHashDataType, uint8_t tHashSize, uint16_t tSampleSize, uint16_t tSetCount>
    bool HashDataSet<tHashDataType, tHashSize, tSampleSize, tSetCount>::SubSet::cleanup(uint64_t pMaxCacheDataSize)
    {
#ifdef PROFILER_ON
        NextCash::Profiler profiler("Hash SubSet Clean");
#endif
        // Mark items as old to keep cache data size under max
        pruneCache(pMaxCacheDataSize);

        // Remove old and removed items from the cache
        // unsigned int prunedCount = 0;
        // unsigned int previousSize = mCache.size();
        for(HashContainerList<HashData *>::Iterator item=mCache.begin();item!=mCache.end();)
        {
            if((*item)->isOld() || (*item)->markedRemove())
            {
                mCacheRawDataSize -= (*item)->size();
                delete *item;
                item = mCache.erase(item);
                // ++prunedCount;
            }
            else
                ++item;
        }

        // Log::addFormatted(Log::DEBUG, NEXTCASH_HASH_DATA_SET_LOG_NAME,
          // "Pruned %d/%d items from cache to new size %d", prunedCount, previousSize, mCache.size());

        saveCache();

        // Open index file
        String filePathName;
        filePathName.writeFormatted("%s%s%04x.index", mFilePath.text(), PATH_SEPARATOR, mID);
        FileInputStream indexFile(filePathName);

        // Reload samples
        loadSamples(&indexFile);

        return true;
    }

    template <class tHashDataType, uint8_t tHashSize, uint16_t tSampleSize, uint16_t tSetCount>
    bool HashDataSet<tHashDataType, tHashSize, tSampleSize, tSetCount>::SubSet::save()
    {
#ifdef PROFILER_ON
        NextCash::Profiler profiler("Hash SubSet Save");
#endif
        mLock.writeLock("Save");

        if(mCache.size() == 0)
        {
            mLock.writeUnlock();
            return true;
        }

        String filePathName;

#ifdef PROFILER_ON
        NextCash::Profiler profilerWriteData("Hash SubSet Save Write Data");
#endif
        // Reopen data file as an output stream
        filePathName.writeFormatted("%s%s%04x.data", mFilePath.text(), PATH_SEPARATOR, mID);
        FileOutputStream *dataOutFile = new FileOutputStream(filePathName);
        HashContainerList<HashData *>::Iterator item;
        uint64_t newCount = 0;
        bool indexNeedsUpdated = false;

        // Write all cached data to file, update or append, so they all have file offsets
        for(item=mCache.begin();item!=mCache.end();++item)
        {
            if((*item)->markedRemove())
            {
                if((*item)->wasWritten())
                    indexNeedsUpdated = true;
            }
            else
            {
                (*item)->writeToDataFile(item.hash(), dataOutFile);
                if((*item)->isNew())
                {
                    ++newCount;
                    indexNeedsUpdated = true;
                }
            }
        }

        delete dataOutFile;
#ifdef PROFILER_ON
        profilerWriteData.stop();
#endif

        if(!indexNeedsUpdated)
        {
            mLock.writeUnlock();
            return true;
        }

#ifdef PROFILER_ON
        NextCash::Profiler profilerReadIndex("Hash SubSet Save Read Index");
#endif
        // Read entire index file
        filePathName.writeFormatted("%s%s%04x.index", mFilePath.text(), PATH_SEPARATOR, mID);
        FileInputStream *indexFile = new FileInputStream(filePathName);
        uint64_t previousSize = indexFile->length() / sizeof(stream_size);
        DistributedVector<stream_size> indices(tSetCount);
        DistributedVector<Hash> hashes(tSetCount);
        unsigned int indicesPerSet = (previousSize / tSetCount) + 1;
        unsigned int readIndices = 0;
        std::vector<stream_size> *indiceSet;
        std::vector<Hash> *hashSet;
        unsigned int setOffset = 0;
        uint64_t reserveSize = previousSize;

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
#ifdef PROFILER_ON
        profilerReadIndex.stop();
#endif

#ifdef PROFILER_ON
        NextCash::Profiler profilerUpdateIndex("Hash SubSet Save Update Index");
        NextCash::Profiler profilerIndexInsert("Hash SubSet Save Index Insert", false);
        NextCash::Profiler profilerIndexInsertPush("Hash SubSet Save Index Insert Push", false);
#endif
        // Update indices
        DistributedVector<Hash>::Iterator hash;
        DistributedVector<stream_size>::Iterator index;
        Hash currentHash;
        int compare;
        bool found;
        unsigned int begin, end, current;
        unsigned int readHeadersCount = 0;//, previousIndices = indices.size();
        bool success = true;

        filePathName.writeFormatted("%s%s%04x.data", mFilePath.text(), PATH_SEPARATOR, mID);
        FileInputStream dataFile(filePathName);

        for(item=mCache.begin();item!=mCache.end() && success;++item)
        {
            if((*item)->markedRemove())
            {
                (*item)->clearNew();

                // Check that it was previously added to the index and data file.
                // Otherwise it isn't in current indices and doesn't need removed.
                if((*item)->wasWritten())
                {
                    // Remove from indices.
                    // They aren't sorted by file offset so in this scenario a linear search is
                    //   required since not all hashes are read and reading them for a binary
                    //   search would presumably be more expensive since it requires reading hashes.
                    found = false;
                    hash = hashes.begin();
                    for(index=indices.begin();index!=indices.end();++index,++hash)
                        if(*index == (*item)->dataOffset())
                        {
                            indices.erase(index);
                            hashes.erase(hash);
                            found = true;
                            break;
                        }

                    if(!found)
                    {
                        Log::addFormatted(Log::ERROR, NEXTCASH_HASH_DATA_SET_LOG_NAME,
                          "Failed to find index to remove for file offset %d : %s", (*item)->dataOffset(),
                          item.hash().hex().text());
                        success = false;
                        break;
                    }
                }
            }
            else if((*item)->isNew())
            {
#ifdef PROFILER_ON
                profilerIndexInsert.start();
#endif
                // For new items perform insert sort into existing indices.
                // This costs more processor time to do the insert for every new item.
                // This saves file reads by not requiring a read of every existing indice like a merge sort
                //   would.
                if(indices.size () == 0)
                {
#ifdef PROFILER_ON
                    profilerIndexInsertPush.start();
#endif
                    // Add as only item
                    indices.push_back((*item)->dataOffset());
                    hashes.push_back(item.hash());
#ifdef PROFILER_ON
                    profilerIndexInsertPush.stop();
#endif
                    (*item)->clearNew();
#ifdef PROFILER_ON
                    profilerIndexInsert.stop();
#endif
                    continue;
                }

                // Check first entry
                currentHash = hashes.front();
                if(currentHash.isEmpty())
                {
                    // Fetch data
                    currentHash = pullHash(&dataFile, indices.front());
                    ++readHeadersCount;
                    if(currentHash.isEmpty())
                    {
                        success = false;
#ifdef PROFILER_ON
                        profilerIndexInsert.stop();
#endif
                        break;
                    }
                    hashes.front() = currentHash;
                }

                compare = item.hash().compare(currentHash);
                if(compare <= 0)
                {
#ifdef PROFILER_ON
                    profilerIndexInsertPush.start();
#endif
                    // Insert as first
                    indices.insert(indices.begin(), (*item)->dataOffset());
                    hashes.insert(hashes.begin(), item.hash());
                    (*item)->clearNew();
#ifdef PROFILER_ON
                    profilerIndexInsertPush.stop();
                    profilerIndexInsert.stop();
#endif
                    continue;
                }

                // Check last entry
                currentHash = hashes.back();
                if(currentHash.isEmpty())
                {
                    // Fetch data
                    currentHash = pullHash(&dataFile, indices.back());
                    ++readHeadersCount;
                    if(currentHash.isEmpty())
                    {
                        success = false;
#ifdef PROFILER_ON
                        profilerIndexInsert.stop();
#endif
                        break;
                    }
                    hashes.back() = currentHash;
                }

                compare = item.hash().compare(currentHash);
                if(compare >= 0)
                {
#ifdef PROFILER_ON
                    profilerIndexInsertPush.start();
#endif
                    // Add to end
                    indices.push_back((*item)->dataOffset());
                    hashes.push_back(item.hash());
                    (*item)->clearNew();
#ifdef PROFILER_ON
                    profilerIndexInsertPush.stop();
                    profilerIndexInsert.stop();
#endif
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
                    currentHash = hashes[current];
                    if(currentHash.isEmpty())
                    {
                        // Fetch data
                        currentHash = pullHash(&dataFile, indices[current]);
                        ++readHeadersCount;
                        if(currentHash.isEmpty())
                        {
                            success = false;
                            break;
                        }
                        hashes[current] = currentHash;
                    }

                    compare = item.hash().compare(currentHash);
                    if(current == begin || compare == 0)
                    {
#ifdef PROFILER_ON
                        profilerIndexInsertPush.start();
#endif
                        if(compare < 0)
                        {
                            // Insert before current
                            index = indices.begin();
                            index += current;
                            indices.insert(index, (*item)->dataOffset());
                            (*item)->clearNew();

                            // Log::addFormatted(Log::VERBOSE, NEXTCASH_HASH_DATA_SET_LOG_NAME,
                              // "Inserted after : %s", hashes[current-1]->id.hex().text());

                            // Log::addFormatted(Log::VERBOSE, NEXTCASH_HASH_DATA_SET_LOG_NAME,
                              // "Inserted index : %s", (*item)->id.hex().text());

                            // Log::addFormatted(Log::VERBOSE, NEXTCASH_HASH_DATA_SET_LOG_NAME,
                              // "Inserted befor : %s", hashes[current]->id.hex().text());

                            hash = hashes.begin();
                            hash += current;
                            hashes.insert(hash, item.hash());
#ifdef PROFILER_ON
                            profilerIndexInsertPush.stop();
#endif
                            break;
                        }
                        else //if(compare >= 0)
                        {
                            // Insert after current
                            index = indices.begin();
                            index += current + 1;
                            indices.insert(index, (*item)->dataOffset());
                            (*item)->clearNew();

                            // Log::addFormatted(Log::VERBOSE, NEXTCASH_HASH_DATA_SET_LOG_NAME,
                              // "Inserted after : %s", hashes[current]->id.hex().text());

                            // Log::addFormatted(Log::VERBOSE, NEXTCASH_HASH_DATA_SET_LOG_NAME,
                              // "Inserted index : %s", (*item)->id.hex().text());

                            // Log::addFormatted(Log::VERBOSE, NEXTCASH_HASH_DATA_SET_LOG_NAME,
                              // "Inserted befor : %s", hashes[current+1]->id.hex().text());

                            hash = hashes.begin();
                            hash += current + 1;
                            hashes.insert(hash, item.hash());
#ifdef PROFILER_ON
                            profilerIndexInsertPush.stop();
#endif
                            break;
                        }
                    }

                    if(compare > 0)
                        begin = current;
                    else //if(compare < 0)
                        end = current;
                }
#ifdef PROFILER_ON
                profilerIndexInsert.stop();
#endif
            }
        }
#ifdef PROFILER_ON
        profilerUpdateIndex.stop();
#endif

        if(success)
        {
#ifdef PROFILER_ON
            NextCash::Profiler profilerWriteIndex("Hash SubSet Save Write Index");
#endif
            // Open index file as an output stream
            filePathName.writeFormatted("%s%s%04x.index", mFilePath.text(), PATH_SEPARATOR, mID);
            FileOutputStream *indexOutFile = new FileOutputStream(filePathName, true);

            // Write the new index
            for(setOffset=0;setOffset<tSetCount;++setOffset)
            {
                // Write set of indices
                indiceSet = indices.dataSet(setOffset);
                indexOutFile->write(indiceSet->data(), indiceSet->size() * sizeof(stream_size));
            }

            // Update size
            mFileSize = indexOutFile->length() / sizeof(stream_size);
            mNewSize = 0;

            // Reopen index file as an input stream
            delete indexOutFile;
#ifdef PROFILER_ON
            profilerWriteIndex.stop();
#endif
        }

        mLock.writeUnlock();
        return true;
    }

    bool testHashDataSet();
}

#endif
