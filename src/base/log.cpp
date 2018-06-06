/**************************************************************************
 * Copyright 2017 NextCash, LLC                                           *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@nextcash.tech>                                  *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#include "log.hpp"

#include "thread.hpp"
#include "file_stream.hpp"

#include <cstdarg>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <iomanip>
#ifdef ANDROID
#include <android/log.h>
#endif


namespace NextCash
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

#ifndef ANDROID
    void rollFile(const char *pFilePathName)
    {
        if(!fileExists(pFilePathName))
            return;

        String dateTimeString;
        dateTimeString.writeFormattedTime(std::time(NULL), "%Y%m%d.%H%M");
        String newFilePathName;
        const char *lastDot = NULL;
        const char *ptr = pFilePathName;
        while(*ptr)
        {
            if(*ptr == '.')
                lastDot = ptr;
            ptr++;
        }

        if(lastDot == NULL)
        {
            newFilePathName = pFilePathName;
            newFilePathName += ".";
            newFilePathName += dateTimeString;
        }
        else
        {
            // Copy the part before the dot
            ptr = pFilePathName;
            while(ptr != lastDot)
                newFilePathName += *ptr++;

            // Put the date string in
            newFilePathName += ".";
            newFilePathName += dateTimeString;

            // Copy the part after the dot
            while(*ptr)
                newFilePathName += *ptr++;
        }

        std::rename(pFilePathName, newFilePathName);
    }

    void Log::roll()
    {
        mLastFileRoll = std::time(NULL);
        if(!mFilePathName)
            return; // Only roll when there is a file name

        delete mStreamToDestroy;
        rollFile(mFilePathName);

        // Reopen file
        mStreamToDestroy = new FileOutputStream(mFilePathName);
        mStream = mStreamToDestroy;
    }
#endif

    Log::Log(OutputStream *pStream, const char *pDateTimeFormat) : mMutex("Log")
    {
#ifdef ANDROID
        return;
#else
        mDateTimeFormat = pDateTimeFormat;
        mLevel = INFO;
        mPendingEntryLevel = INFO;
        mStreamToDestroy = NULL;
        mStream = NULL;
        mUseColor = false;
        mLastFileRoll = 0;
        mRollFrequency = 86400; // Daily

        internalSetOutput(pStream, false);
#endif
    }

    Log::~Log()
    {
#ifdef ANDROID
        return;
#else
        if(mStreamToDestroy != NULL)
            delete mStreamToDestroy;
#endif
    }

    void Log::destroy()
    {
        if(mInstance != NULL)
        {
            delete mInstance;
            mInstance = NULL;
        }
    }

#ifndef ANDROID
    void Log::setLevel(Level pLevel)
    {
        log().mLevel = pLevel;
    }

    void Log::setOutput(OutputStream *pStream, bool pDeleteOnExit)
    {
        log().internalSetOutput(pStream, pDeleteOnExit);
    }

    void Log::setOutputFile(const char *pFilePathName)
    {
        log().internalSetOutputFile(pFilePathName);
    }

    void Log::setRollFrequency(uint64_t pSeconds)
    {
        log().internalSetRollFrequency(pSeconds);
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

    void Log::internalSetOutputFile(const char *pFilePathName)
    {
        if(mStreamToDestroy != NULL)
            delete mStreamToDestroy;
        mFilePathName = pFilePathName;
        mStreamToDestroy = NULL;
        mStream = NULL;
        mUseColor = false;
        roll();
    }

    inline void startForegroundColor(OutputStream *pStream, unsigned int pColor)
    {
        pStream->writeByte(0x1b);
        pStream->writeFormatted("[38;5;%dm", pColor);
    }

    inline void startBackgroundColor(OutputStream *pStream, unsigned int pColor)
    {
        pStream->writeByte(0x1b);
        pStream->writeFormatted("[48;5;%dm", pColor);
    }

    inline void endColor(OutputStream *pStream)
    {
        pStream->writeByte(0x1b);
        pStream->writeFormatted("[0m");
    }
#endif

    static const unsigned int BLACK      = 232;
    static const unsigned int WHITE      = 252;
    static const unsigned int GREY       = 243;
    static const unsigned int LIGHT_GREY = 247;
    static const unsigned int RED        = 160;
    static const unsigned int BLUE       = 27;
    static const unsigned int LIGHT_BLUE = 39;
    static const unsigned int GREEN      = 28;
    static const unsigned int YELLOW     = 190;
    static const unsigned int PURPLE     = 92;
    static const unsigned int ORANGE     = 166;
    static const unsigned int TEAL       = 81;

    bool Log::startEntry(Level pLevel, const char *pName)
    {
#ifdef ANDROID
        lock();
        return true;
#else
        if(mLevel > pLevel)
            return false;

        // Output data/time stamp
        String dateTimeString;
        dateTimeString.writeFormattedTime(std::time(NULL), mDateTimeFormat);
        unsigned int entryColor = WHITE;

        lock();

        if(mFilePathName && std::time(NULL) - mLastFileRoll > mRollFrequency)
            roll();

        if(mUseColor)
            startForegroundColor(mStream, TEAL);
        mStream->writeString(dateTimeString);
        if(mUseColor)
            mStream->writeString("\033[0m");
        mStream->writeByte(' ');

        // Output entry level
        switch(pLevel)
        {
            case DEBUG:
                if(mUseColor)
                    startForegroundColor(mStream, GREY);
                entryColor = GREY;
                mStream->writeString("DEBUG\t");
                break;
            case VERBOSE:
                if(mUseColor)
                    startForegroundColor(mStream, LIGHT_GREY);
                entryColor = LIGHT_GREY;
                mStream->writeString("VERBOSE");
                if(mUseColor)
                    endColor(mStream);
                mStream->writeByte('\t');
                break;
            case INFO:
                if(mUseColor)
                    startForegroundColor(mStream, WHITE);
                entryColor = WHITE;
                mStream->writeString("INFO");
                if(mUseColor)
                    endColor(mStream);
                mStream->writeByte('\t');
                break;
            case WARNING:
                if(mUseColor)
                    startForegroundColor(mStream, YELLOW);
                entryColor = YELLOW;
                mStream->writeString("WARNING");
                if(mUseColor)
                    endColor(mStream);
                mStream->writeByte('\t');
                break;
            case ERROR:
                if(mUseColor)
                    startForegroundColor(mStream, RED);
                entryColor = RED;
                mStream->writeString("ERROR");
                if(mUseColor)
                    endColor(mStream);
                mStream->writeByte('\t');
                break;
            case NOTIFICATION:
                if(mUseColor)
                    startForegroundColor(mStream, ORANGE);
                entryColor = ORANGE;
                mStream->writeString("NOTIFICATION");
                if(mUseColor)
                    endColor(mStream);
                mStream->writeByte('\t');
                break;
            case CRITICAL:
                if(mUseColor)
                    startForegroundColor(mStream, RED);
                entryColor = RED;
                mStream->writeString("CRITICAL");
                if(mUseColor)
                    endColor(mStream);
                mStream->writeByte('\t');
                break;
            default:
                mStream->writeByte('\t');
                break;
        }

        // Output thread ID
        if(mUseColor)
            startForegroundColor(mStream, LIGHT_BLUE);
        mStream->writeString(Thread::currentName());
        mStream->writeByte('\t');
        if(mUseColor)
            endColor(mStream);

        // Output entry name
        if(pName)
        {
            if(mUseColor)
                startForegroundColor(mStream, BLUE);
            mStream->writeString(pName);
            if(mUseColor)
                endColor(mStream);
        }
        mStream->writeByte('\t');

        if(mUseColor)
            startForegroundColor(mStream, entryColor);
#endif

        return true;
    }

#ifdef ANDROID
    int androidLogLevel(Log::Level pLevel)
    {
        switch(pLevel)
        {
        case Log::DEBUG:
            return ANDROID_LOG_VERBOSE; // DEBUG is higher than verbose in android, but not here
        case Log::VERBOSE:
            return ANDROID_LOG_DEBUG;
        default:
        case Log::INFO:
            return ANDROID_LOG_INFO;
        case Log::WARNING:
            return ANDROID_LOG_WARN;
        case Log::ERROR:
            return ANDROID_LOG_ERROR;
        case Log::NOTIFICATION:
            return ANDROID_LOG_INFO;
        case Log::CRITICAL:
            return ANDROID_LOG_FATAL;
        }
    }
#endif

    void Log::add(Level pLevel, const char *pName, const char *pEntry)
    {
        Log &theLog = log();
        if(theLog.startEntry(pLevel, pName))
        {
#ifdef ANDROID
            __android_log_print(androidLogLevel(pLevel), pName, "%s", pEntry);
#else
            theLog.mStream->writeString(pEntry);
            theLog.mStream->writeByte('\n');
            if(theLog.mUseColor)
                endColor(theLog.mStream);
            theLog.mStream->flush();
#endif
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
#ifdef ANDROID
            __android_log_vprint(androidLogLevel(pLevel), pName, pFormatting, args);
#else
            theLog.mStream->writeFormattedList(pFormatting, args);
            theLog.mStream->writeByte('\n');
            if(theLog.mUseColor)
                endColor(theLog.mStream);
            theLog.mStream->flush();
#endif
            va_end(args);
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
#ifdef ANDROID
            __android_log_vprint(androidLogLevel(DEBUG), pName, pFormatting, args);
#else
            theLog.mStream->writeFormattedList(pFormatting, args);
            theLog.mStream->writeByte('\n');
            if(theLog.mUseColor)
                endColor(theLog.mStream);
            theLog.mStream->flush();
#endif
            va_end(args);
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
#ifdef ANDROID
            __android_log_vprint(androidLogLevel(VERBOSE), pName, pFormatting, args);
#else
            theLog.mStream->writeFormattedList(pFormatting, args);
            theLog.mStream->writeByte('\n');
            if(theLog.mUseColor)
                endColor(theLog.mStream);
            theLog.mStream->flush();
#endif
            va_end(args);
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
#ifdef ANDROID
            __android_log_vprint(androidLogLevel(INFO), pName, pFormatting, args);
#else
            theLog.mStream->writeFormattedList(pFormatting, args);
            theLog.mStream->writeByte('\n');
            if(theLog.mUseColor)
                endColor(theLog.mStream);
            theLog.mStream->flush();
#endif
            va_end(args);
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
#ifdef ANDROID
            __android_log_vprint(androidLogLevel(WARNING), pName, pFormatting, args);
#else
            theLog.mStream->writeFormattedList(pFormatting, args);
            theLog.mStream->writeByte('\n');
            if(theLog.mUseColor)
                endColor(theLog.mStream);
            theLog.mStream->flush();
#endif
            va_end(args);
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
#ifdef ANDROID
            __android_log_vprint(androidLogLevel(ERROR), pName, pFormatting, args);
#else
            theLog.mStream->writeFormattedList(pFormatting, args);
            theLog.mStream->writeByte('\n');
            if(theLog.mUseColor)
                endColor(theLog.mStream);
            theLog.mStream->flush();
#endif
            va_end(args);
            theLog.unlock();
        }
    }

    void Log::addHex(Level pLevel, const char *pName, const char *pDescription, InputStream *pStream, stream_size pSize)
    {
        Log &theLog = log();
        if(theLog.startEntry(pLevel, pName))
        {
#ifdef ANDROID
            __android_log_print(androidLogLevel(pLevel), pName, "%s", pDescription);
            __android_log_print(androidLogLevel(pLevel), pName, "%s", pStream->readHexString(pSize).text());
#else
            theLog.mStream->writeString(pDescription);
            theLog.mStream->writeByte('\n');
            theLog.mStream->writeAsHex(pStream, pSize);
            theLog.mStream->writeByte('\n');
            if(theLog.mUseColor)
                endColor(theLog.mStream);
            theLog.mStream->flush();
#endif
            theLog.unlock();
        }
    }
}
