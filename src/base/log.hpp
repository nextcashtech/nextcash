/**************************************************************************
 * Copyright 2017 NextCash, LLC                                           *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@nextcash.tech>                                  *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#ifndef NEXTCASH_LOG_HPP
#define NEXTCASH_LOG_HPP

#include "stream.hpp"
#include "mutex.hpp"


namespace NextCash
{
    class Log
    {
    public:
        enum Level { DEBUG, VERBOSE, INFO, WARNING, ERROR, NOTIFICATION, CRITICAL };

#ifndef ANDROID
        static void setLevel(Level pLevel); // Defaults to INFO
        static void setOutput(OutputStream *pStream, bool pDeleteOnExit = false); // Defaults to std::cerr

        // Set file path name for log file. Auto rolls every day
        static void setOutputFile(const char *pFilePathName);

        // Set log file roll frequency. Only works after setOutputFile
        static void setRollFrequency(uint64_t pSeconds);
#endif

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

        static void destroy();

    private:

        static Log &log();
        static Log *mInstance;

        Log(OutputStream *pStream, const char *pDateTimeFormat);
        ~Log();

        bool startEntry(Level pLevel, const char *pName);

#ifndef ANDROID
        void internalSetOutput(OutputStream *pStream, bool pDeleteOnExit);
        void internalSetOutputFile(const char *pFilePathName);
        void internalSetRollFrequency(uint64_t pSeconds) { mRollFrequency = pSeconds; }

        void roll();

        const char *mDateTimeFormat;
        Level mLevel, mPendingEntryLevel;
        bool mUseColor;

        String mFilePathName;
        uint64_t mLastFileRoll, mRollFrequency;

        OutputStream *mStream, *mStreamToDestroy;
#endif

        MutexWithConstantName mMutex;
    };
}

#endif
