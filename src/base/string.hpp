/**************************************************************************
 * Copyright 2017 ArcMist, LLC                                            *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@arcmist.com>                                    *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#ifndef ARCMIST_STRING_HPP
#define ARCMIST_STRING_HPP

#include "math.hpp"

#include <cstring>

#define ARCMIST_STRING_LOG_NAME "String"

#ifdef _WIN32
    static const char *PATH_SEPARATOR __attribute__ ((unused)) = "\\";
#else
    static const char *PATH_SEPARATOR __attribute__ ((unused)) = "/";
#endif


namespace ArcMist
{
    class String
    {
    public:

        String() { mData = NULL; }
        String(const String &pCopy)
        {
            mData = NULL;
            *this = pCopy;
        }
        String(const char *pText)
        {
            mData = NULL;
            *this = pText; // Call = operator
        }
        ~String()
        {
            if(mData != NULL)
                delete[] mData;
        }

        /*******************************************************************************************
         * Basic functions
         ******************************************************************************************/
        const char *text() const
        {
            if(mData != NULL)
                return mData;
            else
                return "";
        }

        unsigned int length() const
        {
            if(mData == NULL)
                return 0;
            else
                return std::strlen(mData);
        }

        char &operator[] (unsigned int pOffset)
        {
            return mData[pOffset];
        }

        void clear()
        {
            if(mData != NULL)
            {
                delete[] mData;
                mData = NULL;
            }
        }

        // Allocate memory for a string of specific length and return it to be written into
        char *writeAddress(unsigned int pLength);

        void writeHex(const void *pData, unsigned int pSize);

        void writeBase58(const void *pData, unsigned int pSize);

        // Append a file system path seperator and then path part to the string
        String &pathAppend(const char *pPart)
        {
            if(length() > 0 && mData[length()-1] != *PATH_SEPARATOR)
                *this += PATH_SEPARATOR;
            *this += pPart;
            return *this;
        }

        // Uses local time
        void writeFormattedTime(time_t pTime, const char *pFormat = "%F %T"); // YYYY-MM-DD HH:MM::SS

        /*******************************************************************************************
         * Compare/Evaluate Operators
         ******************************************************************************************/
        bool operator !() const { return mData == NULL; }
        operator bool() const { return mData != NULL; }

        operator const char *() const { return text(); }

        bool operator == (const char *pRight) const
        {
            if(mData == NULL)
                return pRight == NULL || *pRight == 0;
            else if(pRight == NULL || *pRight == 0)
                return false;

            return std::strcmp(mData, pRight) == 0;
        }
        bool operator == (const String &pRight) const { return *this == pRight.text(); }

        bool operator != (const char *pRight) const { return !(*this == pRight); }
        bool operator != (const String &pRight) const { return !(*this == pRight.text()); }

        bool operator < (const char *pRight) const
        {
            if(mData == NULL)
                return pRight != NULL && *pRight != 0;
            else if(pRight == NULL || *pRight == 0)
                return false;

            return std::strcmp(mData, pRight) < 0;
        }
        bool operator < (const String &pRight) const { return *this < pRight.text(); }

        bool operator <= (const char *pRight) const { return *this < pRight || *this == pRight; }
        bool operator <= (const String &pRight) const { return *this < pRight || *this == pRight; }

        bool operator > (const char *pRight) const
        {
            if(mData == NULL)
                return false;
            else if(pRight == NULL || *pRight == 0)
                return true;

            return std::strcmp(mData, pRight) > 0;
        }
        bool operator > (const String &pRight) const { return *this > pRight.text(); }

        bool operator >= (const char *pRight) const { return *this > pRight || *this == pRight; }
        bool operator >= (const String &pRight) const { return *this > pRight || *this == pRight; }

        /*******************************************************************************************
         * Modify Operators
         ******************************************************************************************/
        String &operator = (const char *pRight);
        String &operator = (const String &pRight);

        void operator += (const char *pRight);
        void operator += (const String &pRight) { *this += pRight.text(); }
        void operator += (char pRight);

        String operator + (const char *pRight) const
        {
            String result(*this);
            result += pRight;
            return result;
        }
        String operator + (const String &pRight) const
        {
            String result(*this);
            result += pRight;
            return result;
        }

        static bool test();

    private:

        char *mData;

    };

    inline bool isLetter(char pChar)
    {
        return (pChar >= 'a' && pChar <= 'z') || (pChar >= 'A' && pChar <= 'Z');
    }

    inline bool isWhiteSpace(char pChar)
    {
        return pChar == ' ' || pChar == '\t' || pChar == '\r' || pChar == '\n';
    }

    inline bool isInt(char pChar)
    {
        return pChar >= '0' && pChar <= '9';
    }

    inline bool isUpper(char pChar)
    {
        return pChar >= 'A' && pChar <= 'Z';
    }

    inline bool isLower(char pChar)
    {
        return pChar >= 'a' && pChar <= 'z';
    }

    inline char upper(char pChar)
    {
        if (isLower(pChar))
            return pChar - 32;
        else
            return pChar;
    }

    inline char lower(char pChar)
    {
        if (isUpper(pChar))
            return pChar + 32;
        else
            return pChar;
    }
}

#endif
