/**************************************************************************
 * Copyright 2017 NextCash, LLC                                            *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@nextcash.com>                                    *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#ifndef NEXTCASH_MATH_HPP
#define NEXTCASH_MATH_HPP

#include <cstdint>
#include <cstring>
#include <cmath>
#include <cstdlib>
#include <ctime>

#define DOUBLE_BIAS 1023
#define RADIANS_TO_DEGREES 57.29747
#define DEGREES_TO_RADIANS 0.0174528


namespace NextCash
{
    // Seconds since epoch
    inline int32_t getTime()
    {
        return std::time(NULL);
    }

    namespace Math
    {
        inline double square(double pValue)
        {
            return pValue * pValue;
        }

        inline double squareRoot(double pValue)
        {
            return std::sqrt(pValue);
        }

        static bool sRandomSeeded = false;

        inline void seedRandom()
        {
            std::srand(getTime());
        }

        inline uint32_t randomInt()
        {
            if(!sRandomSeeded)
            {
                seedRandom();
                sRandomSeeded = true;
            }
            return rand();
        }

        inline uint32_t randomInt(uint32_t pMax)
        {
            if(!sRandomSeeded)
            {
                seedRandom();
                sRandomSeeded = true;
            }
            return rand() % pMax;
        }

        inline uint64_t randomLong()
        {
            std::srand(getTime());
            return ((uint64_t)rand() << 32) + (uint64_t)rand();
        }

        // Convert 4 bit value to hex character
        inline char nibbleToHex(uint8_t pValue)
        {
            if(pValue < 10)
                return '0' + pValue;
            else
                return 'a' - 10 + pValue;
        }

        inline uint8_t hexToNibble(char pCharacter)
        {
            if(pCharacter >= '0' && pCharacter <= '9')
                return pCharacter - '0';
            else if(pCharacter >= 'a' && pCharacter <= 'f')
                return pCharacter - 'a' + 10;
            else if(pCharacter >= 'A' && pCharacter <= 'F')
                return pCharacter - 'A' + 10;
            else
                return 0;
        }

        // Convert 8 bit value to two hex characters
        static const char *byteToHex[256] __attribute__ ((unused)) = // GCC thinks this is unused, but it isn't
        {
            "00", "01", "02", "03", "04", "05", "06", "07", "08", "09", "0a", "0b", "0c", "0d", "0e", "0f",
            "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "1a", "1b", "1c", "1d", "1e", "1f",
            "20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "2a", "2b", "2c", "2d", "2e", "2f",
            "30", "31", "32", "33", "34", "35", "36", "37", "38", "39", "3a", "3b", "3c", "3d", "3e", "3f",
            "40", "41", "42", "43", "44", "45", "46", "47", "48", "49", "4a", "4b", "4c", "4d", "4e", "4f",
            "50", "51", "52", "53", "54", "55", "56", "57", "58", "59", "5a", "5b", "5c", "5d", "5e", "5f",
            "60", "61", "62", "63", "64", "65", "66", "67", "68", "69", "6a", "6b", "6c", "6d", "6e", "6f",
            "70", "71", "72", "73", "74", "75", "76", "77", "78", "79", "7a", "7b", "7c", "7d", "7e", "7f",
            "80", "81", "82", "83", "84", "85", "86", "87", "88", "89", "8a", "8b", "8c", "8d", "8e", "8f",
            "90", "91", "92", "93", "94", "95", "96", "97", "98", "99", "9a", "9b", "9c", "9d", "9e", "9f",
            "a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7", "a8", "a9", "aa", "ab", "ac", "ad", "ae", "af",
            "b0", "b1", "b2", "b3", "b4", "b5", "b6", "b7", "b8", "b9", "ba", "bb", "bc", "bd", "be", "bf",
            "c0", "c1", "c2", "c3", "c4", "c5", "c6", "c7", "c8", "c9", "ca", "cb", "cc", "cd", "ce", "cf",
            "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "d8", "d9", "da", "db", "dc", "dd", "de", "df",
            "e0", "e1", "e2", "e3", "e4", "e5", "e6", "e7", "e8", "e9", "ea", "eb", "ec", "ed", "ee", "ef",
            "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7", "f8", "f9", "fa", "fb", "fc", "fd", "fe", "ff"
        };

        static char base58Codes[59] __attribute__ ((unused)) = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

        static char base32Codes[33] __attribute__ ((unused)) = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";

        // Reverse the bits in a byte
        static uint8_t reflect8[256] __attribute__ ((unused)) =
        {
            0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0, 0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
            0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8, 0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
            0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4, 0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
            0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec, 0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
            0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2, 0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
            0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea, 0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
            0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6, 0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
            0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee, 0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
            0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1, 0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
            0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9, 0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
            0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5, 0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
            0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed, 0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
            0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3, 0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
            0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb, 0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
            0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7, 0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
            0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef, 0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff
        };

        // Returns true if the bit at the specified offset is set
        inline bool bit(uint8_t pValue, int pOffset)
        {
            switch(pOffset)
            {
            case 0:
                return pValue & 0x80;
                break;
            case 1:
                return pValue & 0x40;
                break;
            case 2:
                return pValue & 0x20;
                break;
            case 3:
                return pValue & 0x10;
                break;
            case 4:
                return pValue & 0x08;
                break;
            case 5:
                return pValue & 0x04;
                break;
            case 6:
                return pValue & 0x02;
                break;
            case 7:
                return pValue & 0x01;
                break;
            default:
                break;
            }

            return false;
        }

        static const uint8_t bitMask[9] = {0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF};

        inline uint32_t rotateLeft(uint32_t x, int n)
        {
            return (x << n) | (x >> (32 - n));
        }

        inline uint32_t rotateRight(uint32_t x, int n)
        {
            return rotateLeft(x, 32 - n);
        }

        inline uint64_t rotateLeft64(uint64_t x, int n)
        {
            return (x << n) | (x >> (64 - n));
        }

        inline uint64_t rotateRight64(uint64_t x, int n)
        {
            return rotateLeft64(x, 64 - n);
        }

        class DoubleMap
        {
        public:

            DoubleMap() {cValue[0] = 0;cValue[1] = 0;}
            DoubleMap(double iValue) {*this = iValue;}

            unsigned int sign() {return(cValue[1] >> 31);}
            unsigned int exponent() {return((cValue[1] << 1) >> 21);}
            unsigned int msFraction() {return(cValue[1] << 12) >> 20;}
            unsigned int lsFraction() {return cValue[0];}
            void operator =(double pValue) {memcpy(cValue, &pValue, 8);}
            bool isZero()  {return exponent() == 0 && msFraction() == 0 && lsFraction() == 0;}
            bool isPositive() {return sign() == 0;}
            bool isNormalized() {return !(exponent() == 0 &&(msFraction() != 0 || lsFraction() != 0));}
            bool isInfinity() {return exponent() == 2047 && msFraction() == 0 && lsFraction() == 0;}
            bool isNAN() {return exponent() == 2047 &&(msFraction() != 0 || lsFraction() != 0);}
    //        bool toString(char *pBuffer, unsigned int pSize);

        private:

            unsigned int cValue[2];
        };

        inline int absoluteValue(int pValue)
        {
            if(pValue < 0)
                return -pValue;
            else
                return pValue;
        }

        inline double absoluteValue(double pValue)
        {
            if(pValue < 0.0)
                return -pValue;
            else
                return pValue;
        }

        inline bool isEven(int pValue)
        {
            if(pValue % 2)
                return false;
            else
                return true;
        }

        inline bool isOdd(int pValue)
        {
            if(pValue % 2)
                return true;
            else
                return false;
        }

        inline double sine(double pValue)
        {
            return std::sin(pValue * DEGREES_TO_RADIANS);
        }

        inline double cosine(double pValue)
        {
            return std::cos(pValue * DEGREES_TO_RADIANS);
        }

        inline double round(double pValue, int pDecimalPlaces)
        {
            double multiplier = std::pow(10.0, pDecimalPlaces);
            double newValue;

            std::modf(multiplier * pValue, &newValue);

            if((multiplier * pValue) - newValue >= 0.5)
                newValue++;
            else if((multiplier * pValue) - newValue <= -0.5)
                newValue--;
            return (double)newValue / multiplier;
        }

        inline double truncate(double pValue, int pDecimalPlaces)
        {
            double multiplier = std::pow(10.0, pDecimalPlaces);
            double newValue;

            std::modf(multiplier * pValue, &newValue);

            //if((multiplier * pValue) - newValue >= 0.5)
            //    newValue++;
            //else if((multiplier * pValue) - newValue <= -0.5)
            //    newValue--;
            return (double)newValue / multiplier;
        }

        inline bool contains(double pLeft, double pTop, double pRight, double pBottom, double pX, double pY)
        {
            if(pX < pLeft)
                return false;
            if(pX > pRight)
                return false;
            if(pTop > pBottom)
            {
                if(pY > pTop)
                    return false;
                if(pY < pBottom)
                    return false;
            }
            else
            {
                if(pY < pTop)
                    return false;
                if(pY > pBottom)
                    return false;
            }
            return true;
        }

        class Rectangle
        {
        public:
            Rectangle() {clear();}
            Rectangle(const Rectangle &iCopy) {*this=iCopy;}
            Rectangle(double iLeft, double iTop, double iRight, double iBottom)
            {
                left   = iLeft;
                top    = iTop;
                right  = iRight;
                bottom = iBottom;
            }
            ~Rectangle() {}

            double left, top, right, bottom;

            void clear()
            {
                left   = 0;
                top    = 0;
                right  = 0;
                bottom = 0;
            }

            Rectangle &operator =(const Rectangle &pRight)
            {
                left   = pRight.left;
                top    = pRight.top;
                right  = pRight.right;
                bottom = pRight.bottom;
                return *this;
            }

            double width() const {return absoluteValue(right - left);}
            double height() const {return absoluteValue(top - bottom);}

            void orient()
            {
                double swap;

                if(left > right)
                {
                    swap  = left;
                    left  = right;
                    right = swap;
                }

                if(bottom > top)
                {
                    swap   = bottom;
                    bottom = top;
                    top    = swap;
                }
            }

            void include(const Rectangle &pSub)
            {
                if(left > pSub.left)
                    left = pSub.left;
                if(right < pSub.right)
                    right = pSub.right;
                if(top < pSub.top)
                    top = pSub.top;
                if(bottom > pSub.bottom)
                    bottom = pSub.bottom;
            }
        };

        inline bool contains(const Rectangle &pRectangle, double pX, double pY)
        {
            return contains(pRectangle.left, pRectangle.top, pRectangle.right, pRectangle.bottom, pX, pY);
        }

        class Point
        {
        public:
            Point() {clear();}
            Point(const Point &iCopy) {*this=iCopy;}
            Point(double iX, double iY)
            {
                x = iX;
                y = iY;
            }
            ~Point() {}

            double x, y;

            void clear()
            {
                x = 0.0;
                y = 0.0;
            }

            Point &operator =(const Point &pRight)
            {
                x = pRight.x;
                y = pRight.y;
                return *this;
            }

            bool operator ==(const Point &pRight)
            {
                return x == pRight.x && y == pRight.y;
            }
        };

        inline bool contains(const Rectangle &pRectangle, const Point &pPoint)
        {
            return contains(pRectangle.left, pRectangle.top, pRectangle.right, pRectangle.bottom, pPoint.x, pPoint.y);
        }

        class DoubleRange
        {
        public:
            DoubleRange()
            {
                bottom = 0.0;
                top    = 0.0;
            }
            DoubleRange(double iBottom, double iTop)
            {
                bottom = iBottom;
                top    = iTop;
            }
            DoubleRange(const DoubleRange &iCopy) {*this = iCopy;}
            ~DoubleRange() {}

            DoubleRange &operator =(const DoubleRange &pRight)
            {
                bottom = pRight.bottom;
                top    = pRight.top;
                return *this;
            }

            double bottom, top;
        };

        inline int powerOf2LargerThan(int pValue)
        {
            int result = 2;

            while(result < pValue)
                result *= 2;

            return result;
        }
    }
}

#endif
