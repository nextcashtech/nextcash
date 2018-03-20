/**************************************************************************
 * Copyright 2017 NextCash, LLC                                            *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@nextcash.com>                                    *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#ifndef NEXTCASH_LOG_HPP
#define NEXTCASH_LOG_HPP

#include "nextcash/io/stream.hpp"
#include "nextcash/base/mutex.hpp"


namespace NextCash
{
    class Log
    {
    public:
        enum Level { DEBUG, VERBOSE, INFO, WARNING, ERROR, NOTIFICATION, CRITICAL };

        static void setLevel(Level pLevel); // Defaults to INFO
        static void setOutput(OutputStream *pStream, bool pDeleteOnExit = false); // Defaults to std::cerr

        // Set file path name for log file. Auto rolls every day
        static void setOutputFile(const char *pFilePathName);

        // Set log file roll frequency. Only works after setOutputFile
        static void setRollFrequency(uint64_t pSeconds);

        static void add(Level pLevel, const char *pName, const char *pEntry);
        static void debug(const char *pName, const char *pEntry) { add(DEBUG, pName, pEntry); }
        static void verbose(const char *pName, const char *pEntry) { add(VERBOSE, pName, pEntry); }
        static void info(const char *pName, const char *pEntry) { add(INFO, pName, pEntry); }
        static void warning(const char *pName, const char *pEntry) { add(WARNING, pName, pEntry); }
        static void error(const char *pName, const char *pEntry) { add(ERROR, pName, pEntry); }

        static void addFormatted(Level pLevel, const char *pName, const char *pFormatting, ...);
        static void debugFormatted(const char *pName, const char *pFormatting, ...);
        static void verboseFormatted(const char *pName, const char *pFormatting, ...);
        static void infoFormatted(const char *pName, const char *pFormatting, ...);
        static void warningFormatted(const char *pName, const char *pFormatting, ...);
        static void errorFormatted(const char *pName, const char *pFormatting, ...);

        // Read binary from pStream and write it as hex text into the log
        static void addHex(Level pLevel, const char *pName, const char *pDescription, InputStream *pStream, stream_size pSize);

        void lock() { mMutex.lock(); }
        void unlock() { mMutex.unlock(); }

    private:

        static Log &log();
        static Log *mInstance;
        static void destroy();

        Log(OutputStream *pStream, const char *pDateTimeFormat);
        ~Log();
        void internalSetOutput(OutputStream *pStream, bool pDeleteOnExit);
        void internalSetOutputFile(const char *pFilePathName);
        void internalSetRollFrequency(uint64_t pSeconds) { mRollFrequency = pSeconds; }
        bool startEntry(Level pLevel, const char *pName);

        const char *mDateTimeFormat;
        Level mLevel, mPendingEntryLevel;
        bool mUseColor;

        void roll();
        String mFilePathName;
        uint64_t mLastFileRoll, mRollFrequency;

        OutputStream *mStream, *mStreamToDestroy;

        Mutex mMutex;
    };
}

#endif
