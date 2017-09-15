/**************************************************************************
 * Copyright 2017 ArcMist, LLC                                            *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@arcmist.com>                                    *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#ifndef ARCMIST_ENDIAN_HPP
#define ARCMIST_ENDIAN_HPP

#include <cstdint>

// Determine endian from compiler (currently only supports g++)
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    #define ARCMIST_LITTLE_ENDIAN
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
    #define ARCMIST_BIG_ENDIAN
#elif __BYTE_ORDER__ == __ORDER_PDP_ENDIAN__
    /* bytes in 16-bit words are laid out in a little-endian fashion, whereas the 16-bit subwords
     * of a 32-bit quantity are laid out in big-endian fashion.
     */
#endif

namespace ArcMist
{
    namespace Endian
    {
        enum Type { BIG, LITTLE };

#ifdef ARCMIST_LITTLE_ENDIAN
        static const Type sSystemType = LITTLE;
#endif
#ifdef ARCMIST_BIG_ENDIAN
        static const Type sSystemType = BIG;
#endif

        inline void reverse(void *pData, int pSize)
        {
            uint8_t swap;
            uint8_t *ptr =(uint8_t *)pData;
            int i = 0;
            int half = pSize / 2;

            while(i < half)
            {
                swap   = ptr[i];
                ptr[i] = ptr[pSize - i - 1];
                ptr[pSize - i - 1] = swap;
                i++;
            }
        }

        inline double reverse(double pValue)
        {
            reverse((uint8_t *)&pValue, sizeof(double));
            return pValue;
        }

        inline uint64_t reverse(uint64_t pValue)
        {
            reverse((uint8_t *)&pValue, sizeof(uint64_t));
            return pValue;
        }

        inline int64_t reverse(int64_t pValue)
        {
            reverse((uint8_t *)&pValue, sizeof(uint64_t));
            return pValue;
        }

        inline uint32_t reverse(uint32_t pValue)
        {
            reverse((uint8_t *)&pValue, sizeof(uint32_t));
            return pValue;
        }

        inline int32_t reverse(int32_t pValue)
        {
            reverse((uint8_t *)&pValue, sizeof(int32_t));
            return pValue;
        }

        inline uint16_t reverse(uint16_t pValue)
        {
            reverse((uint8_t *)&pValue, sizeof(uint16_t));
            return pValue;
        }

        inline int16_t reverse(int16_t pValue)
        {
            reverse((uint8_t *)&pValue, sizeof(int16_t));
            return pValue;
        }

        inline void convert(void *pData, int pSize, Type pEndian)
        {
#ifdef ARCMIST_LITTLE_ENDIAN
            if(pEndian != LITTLE)
                reverse(pData, pSize);
#endif
#ifdef ARCMIST_BIG_ENDIAN
            if(pEndian != BIG)
                reverse(pData, pSize);
#endif
        }

        inline double convert(double pValue, Type pEndian)
        {
            convert((uint8_t *)&pValue, sizeof(double), pEndian);
            return pValue;
        }

        inline uint64_t convert(uint64_t pValue, Type pEndian)
        {
            convert((uint8_t *)&pValue, sizeof(uint64_t), pEndian);
            return pValue;
        }

        inline int64_t convert(int64_t pValue, Type pEndian)
        {
            convert((uint8_t *)&pValue, sizeof(int64_t), pEndian);
            return pValue;
        }

        inline uint32_t convert(uint32_t pValue, Type pEndian)
        {
            convert((uint8_t *)&pValue, sizeof(uint32_t), pEndian);
            return pValue;
        }

        inline int32_t convert(int32_t pValue, Type pEndian)
        {
            convert((uint8_t *)&pValue, sizeof(int32_t), pEndian);
            return pValue;
        }

        inline uint16_t convert(uint16_t pValue, Type pEndian)
        {
            convert((uint8_t *)&pValue, sizeof(uint16_t), pEndian);
            return pValue;
        }

        inline int16_t convert(int16_t pValue, Type pEndian)
        {
            convert((uint8_t *)&pValue, sizeof(int16_t), pEndian);
            return pValue;
        }

        // Reverses the endian based on 4 byte values
        inline void swap4(uint8_t *pValue, unsigned int pLength)
        {
            unsigned int i = 0;
            uint8_t temp;

            while(i < pLength)
            {
                temp = pValue[i];
                pValue[i] = pValue[i + 3];
                pValue[i + 3] = temp;
                temp = pValue[i + 1];
                pValue[i + 1] = pValue[i + 2];
                pValue[i + 2] = temp;
                i+=4;
            }
        }

        // Reverses the endian based on 2 byte values
        inline void swap2(uint8_t *pValue, unsigned int pLength)
        {
            unsigned int i = 0;
            uint8_t temp;

            while(i < pLength)
            {
                temp = pValue[i];
                pValue[i] = pValue[i + 1];
                pValue[i + 1] = temp;
                i+=2;
            }
        }
    }
}

#endif
