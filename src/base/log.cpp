/**************************************************************************
 * Copyright 2017 ArcMist, LLC                                            *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@arcmist.com>                                    *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
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

    void rollFile(const char *pFilePathName)
    {
        if(!fileExists(pFilePathName))
            return;

        time_t rawtime;
        struct tm *timeinfo;
        char *dateTimeString = new char[64];
        std::time(&rawtime);
        timeinfo = std::localtime(&rawtime);
        std::strftime(dateTimeString, 64, "%Y%m%d.%H%M", timeinfo);

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

    Log::Log(OutputStream *pStream, const char *pDateTimeFormat) : mMutex("Log")
    {
        mDateTimeFormat = pDateTimeFormat;
        mLevel = INFO;
        mPendingEntryLevel = INFO;
        mStreamToDestroy = NULL;
        mStream = NULL;
        mUseColor = false;
        mLastFileRoll = 0;
        mRollFrequency = 86400; // Daily

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
        if(mLevel > pLevel)
            return false;

        // Output data/time stamp
        time_t rawtime;
        struct tm *timeinfo;
        char *dateTimeString = new char[64];
        std::time(&rawtime);
        timeinfo = std::localtime(&rawtime);
        std::strftime(dateTimeString, 64, mDateTimeFormat, timeinfo);
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
        delete[] dateTimeString;

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

        return true;
    }

    void Log::add(Level pLevel, const char *pName, const char *pEntry)
    {
        Log &theLog = log();
        if(theLog.startEntry(pLevel, pName))
        {
            theLog.mStream->writeString(pEntry);
            theLog.mStream->writeByte('\n');
            if(theLog.mUseColor)
                endColor(theLog.mStream);
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
            if(theLog.mUseColor)
                endColor(theLog.mStream);
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
            if(theLog.mUseColor)
                endColor(theLog.mStream);
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
            if(theLog.mUseColor)
                endColor(theLog.mStream);
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
            if(theLog.mUseColor)
                endColor(theLog.mStream);
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
            if(theLog.mUseColor)
                endColor(theLog.mStream);
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
            if(theLog.mUseColor)
                endColor(theLog.mStream);
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
            if(theLog.mUseColor)
                endColor(theLog.mStream);
            theLog.mStream->flush();
            theLog.unlock();
        }
    }
}
