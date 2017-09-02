#include "string.hpp"

#include "arcmist/base/math.hpp"
#include "arcmist/base/endian.hpp"
#include "arcmist/base/log.hpp"
#include "arcmist/io/buffer.hpp"
#include "arcmist/io/file_stream.hpp"

#include <vector>
#include <new>


namespace ArcMist
{
    String &String::operator = (const char *pRight)
    {
        clear();
        if(pRight == NULL)
            mData = NULL;
        else
        {
            unsigned int newLength = std::strlen(pRight);
            if(newLength == 0)
                mData = NULL;
            else
            {
                mData = NULL;
                try
                {
                    mData = new char[newLength+1];
                }
                catch(std::bad_alloc &pBadAlloc)
                {
                    ArcMist::Log::addFormatted(ArcMist::Log::ERROR, ARCMIST_STRING_LOG_NAME, "Bad allocation : %s", pBadAlloc.what());
                    return *this;
                }
                catch(...)
                {
                    ArcMist::Log::add(ArcMist::Log::ERROR, ARCMIST_STRING_LOG_NAME, "Bad allocation : unknown");
                    return *this;
                }
                std::strcpy(mData, pRight);
            }
        }

        return *this;
    }

    String &String::operator = (const String &pRight)
    {
        clear();
        if(pRight.mData != NULL)
        {
            mData = NULL;
            try
            {
                mData = new char[pRight.length()+1];
            }
            catch(std::bad_alloc &pBadAlloc)
            {
                ArcMist::Log::addFormatted(ArcMist::Log::ERROR, ARCMIST_STRING_LOG_NAME, "Bad allocation : %s", pBadAlloc.what());
                return *this;
            }
            catch(...)
            {
                ArcMist::Log::add(ArcMist::Log::ERROR, ARCMIST_STRING_LOG_NAME, "Bad allocation : unknown");
                return *this;
            }
            std::strcpy(mData, pRight.mData);
        }
        else
            mData = NULL;

        return *this;
    }

    void String::operator += (const char *pRight)
    {
        if(pRight == NULL)
            return;

        unsigned int leftLength = length();
        unsigned int rightLength = std::strlen(pRight);

        if(rightLength == 0)
            return;

        char *newData = NULL;
        try
        {
            newData = new char[leftLength + rightLength + 1];
        }
        catch(std::bad_alloc &pBadAlloc)
        {
            ArcMist::Log::addFormatted(ArcMist::Log::ERROR, ARCMIST_STRING_LOG_NAME, "Bad allocation : %s", pBadAlloc.what());
            return;
        }
        catch(...)
        {
            ArcMist::Log::add(ArcMist::Log::ERROR, ARCMIST_STRING_LOG_NAME, "Bad allocation : unknown");
            return;
        }

        if(mData != NULL)
            std::strcpy(newData, mData);
        std::strcpy(newData + leftLength, pRight);

        clear();
        mData = newData;
    }

    char *String::writeAddress(unsigned int pLength)
    {
        clear();
        if(pLength == 0)
            return NULL;
        mData = NULL;
        try
        {
            mData = new char[pLength+1];
        }
        catch(std::bad_alloc &pBadAlloc)
        {
            ArcMist::Log::addFormatted(ArcMist::Log::ERROR, ARCMIST_STRING_LOG_NAME, "Bad allocation : %s", pBadAlloc.what());
            return NULL;
        }
        catch(...)
        {
            ArcMist::Log::add(ArcMist::Log::ERROR, ARCMIST_STRING_LOG_NAME, "Bad allocation : unknown");
            return NULL;
        }
        mData[pLength] = 0;
        return mData;
    }

    void String::writeHex(const void *pData, unsigned int pSize)
    {
        clear();
        mData = NULL;
        try
        {
            mData = new char[(pSize * 2) + 1];
        }
        catch(std::bad_alloc &pBadAlloc)
        {
            ArcMist::Log::addFormatted(ArcMist::Log::ERROR, ARCMIST_STRING_LOG_NAME, "Bad allocation : %s", pBadAlloc.what());
            return;
        }
        catch(...)
        {
            ArcMist::Log::add(ArcMist::Log::ERROR, ARCMIST_STRING_LOG_NAME, "Bad allocation : unknown");
            return;
        }

        mData[pSize * 2] = 0;
        const char *hexByte;

        for(unsigned int i=0;i<pSize;i++)
        {
            hexByte = Math::byteToHex[((uint8_t *)pData)[i]];
            mData[i*2] = hexByte[0];
            mData[(i*2) + 1] = hexByte[1];
        }
    }

    void String::writeBase58(const void *pData, unsigned int pSize)
    {
        // Skip & count leading zeroes.
        unsigned int leadingZeroes = 0;
        unsigned int length = 0;
        unsigned int offset = 0;

        while(offset < pSize && ((uint8_t *)pData)[offset] == 0)
        {
            offset++;
            leadingZeroes++;
        }

        // Allocate enough space in big-endian base58 representation.
        unsigned int i, size = (pSize - offset) * 138 / 100 + 1; // log(256) / log(58), rounded up.
        unsigned int value;
        std::vector<uint8_t> b58(size);

        // Process the bytes.
        while(offset < pSize)
        {
            i = 0;
            value = ((uint8_t *)pData)[offset++];

            // Apply "b58 = b58 * 256 + ch".
            for (std::vector<uint8_t>::reverse_iterator it = b58.rbegin(); (value != 0 || i < length) && (it != b58.rend()); it++, i++)
            {
                value += 256 * (*it);
                *it = value % 58;
                value /= 58;
            }

            length = i;
        }

        // Skip leading zeroes in base58 result.
        std::vector<uint8_t>::iterator it = b58.begin() + (size - length);
        while (it != b58.end() && *it == 0)
            it++;

        // Translate the result into a string.
        if(mData != NULL)
            delete[] mData;
        mData = NULL;
        try
        {
            mData = new char[leadingZeroes + (b58.end() - it) + 1];
        }
        catch(std::bad_alloc &pBadAlloc)
        {
            ArcMist::Log::addFormatted(ArcMist::Log::ERROR, ARCMIST_STRING_LOG_NAME, "Bad allocation : %s", pBadAlloc.what());
            return;
        }
        catch(...)
        {
            ArcMist::Log::add(ArcMist::Log::ERROR, ARCMIST_STRING_LOG_NAME, "Bad allocation : unknown");
            return;
        }

        mData[leadingZeroes + (b58.end() - it)] = 0;

        for(offset = 0;offset<leadingZeroes;offset++)
            mData[offset] = Math::base58Codes[0];

        while(it != b58.end())
            mData[offset++] = Math::base58Codes[*(it++)];
    }

    bool String::test()
    {
        bool result = true;

        /******************************************************************************************
         * Test that empty string allocates no memory
         ******************************************************************************************/
        String empty;
        if(empty.mData == NULL)
            Log::add(Log::INFO, ARCMIST_STRING_LOG_NAME, "Passed empty");
        else
        {
            Log::add(Log::ERROR, ARCMIST_STRING_LOG_NAME, "Failed empty");
            result = false;
        }

        /******************************************************************************************
         * Test that constructor assigns value
         ******************************************************************************************/
        String constructorValue("value");
        if(constructorValue.mData != NULL && std::strcmp(constructorValue.mData, "value") == 0)
            Log::add(Log::INFO, ARCMIST_STRING_LOG_NAME, "Passed constructor value");
        else
        {
            Log::add(Log::ERROR, ARCMIST_STRING_LOG_NAME, "Failed constructor value");
            result = false;
        }

        /******************************************************************************************
         * Test that equal constructor assigns value
         ******************************************************************************************/
        String constructorAssignValue = "value";
        if(constructorAssignValue.mData != NULL && std::strcmp(constructorAssignValue.mData, "value") == 0)
            Log::add(Log::INFO, ARCMIST_STRING_LOG_NAME, "Passed constructor assign value");
        else
        {
            Log::add(Log::ERROR, ARCMIST_STRING_LOG_NAME, "Failed constructor assign value");
            result = false;
        }

        /******************************************************************************************
         * Test that equal operator assigns value
         ******************************************************************************************/
        String operatorAssignValue;
        operatorAssignValue = constructorValue;
        if(operatorAssignValue.mData != NULL && operatorAssignValue.mData != constructorValue.mData &&
          std::strcmp(operatorAssignValue.mData, "value") == 0)
            Log::add(Log::INFO, ARCMIST_STRING_LOG_NAME, "Passed operator assign value");
        else
        {
            Log::add(Log::ERROR, ARCMIST_STRING_LOG_NAME, "Failed operator assign value");
            result = false;
        }

        /******************************************************************************************
         * Test text function
         ******************************************************************************************/
        if(std::strcmp(operatorAssignValue.text(), "value") == 0)
            Log::add(Log::INFO, ARCMIST_STRING_LOG_NAME, "Passed text function");
        else
        {
            Log::add(Log::ERROR, ARCMIST_STRING_LOG_NAME, "Failed text function");
            result = false;
        }

        /******************************************************************************************
         * Test length function
         ******************************************************************************************/
        if(operatorAssignValue.length() == 5)
            Log::add(Log::INFO, ARCMIST_STRING_LOG_NAME, "Passed length function");
        else
        {
            Log::add(Log::ERROR, ARCMIST_STRING_LOG_NAME, "Failed length function");
            result = false;
        }

        /******************************************************************************************
         * Test clear function
         ******************************************************************************************/
        operatorAssignValue.clear();
        if(operatorAssignValue.mData == NULL)
            Log::add(Log::INFO, ARCMIST_STRING_LOG_NAME, "Passed clear function");
        else
        {
            Log::add(Log::ERROR, ARCMIST_STRING_LOG_NAME, "Failed clear function");
            result = false;
        }

        /******************************************************************************************
         * Test operator ==
         ******************************************************************************************/
        String equal = "equal";
        if(equal == "equal")
            Log::add(Log::INFO, ARCMIST_STRING_LOG_NAME, "Passed operator ==");
        else
        {
            Log::add(Log::ERROR, ARCMIST_STRING_LOG_NAME, "Failed operator ==");
            result = false;
        }

        /******************************************************************************************
         * Test operator == nulls equal
         ******************************************************************************************/
        equal = "";
        if(equal == "")
            Log::add(Log::INFO, ARCMIST_STRING_LOG_NAME, "Passed operator == nulls equal");
        else
        {
            Log::add(Log::ERROR, ARCMIST_STRING_LOG_NAME, "Failed operator == nulls equal");
            result = false;
        }

        /******************************************************************************************
         * Test operator == left null right not
         ******************************************************************************************/
        equal = "";
        if(equal == "test")
        {
            Log::add(Log::ERROR, ARCMIST_STRING_LOG_NAME, "Failed operator == left null right not");
            result = false;
        }
        else
            Log::add(Log::INFO, ARCMIST_STRING_LOG_NAME, "Passed operator == left null right not");

        /******************************************************************************************
         * Test operator == right null left not
         ******************************************************************************************/
        equal = "test";
        if(equal == "")
        {
            Log::add(Log::ERROR, ARCMIST_STRING_LOG_NAME, "Failed operator == right null left not");
            result = false;
        }
        else
            Log::add(Log::INFO, ARCMIST_STRING_LOG_NAME, "Passed operator == right null left not");

        /******************************************************************************************
         * Test operator !=
         ******************************************************************************************/
        if(equal != "not equal")
            Log::add(Log::INFO, ARCMIST_STRING_LOG_NAME, "Passed operator !=");
        else
        {
            Log::add(Log::ERROR, ARCMIST_STRING_LOG_NAME, "Failed operator !=");
            result = false;
        }

        /******************************************************************************************
         * Test operator bool
         ******************************************************************************************/
        String testBool = "123";
        if(testBool)
            Log::add(Log::INFO, ARCMIST_STRING_LOG_NAME, "Passed operator bool");
        else
        {
            Log::add(Log::ERROR, ARCMIST_STRING_LOG_NAME, "Failed operator bool");
            result = false;
        }

        /******************************************************************************************
         * Test operator bool false
         ******************************************************************************************/
        testBool = "";
        if(testBool)
        {
            Log::add(Log::ERROR, ARCMIST_STRING_LOG_NAME, "Failed operator bool false");
            result = false;
        }
        else
            Log::add(Log::INFO, ARCMIST_STRING_LOG_NAME, "Passed operator bool false");

        /******************************************************************************************
         * Test operator !
         ******************************************************************************************/
        testBool = "";
        if(!testBool)
            Log::add(Log::INFO, ARCMIST_STRING_LOG_NAME, "Passed operator !");
        else
        {
            Log::add(Log::ERROR, ARCMIST_STRING_LOG_NAME, "Failed operator !");
            result = false;
        }

        /******************************************************************************************
         * Test operator ! false
         ******************************************************************************************/
        testBool = "123";
        if(!testBool)
        {
            Log::add(Log::ERROR, ARCMIST_STRING_LOG_NAME, "Failed operator ! false");
            result = false;
        }
        else
            Log::add(Log::INFO, ARCMIST_STRING_LOG_NAME, "Passed operator ! false");

        /******************************************************************************************
         * Test operator +
         ******************************************************************************************/
        String left = "left";
        String right = "right";
        String add = left + right;
        if(add == "leftright")
            Log::add(Log::INFO, ARCMIST_STRING_LOG_NAME, "Passed operator +");
        else
        {
            Log::add(Log::ERROR, ARCMIST_STRING_LOG_NAME, "Failed operator +");
            result = false;
        }

        /******************************************************************************************
         * Test operator +=
         ******************************************************************************************/
        String append = left;
        append += right;
        if(append == "leftright")
            Log::add(Log::INFO, ARCMIST_STRING_LOG_NAME, "Passed operator +=");
        else
        {
            Log::add(Log::ERROR, ARCMIST_STRING_LOG_NAME, "Failed operator +=");
            result = false;
        }

        /******************************************************************************************
         * Test operator >
         ******************************************************************************************/
        left = "bcd";
        right = "abc";
        if(left > right)
            Log::add(Log::INFO, ARCMIST_STRING_LOG_NAME, "Passed operator >");
        else
        {
            Log::add(Log::ERROR, ARCMIST_STRING_LOG_NAME, "Failed operator >");
            result = false;
        }

        /******************************************************************************************
         * Test operator > left null
         ******************************************************************************************/
        left = "";
        right = "abc";
        if(left > right)
        {
            Log::add(Log::ERROR, ARCMIST_STRING_LOG_NAME, "Failed operator > left null");
            result = false;
        }
        else
            Log::add(Log::INFO, ARCMIST_STRING_LOG_NAME, "Passed operator > left null");

        /******************************************************************************************
         * Test operator > right null
         ******************************************************************************************/
        left = "bcd";
        right = "";
        if(left > right)
            Log::add(Log::INFO, ARCMIST_STRING_LOG_NAME, "Passed operator > right null");
        else
        {
            Log::add(Log::ERROR, ARCMIST_STRING_LOG_NAME, "Failed operator > right null");
            result = false;
        }

        /******************************************************************************************
         * Test operator <
         ******************************************************************************************/
        left = "abc";
        right = "bcd";
        if(left < right)
            Log::add(Log::INFO, ARCMIST_STRING_LOG_NAME, "Passed operator <");
        else
        {
            Log::add(Log::ERROR, ARCMIST_STRING_LOG_NAME, "Failed operator <");
            result = false;
        }

        /******************************************************************************************
         * Test operator < left null
         ******************************************************************************************/
        left = "";
        right = "abc";
        if(left < right)
            Log::add(Log::INFO, ARCMIST_STRING_LOG_NAME, "Passed operator < left null");
        else
        {
            Log::add(Log::ERROR, ARCMIST_STRING_LOG_NAME, "Failed operator < left null");
            result = false;
        }

        /******************************************************************************************
         * Test operator < right null
         ******************************************************************************************/
        left = "bcd";
        right = "";
        if(left < right)
        {
            Log::add(Log::ERROR, ARCMIST_STRING_LOG_NAME, "Failed operator < right null");
            result = false;
        }
        else
            Log::add(Log::INFO, ARCMIST_STRING_LOG_NAME, "Passed operator < right null");

        /******************************************************************************************
         * Test base58 test 1
         ******************************************************************************************/
        String base58;
        uint8_t base58Data[10] = { 0x00, 0x00, 0x4e, 0x12, 0x9f, 0xa3, 0x39, 0xb5, 0xc1, 0x76 };
        base58.writeBase58(base58Data, 10);

        if(base58 == "11E4QQELDrmnD")
            Log::addFormatted(Log::INFO, ARCMIST_STRING_LOG_NAME, "Passed base58 test 1 : %s", base58.text());
        else
        {
            Log::addFormatted(Log::ERROR, ARCMIST_STRING_LOG_NAME, "Failed base58 test 1 : %s", base58.text());
            result = false;
        }

        /******************************************************************************************
         * Test base58 test 2
         ******************************************************************************************/
        uint8_t base58Data2[21] = { 0x00, 0x5a, 0x1f, 0xc5, 0xdd, 0x9e, 0x6f, 0x03, 0x81, 0x9f, 0xca, 0x94, 0xa2, 0xd8, 0x96, 0x69, 0x46, 0x96, 0x67, 0xf9, 0xa0 };
        base58.writeBase58(base58Data2, 21);

        if(base58 == "12FpmoFq5cpWVRp4dCgkYB3HiTzx7") // "x19DXstMaV43WpYg4ceREiiTv2UntmoiA9j")
            Log::addFormatted(Log::INFO, ARCMIST_STRING_LOG_NAME, "Passed base58 test 2 : %s", base58.text());
        else
        {
            Log::addFormatted(Log::ERROR, ARCMIST_STRING_LOG_NAME, "Failed base58 test 2 : %s", base58.text());
            result = false;
        }

        /******************************************************************************************
         * Test base58 test 3
         ******************************************************************************************/
        uint8_t base58Data3[3] = { 'a', 'b', 'c' };
        base58.writeBase58(base58Data3, 3);

        if(base58 == "ZiCa")
            Log::addFormatted(Log::INFO, ARCMIST_STRING_LOG_NAME, "Passed base58 test 3 : %s", base58.text());
        else
        {
            Log::addFormatted(Log::ERROR, ARCMIST_STRING_LOG_NAME, "Failed base58 test 3 : %s", base58.text());
            result = false;
        }

        return result;
    }
}
