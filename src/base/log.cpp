#include "log.hpp"

#include "arcmist/base/thread.hpp"
#include "arcmist/io/file_stream.hpp"

#include <cstdarg>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <iomanip>


namespace ArcMist
{
    Log *Log::mInstance = NULL;

    Log &Log::log()
    {
        if(!mInstance)
        {
            mInstance = new Log(NULL, "%F %T"); // YYYY-MM-DD HH:MM::SS
            std::atexit(Log::destroy);
        }

        return *mInstance;
    }

    Log::Log(OutputStream *pStream, const char *pDateTimeFormat) : mMutex("Log")
    {
        mDateTimeFormat = pDateTimeFormat;
        mLevel = INFO;
        mPendingEntryLevel = INFO;
        mStreamToDestroy = NULL;
        mStream = NULL;
        mUseColor = false;

        internalSetOutput(pStream, false);
    }

    Log::~Log()
    {
        if(mStreamToDestroy != NULL)
            delete mStreamToDestroy;
    }

    void Log::destroy()
    {
        if(mInstance != NULL)
        {
            delete mInstance;
            mInstance = NULL;
        }
    }

    void Log::setLevel(Level pLevel)
    {
        log().mLevel = pLevel;
    }

    void Log::setOutput(OutputStream *pStream, bool pDeleteOnExit)
    {
        log().internalSetOutput(pStream, pDeleteOnExit);
    }

    void Log::internalSetOutput(OutputStream *pStream, bool pDeleteOnExit)
    {
        if(mStreamToDestroy != NULL)
            delete mStreamToDestroy;

        if(pStream != NULL)
        {
            if(pDeleteOnExit)
                mStreamToDestroy = pStream;
            else
                mStreamToDestroy = NULL;
            mStream = pStream;
            mUseColor = false;
        }
        else
        {
            mStreamToDestroy = new FileOutputStream(std::cerr);
            mStream = mStreamToDestroy;
            mUseColor = true;
        }
    }

    bool Log::startEntry(Level pLevel, const char *pName)
    {
        if(mLevel > pLevel)
            return false;

        // Output data/time stamp
        time_t rawtime;
        struct tm *timeinfo;
        char *dateTimeString = new char[64];
        std::time(&rawtime);
        timeinfo = std::localtime(&rawtime);
        std::strftime(dateTimeString, 64, mDateTimeFormat, timeinfo);

        lock();

        if(mUseColor)
            mStream->writeString("\033[1;35m");
        mStream->writeString(dateTimeString);
        if(mUseColor)
            mStream->writeString("\033[0m");
        mStream->writeByte(' ');
        delete[] dateTimeString;

        // Output entry level
        switch(pLevel)
        {
            case DEBUG:
                mStream->writeString("DEBUG\t");
                break;
            case VERBOSE:
                if(mUseColor)
                    mStream->writeString("\033[0;34m");
                mStream->writeString("VERBOSE");
                if(mUseColor)
                    mStream->writeString("\033[0m");
                mStream->writeByte('\t');
                break;
            case INFO:
                if(mUseColor)
                    mStream->writeString("\033[0;32m");
                mStream->writeString("INFO");
                if(mUseColor)
                    mStream->writeString("\033[0m");
                mStream->writeByte('\t');
                break;
            case WARNING:
                if(mUseColor)
                    mStream->writeString("\033[1;33m");
                mStream->writeString("WARNING");
                if(mUseColor)
                    mStream->writeString("\033[0m");
                mStream->writeByte('\t');
                break;
            case ERROR:
                if(mUseColor)
                    mStream->writeString("\033[0;31m");
                mStream->writeString("ERROR");
                if(mUseColor)
                    mStream->writeString("\033[0m");
                mStream->writeByte('\t');
                break;
            default:
                mStream->writeByte('\t');
                break;
        }

        // Output thread ID
        if(mUseColor)
            mStream->writeString("\033[0;35m");
        mStream->writeString(Thread::currentName());
        mStream->writeByte('\t');
        if(mUseColor)
            mStream->writeString("\033[0m");

        // Output entry name
        if(pName)
        {
            if(mUseColor)
                mStream->writeString("\033[0;36m");
            mStream->writeString(pName);
            if(mUseColor)
                mStream->writeString("\033[0m");
        }
        mStream->writeByte('\t');

        return true;
    }

    void Log::add(Level pLevel, const char *pName, const char *pEntry)
    {
        Log &theLog = log();
        if(theLog.startEntry(pLevel, pName))
        {
            theLog.mStream->writeString(pEntry);
            theLog.mStream->writeByte('\n');
            theLog.mStream->flush();
            theLog.unlock();
        }
    }

    void Log::addFormatted(Level pLevel, const char *pName, const char *pFormatting, ...)
    {
        Log &theLog = log();
        if(theLog.startEntry(pLevel, pName))
        {
            va_list args;
            va_start(args, pFormatting);
            theLog.mStream->writeFormattedList(pFormatting, args);
            va_end(args);
            theLog.mStream->writeByte('\n');
            theLog.mStream->flush();
            theLog.unlock();
        }
    }

    void Log::debugFormatted(const char *pName, const char *pFormatting, ...)
    {
        Log &theLog = log();
        if(theLog.startEntry(DEBUG, pName))
        {
            va_list args;
            va_start(args, pFormatting);
            theLog.mStream->writeFormattedList(pFormatting, args);
            va_end(args);
            theLog.mStream->writeByte('\n');
            theLog.mStream->flush();
            theLog.unlock();
        }
    }

    void Log::verboseFormatted(const char *pName, const char *pFormatting, ...)
    {
        Log &theLog = log();
        if(theLog.startEntry(VERBOSE, pName))
        {
            va_list args;
            va_start(args, pFormatting);
            theLog.mStream->writeFormattedList(pFormatting, args);
            va_end(args);
            theLog.mStream->writeByte('\n');
            theLog.mStream->flush();
            theLog.unlock();
        }
    }

    void Log::infoFormatted(const char *pName, const char *pFormatting, ...)
    {
        Log &theLog = log();
        if(theLog.startEntry(INFO, pName))
        {
            va_list args;
            va_start(args, pFormatting);
            theLog.mStream->writeFormattedList(pFormatting, args);
            va_end(args);
            theLog.mStream->writeByte('\n');
            theLog.mStream->flush();
            theLog.unlock();
        }
    }

    void Log::warningFormatted(const char *pName, const char *pFormatting, ...)
    {
        Log &theLog = log();
        if(theLog.startEntry(WARNING, pName))
        {
            va_list args;
            va_start(args, pFormatting);
            theLog.mStream->writeFormattedList(pFormatting, args);
            va_end(args);
            theLog.mStream->writeByte('\n');
            theLog.mStream->flush();
            theLog.unlock();
        }
    }

    void Log::errorFormatted(const char *pName, const char *pFormatting, ...)
    {
        Log &theLog = log();
        if(theLog.startEntry(ERROR, pName))
        {
            va_list args;
            va_start(args, pFormatting);
            theLog.mStream->writeFormattedList(pFormatting, args);
            va_end(args);
            theLog.mStream->writeByte('\n');
            theLog.mStream->flush();
            theLog.unlock();
        }
    }

    void Log::addHex(Level pLevel, const char *pName, const char *pDescription, InputStream *pStream, unsigned int pSize)
    {
        Log &theLog = log();
        if(theLog.startEntry(pLevel, pName))
        {
            theLog.mStream->writeString(pDescription);
            theLog.mStream->writeByte('\n');
            theLog.mStream->writeAsHex(pStream, pSize);
            theLog.mStream->writeByte('\n');
            theLog.mStream->flush();
            theLog.unlock();
        }
    }
}
