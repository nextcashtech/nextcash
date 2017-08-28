#ifndef ARCMIST_LOG_HPP
#define ARCMIST_LOG_HPP

#include "arcmist/io/stream.hpp"
#include "arcmist/base/mutex.hpp"


namespace ArcMist
{
    class Log
    {
    public:
        enum Level { DEBUG, VERBOSE, INFO, WARNING, ERROR, NOTIFICATION, CRITICAL };

        static void setLevel(Level pLevel); // Defaults to INFO
        static void setOutput(OutputStream *pStream); // Defaults to std::cerr

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
        static void addHex(Level pLevel, const char *pName, const char *pDescription, InputStream *pStream, unsigned int pSize);

        void lock() { mMutex.lock(); }
        void unlock() { mMutex.unlock(); }

    private:

        static Log &log();
        static Log *mInstance;
        static void destroy();

        Log(OutputStream *pStream, const char *pDateTimeFormat);
        ~Log();
        void internalSetOutput(OutputStream *pStream);
        bool startEntry(Level pLevel, const char *pName);

        const char *mDateTimeFormat;
        Level mLevel, mPendingEntryLevel;
        bool mUseColor;

        OutputStream *mStream, *mStreamToDestroy;

        Mutex mMutex;
    };
}

#endif
