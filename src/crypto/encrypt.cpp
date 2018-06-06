/**************************************************************************
 * Copyright 2018 NextCash, LLC                                           *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@nextcash.tech>                                  *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#include "encrypt.hpp"

#include "log.hpp"

#include <cstring>


#define NEXTCASH_ENCRYPT_LOG_NAME "Encrypt"


namespace NextCash
{
    unsigned int keySize(Encryption::Type pType)
    {
        switch(pType)
        {
        case Encryption::AES_128:
            return 16;
        case Encryption::AES_192:
            return 24;
        case Encryption::AES_256:
            return 32;
        default:
            return 0;
        }
    }

    unsigned int blockSize(Encryption::Type pType, Encryption::BlockMethod pBlockMethod)
    {
        switch(pType)
        {
        case Encryption::AES_128:
        case Encryption::AES_192:
        case Encryption::AES_256:
            return 16;
        default:
            return 0;
        }
    }

    void xorBlock(const uint8_t *pKey, unsigned int pKeySize, uint8_t *pBlock,
      unsigned int pBlockSize)
    {
        stream_size j = 0;
        for(stream_size i=0;i<pBlockSize;++i)
        {
            pBlock[i] ^= pKey[j++];
            if(j >= pKeySize)
                j = 0;
        }
    }

    namespace Rijndael
    {
        static uint8_t sCon [256] = { 0x01, 0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36, 0x6c, 0xd8, 0xab, 0x4d,
                                      0x9a, 0x2f, 0x5e, 0xbc, 0x63, 0xc6, 0x97, 0x35, 0x6a, 0xd4, 0xb3, 0x7d, 0xfa, 0xef, 0xc5, 0x91,
                                      0x39, 0x72, 0xe4, 0xd3, 0xbd, 0x61, 0xc2, 0x9f, 0x25, 0x4a, 0x94, 0x33, 0x66, 0xcc, 0x83, 0x1d,
                                      0x3a, 0x74, 0xe8, 0xcb, 0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36, 0x6c,
                                      0xd8, 0xab, 0x4d, 0x9a, 0x2f, 0x5e, 0xbc, 0x63, 0xc6, 0x97, 0x35, 0x6a, 0xd4, 0xb3, 0x7d, 0xfa,
                                      0xef, 0xc5, 0x91, 0x39, 0x72, 0xe4, 0xd3, 0xbd, 0x61, 0xc2, 0x9f, 0x25, 0x4a, 0x94, 0x33, 0x66,
                                      0xcc, 0x83, 0x1d, 0x3a, 0x74, 0xe8, 0xcb, 0x8d, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,
                                      0x1b, 0x36, 0x6c, 0xd8, 0xab, 0x4d, 0x9a, 0x2f, 0x5e, 0xbc, 0x63, 0xc6, 0x97, 0x35, 0x6a, 0xd4,
                                      0xb3, 0x7d, 0xfa, 0xef, 0xc5, 0x91, 0x39, 0x72, 0xe4, 0xd3, 0xbd, 0x61, 0xc2, 0x9f, 0x25, 0x4a,
                                      0x94, 0x33, 0x66, 0xcc, 0x83, 0x1d, 0x3a, 0x74, 0xe8, 0xcb, 0x8d, 0x01, 0x02, 0x04, 0x08, 0x10,
                                      0x20, 0x40, 0x80, 0x1b, 0x36, 0x6c, 0xd8, 0xab, 0x4d, 0x9a, 0x2f, 0x5e, 0xbc, 0x63, 0xc6, 0x97,
                                      0x35, 0x6a, 0xd4, 0xb3, 0x7d, 0xfa, 0xef, 0xc5, 0x91, 0x39, 0x72, 0xe4, 0xd3, 0xbd, 0x61, 0xc2,
                                      0x9f, 0x25, 0x4a, 0x94, 0x33, 0x66, 0xcc, 0x83, 0x1d, 0x3a, 0x74, 0xe8, 0xcb, 0x8d, 0x01, 0x02,
                                      0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36, 0x6c, 0xd8, 0xab, 0x4d, 0x9a, 0x2f, 0x5e, 0xbc,
                                      0x63, 0xc6, 0x97, 0x35, 0x6a, 0xd4, 0xb3, 0x7d, 0xfa, 0xef, 0xc5, 0x91, 0x39, 0x72, 0xe4, 0xd3,
                                      0xbd, 0x61, 0xc2, 0x9f, 0x25, 0x4a, 0x94, 0x33, 0x66, 0xcc, 0x83, 0x1d, 0x3a, 0x74, 0xe8, 0xcb };

        //                   Rijndael S-Box
        //    | 0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
        // ---|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|
        // 00 |63 7c 77 7b f2 6b 6f c5 30 01 67 2b fe d7 ab 76
        // 10 |ca 82 c9 7d fa 59 47 f0 ad d4 a2 af 9c a4 72 c0
        // 20 |b7 fd 93 26 36 3f f7 cc 34 a5 e5 f1 71 d8 31 15
        // 30 |04 c7 23 c3 18 96 05 9a 07 12 80 e2 eb 27 b2 75
        // 40 |09 83 2c 1a 1b 6e 5a a0 52 3b d6 b3 29 e3 2f 84
        // 50 |53 d1 00 ed 20 fc b1 5b 6a cb be 39 4a 4c 58 cf
        // 60 |d0 ef aa fb 43 4d 33 85 45 f9 02 7f 50 3c 9f a8
        // 70 |51 a3 40 8f 92 9d 38 f5 bc b6 da 21 10 ff f3 d2
        // 80 |cd 0c 13 ec 5f 97 44 17 c4 a7 7e 3d 64 5d 19 73
        // 90 |60 81 4f dc 22 2a 90 88 46 ee b8 14 de 5e 0b db
        // a0 |e0 32 3a 0a 49 06 24 5c c2 d3 ac 62 91 95 e4 79
        // b0 |e7 c8 37 6d 8d d5 4e a9 6c 56 f4 ea 65 7a ae 08
        // c0 |ba 78 25 2e 1c a6 b4 c6 e8 dd 74 1f 4b bd 8b 8a
        // d0 |70 3e b5 66 48 03 f6 0e 61 35 57 b9 86 c1 1d 9e
        // e0 |e1 f8 98 11 69 d9 8e 94 9b 1e 87 e9 ce 55 28 df
        // f0 |8c a1 89 0d bf e6 42 68 41 99 2d 0f b0 54 bb 16

        static uint8_t sBox [256] = { 0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
                                      0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
                                      0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
                                      0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
                                      0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
                                      0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
                                      0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
                                      0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
                                      0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
                                      0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
                                      0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
                                      0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
                                      0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
                                      0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
                                      0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
                                      0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16 };

        static uint8_t sInverseSBox [256] = { 0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
                                              0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
                                              0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
                                              0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
                                              0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
                                              0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
                                              0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
                                              0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
                                              0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
                                              0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
                                              0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
                                              0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
                                              0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
                                              0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
                                              0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
                                              0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d };
    }

    class AES
    {
    public:

        AES(unsigned int pKeySize, const uint8_t *pKey, unsigned int pKeyLength);
        ~AES()
        {
            if(mExpandedKey != NULL)
                delete[] mExpandedKey;
            if(mPreviousState != NULL)
                delete[] mPreviousState;
        }

        void encryptBlock(uint8_t *pData);
        bool decryptBlock(uint8_t *pData);

    private:

        unsigned int mKeySize;
        unsigned int mBlockByteCount;
        unsigned int mRowLength;
        unsigned int mRoundCount;
        unsigned int mKeyOffset;
        uint8_t *mState;
        uint8_t *mPreviousState;
        uint8_t *mExpandedKey;

        inline int gridOffset(unsigned int pRow, unsigned int pColumn, unsigned int pRowLength)
        {
            return (pColumn * pRowLength) + pRow;
        }

        void keyScheduleCore(uint8_t *pData, unsigned int pIteration)
        {
            uint8_t swap = pData[0];

            swap = pData [0];
            pData[0] = pData[1];
            pData[1] = pData[2];
            pData[2] = pData[3];
            pData[3] = swap;

            for(unsigned int i=0;i<4;i++)
                pData[i] = Rijndael::sBox[pData[i]];

            pData[0] ^= Rijndael::sCon[pIteration];
        }

        void expandKey(const uint8_t *pKey, unsigned int pKeyLength);

        void addRoundKey()
        {
            unsigned int r, c;

            for(r=0;r<4;r++)
                for(c=0;c<mRowLength;c++)
                    mState[(r * mRowLength) + c] ^= mExpandedKey[mKeyOffset++];
        }

        void shiftRows()
        {
            unsigned int shift, shiftTo, shiftFrom, row;

            std::memcpy(mPreviousState, mState, mBlockByteCount);

            if(mRowLength < 7)
            {
                for(row=1,shift=1;row<4;row++,shift++)
                    for(shiftTo=0,shiftFrom=shift;shiftTo<mRowLength;shiftTo++,shiftFrom++)
                        mState[gridOffset (row, shiftTo, mRowLength)] =
                          mPreviousState[gridOffset(row, shiftFrom%mRowLength, mRowLength)];
            }
            else if(mRowLength == 7)
            {
                for(row=1,shift=0;row<4;row++)
                {
                    switch(shift)
                    {
                    case 0:
                        shift = 1;
                        break;
                    case 1:
                        shift = 2;
                        break;
                    case 2:
                        shift = 4;
                        break;
                    default:
                        break;
                    }

                    for(shiftTo=0,shiftFrom=shift;shiftTo<mRowLength;shiftTo++,shiftFrom++)
                        mState[gridOffset (row, shiftTo, mRowLength)] =
                          mPreviousState[gridOffset(row, shiftFrom%mRowLength, mRowLength)];
                }

            }
            else if(mRowLength == 8)
            {
                for(row=1,shift=0;row<4;row++)
                {
                    switch(shift)
                    {
                    case 0:
                        shift = 1;
                        break;
                    case 1:
                        shift = 3;
                        break;
                    case 3:
                        shift = 4;
                        break;
                    default:
                        break;
                    }

                    for(shiftTo=0,shiftFrom=shift;shiftTo<mRowLength;shiftTo++,shiftFrom++)
                        mState[gridOffset (row, shiftTo, mRowLength)] =
                          mPreviousState[gridOffset (row, shiftFrom%mRowLength, mRowLength)];
                }
            }
        }

        uint8_t multiply(uint8_t a, uint8_t b)
        {
            uint8_t result = 0;
            uint8_t i;
            uint8_t hi_bit_set;

            for(i=0;i<8;i++)
            {
                if(b & 0x01)
                    result ^= a;
                hi_bit_set = (a & 0x80);
                a <<= 1;
                if(hi_bit_set)
                    a ^= 0x1b; /* x^8 + x^4 + x^3 + x + 1 */
                b >>= 1;
            }

            return result;
        }

        void mixColumns()
        {
            unsigned int row, col;

            std::memcpy(mPreviousState, mState, mBlockByteCount);

            for(col=0;col<mRowLength;col++)
                for(row=0;row<4;row++)
                    mState[gridOffset (row, col, mRowLength)] =
                      multiply(0x02, mPreviousState [gridOffset (row, col, mRowLength)]) ^
                      multiply(0x03, mPreviousState[gridOffset((row + 1) % 4, col, mRowLength)]) ^
                      mPreviousState[gridOffset((row + 2) % 4, col, mRowLength)] ^
                      mPreviousState[gridOffset((row + 3) % 4, col, mRowLength)];
        }

        void subtractRoundKey()
        {
            unsigned int r, c;
            unsigned int otherKeyOffset = mKeyOffset;

            for(r=0;r<4;r++)
                for(c=0;c<mRowLength;c++)
                    mState[(r * mRowLength) + c] ^= mExpandedKey[otherKeyOffset++];

            mKeyOffset -= mBlockByteCount;
        }

        bool inverseShiftRows()
        {
            unsigned int shift, shiftTo, shiftFrom, row;

            std::memcpy(mPreviousState, mState, mBlockByteCount);

            if(mRowLength < 7)
            {
                for(row=1,shift=1;row<4;row++,shift++)
                    for(shiftTo=0,shiftFrom=shift;shiftTo<mRowLength;shiftTo++,shiftFrom++)
                        mState[gridOffset(row, shiftFrom%mRowLength, mRowLength)] =
                          mPreviousState[gridOffset(row, shiftTo, mRowLength)];
            }
            else if(mRowLength == 7)
            {
                for(row=1,shift=0;row<4;row++)
                {
                    switch(shift)
                    {
                    case 0:
                        shift = 1;
                        break;
                    case 1:
                        shift = 2;
                        break;
                    case 2:
                        shift = 4;
                        break;
                    default:
                        return false;
                    }

                    for(shiftTo=0,shiftFrom=shift;shiftTo<mRowLength;shiftTo++,shiftFrom++)
                        mState[gridOffset(row, shiftFrom%mRowLength, mRowLength)] =
                          mPreviousState[gridOffset(row, shiftTo, mRowLength)];
                }

            }
            else if (mRowLength == 8)
            {
                for(row=1,shift=0;row<4;row++)
                {
                    switch(shift)
                    {
                    case 0:
                        shift = 1;
                        break;
                    case 1:
                        shift = 3;
                        break;
                    case 3:
                        shift = 4;
                        break;
                        break;
                    default:
                        return false;
                    }

                    for(shiftTo=0,shiftFrom=shift;shiftTo<mRowLength;shiftTo++,shiftFrom++)
                        mState[gridOffset(row, shiftFrom%mRowLength, mRowLength)] =
                          mPreviousState[gridOffset(row, shiftTo, mRowLength)];
                }
            }
            else
                return false;

            return true;
        }

        void inverseMixColumns()
        {
            unsigned int row, col;

            std::memcpy(mPreviousState, mState, mBlockByteCount);

            for(col=0;col<mRowLength;col++)
                for(row=0;row<4;row++)
                    mState[gridOffset(row, col, mRowLength)] =
                      multiply(0x0e, mPreviousState[gridOffset(row, col, mRowLength)]) ^
                      multiply(0x0b, mPreviousState[gridOffset((row + 1) % 4, col, mRowLength)]) ^
                      multiply(0x0d, mPreviousState[gridOffset((row + 2) % 4, col, mRowLength)]) ^
                      multiply(0x09, mPreviousState[gridOffset((row + 3) % 4, col, mRowLength)]);
        }
    };

    AES::AES(unsigned int pKeySize, const uint8_t *pKey, unsigned int pKeyLength)
    {
        mKeySize = pKeySize;
        mBlockByteCount = 16;
        mRowLength = mBlockByteCount / 4;

        unsigned int roundIndex;
        if(mRowLength > (mKeySize / 4))
            roundIndex = mRowLength;
        else
            roundIndex = (mKeySize / 4);

        mRoundCount = roundIndex + 6;
        mKeyOffset = 0;
        mExpandedKey = NULL;
        mState = NULL;
        mPreviousState = new uint8_t[mBlockByteCount];

        expandKey(pKey, pKeyLength);
    }

    void AES::expandKey(const uint8_t *pKey, unsigned int pKeyLength)
    {
        if(mExpandedKey != NULL)
            delete[] mExpandedKey;

        mExpandedKey = new uint8_t[((mRoundCount + 2) * mBlockByteCount)];
        std::memcpy(mExpandedKey, pKey, pKeyLength);
        if(mKeySize > pKeyLength)
            std::memset(mExpandedKey + pKeyLength, 0, mKeySize - pKeyLength);

        uint8_t currentCore[4];
        unsigned int i = 2, j, k, m;
        unsigned int keyCount = mKeySize;

        for(j=0;j<4;j++)
            currentCore[j] = mExpandedKey[mKeySize - 4 + j];

        while(keyCount < (mRoundCount + 1) * mBlockByteCount)
        {
            keyScheduleCore(currentCore, i++);

            for(j=0;j<4;j++)
            {
                for(k=0;k<4;k++)
                    currentCore[k] ^= mExpandedKey[keyCount + k - mKeySize];

                std::memcpy(mExpandedKey + keyCount, currentCore, 4);
                keyCount += 4;
            }

            if(mKeySize == 32) // AES 256
            {
                for(j=0;j<4;j++)
                    currentCore[j] = Rijndael::sBox[currentCore [j]];

                for(k=0;k<4;k++)
                    currentCore[k] ^= mExpandedKey[keyCount + k - mKeySize];

                std::memcpy(mExpandedKey + keyCount, currentCore, 4);
                keyCount += 4;
            }

            if(mKeySize == 32)
                m = 3;
            else if(mKeySize == 24)
                m = 2;
            else
                m = 0;

            while(m)
            {
                for(k=0;k<4;k++)
                    currentCore[k] ^= mExpandedKey[keyCount + k - mKeySize];

                std::memcpy(mExpandedKey + keyCount, currentCore, 4);
                keyCount += 4;

                m--;
            }
        }
    }

    void AES::encryptBlock(uint8_t *pData)
    {
        unsigned int i;

        mState = pData;
        mKeyOffset = 0;

        // Initial Round
        // Add Round Key
        addRoundKey();

        // Intermediate Rounds
        for(unsigned int roundOffset=1;roundOffset<mRoundCount;roundOffset++)
        {
            // Sub Bytes
            for(i=0;i<mBlockByteCount;i++)
                mState[i] = Rijndael::sBox[mState[i]];

            // Shift Rows
            shiftRows();

            // Mix Columns
            mixColumns();

            // Add Round Key
            addRoundKey();
        }

        // Final Round
        // Sub Bytes
        for(i=0;i<mBlockByteCount;i++)
            mState[i] = Rijndael::sBox[mState[i]];

        // Shift Rows
        shiftRows();

        // Add Round Key
        addRoundKey();

        mState = NULL;
    }

    bool AES::decryptBlock(uint8_t *pData)
    {
        unsigned int i;

        mState = pData;
        mKeyOffset = ((1 + mRoundCount) * mBlockByteCount) - mBlockByteCount;

        // Initial Round

        // Subtract Round Key
        subtractRoundKey();

        // Intermediate Rounds
        for(unsigned int roundOffset=1;roundOffset<mRoundCount;roundOffset++)
        {
            // Inverse Shift Rows
            if(!inverseShiftRows())
                return false;

            // Inverse Sub Bytes
            for(i=0;i<mBlockByteCount;i++)
                mState[i] = Rijndael::sInverseSBox[mState[i]];

            // Subtract Round Key
            subtractRoundKey();

            // Inverse Mix Columns
            inverseMixColumns();
        }

        // Final Round

        // Inverse Shift Rows
        if(!inverseShiftRows())
            return false;

        // Inverse Sub Bytes
        for(i=0;i<mBlockByteCount;i++)
            mState[i] = Rijndael::sInverseSBox[mState[i]];

        // Subtract Round Key
        subtractRoundKey();

        mState = NULL;
        return true;
    }

    Encryptor::Encryptor(OutputStream *pOutput, Encryption::Type pType,
      Encryption::BlockMethod pBlockMethod)
    {
        mOutput = pOutput;
        mType = pType;
        mBlockMethod = pBlockMethod;
        mByteCount = 0;
        mAES = NULL;
        mBlockSize = blockSize(pType, pBlockMethod);
        mBlock = new uint8_t[mBlockSize];
    }

    Encryptor::~Encryptor()
    {
        finalize();
        delete[] mBlock;
    }

    void Encryptor::setup(const uint8_t *pKey, int pKeyLength, const uint8_t *pInitializationVector,
      int pInitializationVectorLength)
    {
        mByteCount = 0;
        mData.clear();

        if(mAES != NULL)
            delete mAES;

        mAES = new AES(keySize(mType), pKey, pKeyLength);

        mVector.clear();
        mVector.resize(pInitializationVectorLength);
        std::memcpy(mVector.data(), pInitializationVector, pInitializationVectorLength);
    }

    void Encryptor::process()
    {
        while(mData.remaining() >= mBlockSize)
        {
            mData.read(mBlock, mBlockSize);

            // Perform the Block Cipher
            switch (mBlockMethod)
            {
            case Encryption::CBC:
                xorBlock(mVector.data(), mVector.size(), mBlock, mBlockSize);
                break;
            case Encryption::ECB:
            default:
                break;
            }

            // Encrypt the block
            switch (mType)
            {
            case Encryption::AES_128:
            case Encryption::AES_192:
            case Encryption::AES_256:
                mAES->encryptBlock(mBlock);
                break;
            }

            // Perform the Block Cipher
            switch (mBlockMethod)
            {
            case Encryption::CBC:
                std::memcpy(mVector.data(), mBlock, mVector.size());
                break;
            case Encryption::ECB:
            default:
                break;
            }

            mOutput->write(mBlock, mBlockSize);
        }

        mData.flush();
    }

    void Encryptor::finalize()
    {
        if(mData.remaining() > 0)
        {
            while(mData.remaining() < mBlockSize)
                mData.writeByte(0);
            process();
        }
    }

    void Encryptor::write(const void *pInput, stream_size pSize)
    {
        mData.write(pInput, pSize);
        mByteCount += pSize;
        process();
    }

    Decryptor::Decryptor(InputStream *pInput, Encryption::Type pType,
      Encryption::BlockMethod pBlockMethod)
    {
        mInput = pInput;
        mType = pType;
        mBlockMethod = pBlockMethod;
        mAES = NULL;
        mBlockSize = blockSize(pType, pBlockMethod);
        mBlock = new uint8_t[mBlockSize];
        mEncryptedBlock = new uint8_t[mBlockSize];
    }

    Decryptor::~Decryptor()
    {
        delete[] mBlock;
        delete[] mEncryptedBlock;
    }

    void Decryptor::setup(const uint8_t *pKey, int pKeyLength,
      const uint8_t *pInitializationVector, int pInitializationVectorLength)
    {
        mData.clear();

        if(mAES != NULL)
            delete mAES;

        mAES = new AES(keySize(mType), pKey, pKeyLength);

        mVector.clear();
        mVector.resize(pInitializationVectorLength);
        std::memcpy(mVector.data(), pInitializationVector, pInitializationVectorLength);
    }

    void Decryptor::read(void *pOutput, stream_size pSize)
    {
        while(mData.remaining() < pSize)
        {
            if(mInput->remaining() == 0)
                break;
            else if(mInput->remaining() < mBlockSize)
            {
                // Zeroize end of block
                std::memset(mBlock + mInput->remaining(), 0, mBlockSize - mInput->remaining());
                mInput->read(mBlock, mInput->remaining());
            }
            else
                mInput->read(mBlock, mBlockSize);

            std::memcpy(mEncryptedBlock, mBlock, mBlockSize);

            // Decrypt a block
            switch (mType)
            {
            case Encryption::AES_128:
            case Encryption::AES_192:
            case Encryption::AES_256:
                mAES->decryptBlock(mBlock);
                break;
            }

            // Perform the Block Cipher
            switch (mBlockMethod)
            {
            case Encryption::CBC:
                xorBlock(mVector.data(), mVector.size(), mBlock, mBlockSize);
                std::memcpy(mVector.data(), mEncryptedBlock, mVector.size());
                break;
            case Encryption::ECB:
            default:
                break;
            }

            mData.write(mBlock, mBlockSize);
        }

        mData.read(pOutput, pSize);
    }

    bool Encryption::test()
    {
        Log::add(NextCash::Log::INFO, NEXTCASH_ENCRYPT_LOG_NAME,
          "------------- Starting Encryption Tests -------------");

        bool result = true;
        Buffer data, encryptedData, key, initVector, correctOutput;
        Encryptor aes128ECB(&encryptedData, Encryption::AES_128, Encryption::ECB);
        Encryptor aes192ECB(&encryptedData, Encryption::AES_192, Encryption::ECB);
        Encryptor aes256ECB(&encryptedData, Encryption::AES_256, Encryption::ECB);
        Encryptor aes128CBC(&encryptedData, Encryption::AES_128, Encryption::CBC);
        Encryptor aes192CBC(&encryptedData, Encryption::AES_192, Encryption::CBC);
        Encryptor aes256CBC(&encryptedData, Encryption::AES_256, Encryption::CBC);
        Decryptor unaes128ECB(&encryptedData, Encryption::AES_128, Encryption::ECB);
        Decryptor unaes192ECB(&encryptedData, Encryption::AES_192, Encryption::ECB);
        Decryptor unaes256ECB(&encryptedData, Encryption::AES_256, Encryption::ECB);
        Decryptor unaes128CBC(&encryptedData, Encryption::AES_128, Encryption::CBC);
        Decryptor unaes192CBC(&encryptedData, Encryption::AES_192, Encryption::CBC);
        Decryptor unaes256CBC(&encryptedData, Encryption::AES_256, Encryption::CBC);

        /******************************************************************************************
         * AES Document Test Vector
         ******************************************************************************************/
        data.clear();
        data.writeHex("3243f6a8885a308d313198a2e0370734");

        key.clear();
        key.writeHex("2b7e151628aed2a6abf7158809cf4f3c");
        initVector.clear();
        data.setReadOffset(0);
        encryptedData.clear();

        aes128ECB.setup(key.startPointer(), key.length(), initVector.startPointer(), initVector.length());
        aes128ECB.writeStream(&data, data.remaining());
        aes128ECB.finalize();

        correctOutput.clear();
        correctOutput.writeHex("3925841d02dc09fbdc118597196a0b32");

        if(encryptedData == correctOutput)
            Log::add(Log::INFO, NEXTCASH_ENCRYPT_LOG_NAME, "Passed AES 128 ECB");
        else
        {
            Log::add(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Failed AES 128 ECB");
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Correct Output : %s",
              correctOutput.readHexString(correctOutput.length()).text());
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Result Output  : %s",
              encryptedData.readHexString(encryptedData.length()).text());
            result = false;
        }

        unaes128ECB.setup(key.startPointer(), key.length(), initVector.startPointer(), initVector.length());
        encryptedData.setReadOffset(0);
        data.setReadOffset(0);
        correctOutput.clear();
        correctOutput.writeStream(&data, data.length());
        data.clear();
        data.writeStream(&unaes128ECB, unaes128ECB.length());

        if(data == correctOutput)
            Log::add(Log::INFO, NEXTCASH_ENCRYPT_LOG_NAME, "Passed Decrypt AES 128 ECB");
        else
        {
            Log::add(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Failed Decrypt AES 128 ECB");
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Correct Output : %s",
              correctOutput.readHexString(correctOutput.length()).text());
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Result Output  : %s",
              data.readHexString(data.length()).text());
            result = false;
        }

        key.clear();
        key.writeHex("2b7e151628aed2a6abf7158809cf4f3c762e7160f38b4da5");
        initVector.clear();
        data.setReadOffset(0);
        encryptedData.clear();

        aes192ECB.setup(key.startPointer(), key.length(), initVector.startPointer(), initVector.length());
        aes192ECB.writeStream(&data, data.remaining());
        aes192ECB.finalize();

        correctOutput.clear();
        correctOutput.writeHex("f9fb29aefc384a250340d833b87ebc00");

        if(encryptedData == correctOutput)
            Log::add(Log::INFO, NEXTCASH_ENCRYPT_LOG_NAME, "Passed AES 192 ECB");
        else
        {
            Log::add(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Failed AES 192 ECB");
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Correct Output : %s",
              correctOutput.readHexString(correctOutput.length()).text());
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Result Output  : %s",
              encryptedData.readHexString(encryptedData.length()).text());
            result = false;
        }

        unaes192ECB.setup(key.startPointer(), key.length(), initVector.startPointer(), initVector.length());
        encryptedData.setReadOffset(0);
        data.setReadOffset(0);
        correctOutput.clear();
        correctOutput.writeStream(&data, data.length());
        data.clear();
        data.writeStream(&unaes192ECB, unaes192ECB.length());

        if(data == correctOutput)
            Log::add(Log::INFO, NEXTCASH_ENCRYPT_LOG_NAME, "Passed Decrypt AES 192 ECB");
        else
        {
            Log::add(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Failed Decrypt AES 192 ECB");
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Correct Output : %s",
              correctOutput.readHexString(correctOutput.length()).text());
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Result Output  : %s",
              data.readHexString(data.length()).text());
            result = false;
        }

        key.clear();
        key.writeHex("2b7e151628aed2a6abf7158809cf4f3c762e7160f38b4da56a784d9045190cfe");
        initVector.clear();
        data.setReadOffset(0);
        encryptedData.clear();

        aes256ECB.setup(key.startPointer(), key.length(), initVector.startPointer(), initVector.length());
        aes256ECB.writeStream(&data, data.remaining());
        aes256ECB.finalize();

        correctOutput.clear();
        correctOutput.writeHex("1a6e6c2c662e7da6501ffb62bc9e93f3");

        if(encryptedData == correctOutput)
            Log::add(Log::INFO, NEXTCASH_ENCRYPT_LOG_NAME, "Passed AES 256 ECB");
        else
        {
            Log::add(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Failed AES 256 ECB");
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Correct Output : %s",
              correctOutput.readHexString(correctOutput.length()).text());
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Result Output  : %s",
              encryptedData.readHexString(encryptedData.length()).text());
            result = false;
        }

        unaes256ECB.setup(key.startPointer(), key.length(), initVector.startPointer(), initVector.length());
        encryptedData.setReadOffset(0);
        data.setReadOffset(0);
        correctOutput.clear();
        correctOutput.writeStream(&data, data.length());
        data.clear();
        data.writeStream(&unaes256ECB, unaes256ECB.length());

        if(data == correctOutput)
            Log::add(Log::INFO, NEXTCASH_ENCRYPT_LOG_NAME, "Passed Decrypt AES 256 ECB");
        else
        {
            Log::add(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Failed Decrypt AES 256 ECB");
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Correct Output : %s",
              correctOutput.readHexString(correctOutput.length()).text());
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Result Output  : %s",
              data.readHexString(data.length()).text());
            result = false;
        }

        /******************************************************************************************
         * WikiPedia Test Vector
         ******************************************************************************************/
        data.clear();
        data.writeHex("4ec137a426dabf8aa0beb8bc0c2b89d6");

        key.clear();
        key.writeHex("95a8ee8e89979b9efdcbc6eb9797528d");
        initVector.clear();
        data.setReadOffset(0);
        encryptedData.clear();

        aes128ECB.setup(key.startPointer(), key.length(), initVector.startPointer(), initVector.length());
        aes128ECB.writeStream(&data, data.remaining());
        aes128ECB.finalize();

        correctOutput.clear();
        correctOutput.writeHex("d9b65d1232ba0199cdbd487b2a1fd646");

        if(encryptedData == correctOutput)
            Log::add(Log::INFO, NEXTCASH_ENCRYPT_LOG_NAME, "Passed Wiki AES 128 ECB");
        else
        {
            Log::add(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Failed Wiki AES 128 ECB");
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Correct Output : %s",
              correctOutput.readHexString(correctOutput.length()).text());
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Result Output  : %s",
              encryptedData.readHexString(encryptedData.length()).text());
            result = false;
        }

        unaes128ECB.setup(key.startPointer(), key.length(), initVector.startPointer(), initVector.length());
        encryptedData.setReadOffset(0);
        data.setReadOffset(0);
        correctOutput.clear();
        correctOutput.writeStream(&data, data.length());
        data.clear();
        data.writeStream(&unaes128ECB, unaes128ECB.length());

        if(data == correctOutput)
            Log::add(Log::INFO, NEXTCASH_ENCRYPT_LOG_NAME, "Passed Decrypt Wiki AES 128 ECB");
        else
        {
            Log::add(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Failed Decrypt Wiki AES 128 ECB");
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Correct Output : %s",
              correctOutput.readHexString(correctOutput.length()).text());
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Result Output  : %s",
              data.readHexString(data.length()).text());
            result = false;
        }

        key.clear();
        key.writeHex("95a8ee8e89979b9efdcbc6eb9797528d432dc26061553818");
        initVector.clear();
        data.setReadOffset(0);
        encryptedData.clear();

        aes192ECB.setup(key.startPointer(), key.length(), initVector.startPointer(), initVector.length());
        aes192ECB.writeStream(&data, data.remaining());
        aes192ECB.finalize();

        correctOutput.clear();
        correctOutput.writeHex("b18bb3e7e10732be1358443a504dbb49");

        if(encryptedData == correctOutput)
            Log::add(Log::INFO, NEXTCASH_ENCRYPT_LOG_NAME, "Passed Wiki AES 192 ECB");
        else
        {
            Log::add(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Failed Wiki AES 192 ECB");
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Correct Output : %s",
              correctOutput.readHexString(correctOutput.length()).text());
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Result Output  : %s",
              encryptedData.readHexString(encryptedData.length()).text());
            result = false;
        }

        unaes192ECB.setup(key.startPointer(), key.length(), initVector.startPointer(), initVector.length());
        encryptedData.setReadOffset(0);
        data.setReadOffset(0);
        correctOutput.clear();
        correctOutput.writeStream(&data, data.length());
        data.clear();
        data.writeStream(&unaes192ECB, unaes192ECB.length());

        if(data == correctOutput)
            Log::add(Log::INFO, NEXTCASH_ENCRYPT_LOG_NAME, "Passed Decrypt Wiki AES 192 ECB");
        else
        {
            Log::add(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Failed Decrypt Wiki AES 192 ECB");
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Correct Output : %s",
              correctOutput.readHexString(correctOutput.length()).text());
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Result Output  : %s",
              data.readHexString(data.length()).text());
            result = false;
        }

        key.clear();
        key.writeHex("95a8ee8e89979b9efdcbc6eb9797528d432dc26061553818ea635ec5d5a7727e");
        initVector.clear();
        data.setReadOffset(0);
        encryptedData.clear();

        aes256ECB.setup(key.startPointer(), key.length(), initVector.startPointer(), initVector.length());
        aes256ECB.writeStream(&data, data.remaining());
        aes256ECB.finalize();

        correctOutput.clear();
        correctOutput.writeHex("2f9cfddbffcde6b9f37ef8e40d512cf4");

        if(encryptedData == correctOutput)
            Log::add(Log::INFO, NEXTCASH_ENCRYPT_LOG_NAME, "Passed Wiki AES 256 ECB");
        else
        {
            Log::add(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Failed Wiki AES 256 ECB");
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Correct Output : %s",
              correctOutput.readHexString(correctOutput.length()).text());
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Result Output  : %s",
              encryptedData.readHexString(encryptedData.length()).text());
            result = false;
        }

        unaes256ECB.setup(key.startPointer(), key.length(), initVector.startPointer(), initVector.length());
        encryptedData.setReadOffset(0);
        data.setReadOffset(0);
        correctOutput.clear();
        correctOutput.writeStream(&data, data.length());
        data.clear();
        data.writeStream(&unaes256ECB, unaes256ECB.length());

        if(data == correctOutput)
            Log::add(Log::INFO, NEXTCASH_ENCRYPT_LOG_NAME, "Passed Decrypt Wiki AES 256 ECB");
        else
        {
            Log::add(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Failed Decrypt Wiki AES 256 ECB");
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Correct Output : %s",
              correctOutput.readHexString(correctOutput.length()).text());
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Result Output  : %s",
              data.readHexString(data.length()).text());
            result = false;
        }

        /******************************************************************************************
         * Inconteam.com Test Vector
         * www.inconteam.com/index.php?option=com_content&view=article&id=55:aes-test-vectors&catid=41:encryption&Itemid=60
         ******************************************************************************************/
        data.clear();
        data.writeHex("6bc1bee22e409f96e93d7e117393172aae2d8a571e03ac9c9eb76fac45af8e5130c81c46a35ce411e5fbc1191a0a52eff69f2445df4f9b17ad2b417be66c3710");

        key.clear();
        key.writeHex("2b7e151628aed2a6abf7158809cf4f3c");
        initVector.clear();
        data.setReadOffset(0);
        encryptedData.clear();

        aes128ECB.setup(key.startPointer(), key.length(), initVector.startPointer(), initVector.length());
        aes128ECB.writeStream(&data, data.remaining());
        aes128ECB.finalize();

        correctOutput.clear();
        correctOutput.writeHex("3ad77bb40d7a3660a89ecaf32466ef97f5d3d58503b9699de785895a96fdbaaf43b1cd7f598ece23881b00e3ed0306887b0c785e27e8ad3f8223207104725dd4");

        if(encryptedData == correctOutput)
            Log::add(Log::INFO, NEXTCASH_ENCRYPT_LOG_NAME, "Passed Inconteam AES 128 ECB");
        else
        {
            Log::add(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Failed Inconteam AES 128 ECB");
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Correct Output : %s",
              correctOutput.readHexString(correctOutput.length()).text());
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Result Output  : %s",
              encryptedData.readHexString(encryptedData.length()).text());
            result = false;
        }

        unaes128ECB.setup(key.startPointer(), key.length(), initVector.startPointer(), initVector.length());
        encryptedData.setReadOffset(0);
        data.setReadOffset(0);
        correctOutput.clear();
        correctOutput.writeStream(&data, data.length());
        data.clear();
        data.writeStream(&unaes128ECB, unaes128ECB.length());

        if(data == correctOutput)
            Log::add(Log::INFO, NEXTCASH_ENCRYPT_LOG_NAME, "Passed Decrypt Inconteam AES 128 ECB");
        else
        {
            Log::add(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Failed Decrypt Inconteam AES 128 ECB");
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Correct Output : %s",
              correctOutput.readHexString(correctOutput.length()).text());
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Result Output  : %s",
              data.readHexString(data.length()).text());
            result = false;
        }

        key.clear();
        key.writeHex("8e73b0f7da0e6452c810f32b809079e562f8ead2522c6b7b");
        initVector.clear();
        data.setReadOffset(0);
        encryptedData.clear();

        aes192ECB.setup(key.startPointer(), key.length(), initVector.startPointer(), initVector.length());
        aes192ECB.writeStream(&data, data.remaining());
        aes192ECB.finalize();

        correctOutput.clear();
        correctOutput.writeHex("bd334f1d6e45f25ff712a214571fa5cc974104846d0ad3ad7734ecb3ecee4eefef7afd2270e2e60adce0ba2face6444e9a4b41ba738d6c72fb16691603c18e0e");

        if(encryptedData == correctOutput)
            Log::add(Log::INFO, NEXTCASH_ENCRYPT_LOG_NAME, "Passed Inconteam AES 192 ECB");
        else
        {
            Log::add(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Failed Inconteam AES 192 ECB");
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Correct Output : %s",
              correctOutput.readHexString(correctOutput.length()).text());
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Result Output  : %s",
              encryptedData.readHexString(encryptedData.length()).text());
            result = false;
        }

        unaes192ECB.setup(key.startPointer(), key.length(), initVector.startPointer(), initVector.length());
        encryptedData.setReadOffset(0);
        data.setReadOffset(0);
        correctOutput.clear();
        correctOutput.writeStream(&data, data.length());
        data.clear();
        data.writeStream(&unaes192ECB, unaes192ECB.length());

        if(data == correctOutput)
            Log::add(Log::INFO, NEXTCASH_ENCRYPT_LOG_NAME, "Passed Decrypt Inconteam AES 192 ECB");
        else
        {
            Log::add(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Failed Decrypt Inconteam AES 192 ECB");
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Correct Output : %s",
              correctOutput.readHexString(correctOutput.length()).text());
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Result Output  : %s",
              data.readHexString(data.length()).text());
            result = false;
        }

        key.clear();
        key.writeHex("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4");
        initVector.clear();
        data.setReadOffset(0);
        encryptedData.clear();

        aes256ECB.setup(key.startPointer(), key.length(), initVector.startPointer(), initVector.length());
        aes256ECB.writeStream(&data, data.remaining());
        aes256ECB.finalize();

        correctOutput.clear();
        correctOutput.writeHex("f3eed1bdb5d2a03c064b5a7e3db181f8591ccb10d410ed26dc5ba74a31362870b6ed21b99ca6f4f9f153e7b1beafed1d23304b7a39f9f3ff067d8d8f9e24ecc7");

        if(encryptedData == correctOutput)
            Log::add(Log::INFO, NEXTCASH_ENCRYPT_LOG_NAME, "Passed Inconteam AES 256 ECB");
        else
        {
            Log::add(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Failed Inconteam AES 256 ECB");
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Correct Output : %s",
              correctOutput.readHexString(correctOutput.length()).text());
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Result Output  : %s",
              encryptedData.readHexString(encryptedData.length()).text());
            result = false;
        }

        unaes256ECB.setup(key.startPointer(), key.length(), initVector.startPointer(), initVector.length());
        encryptedData.setReadOffset(0);
        data.setReadOffset(0);
        correctOutput.clear();
        correctOutput.writeStream(&data, data.length());
        data.clear();
        data.writeStream(&unaes256ECB, unaes256ECB.length());

        if(data == correctOutput)
            Log::add(Log::INFO, NEXTCASH_ENCRYPT_LOG_NAME, "Passed Decrypt Inconteam AES 256 ECB");
        else
        {
            Log::add(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Failed Decrypt Inconteam AES 256 ECB");
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Correct Output : %s",
              correctOutput.readHexString(correctOutput.length()).text());
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Result Output  : %s",
              data.readHexString(data.length()).text());
            result = false;
        }

        /******************************************************************************************
         * Inconteam.com Test Vector CBC
         * www.inconteam.com/index.php?option=com_content&view=article&id=55:aes-test-vectors&catid=41:encryption&Itemid=60
         ******************************************************************************************/
        initVector.clear();
        initVector.writeHex("000102030405060708090a0b0c0d0e0f");

        key.clear();
        key.writeHex("2b7e151628aed2a6abf7158809cf4f3c");
        data.setReadOffset(0);
        encryptedData.clear();

        aes128CBC.setup(key.startPointer(), key.length(), initVector.startPointer(), initVector.length());
        aes128CBC.writeStream(&data, data.remaining());
        aes128CBC.finalize();

        correctOutput.clear();
        correctOutput.writeHex("7649abac8119b246cee98e9b12e9197d5086cb9b507219ee95db113a917678b273bed6b8e3c1743b7116e69e222295163ff1caa1681fac09120eca307586e1a7");

        if(encryptedData == correctOutput)
            Log::add(Log::INFO, NEXTCASH_ENCRYPT_LOG_NAME, "Passed Inconteam AES 128 CBC");
        else
        {
            Log::add(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Failed Inconteam AES 128 CBC");
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Correct Output : %s",
              correctOutput.readHexString(correctOutput.length()).text());
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Result Output  : %s",
              encryptedData.readHexString(encryptedData.length()).text());
            result = false;
        }

        unaes128CBC.setup(key.startPointer(), key.length(), initVector.startPointer(), initVector.length());
        encryptedData.setReadOffset(0);
        data.setReadOffset(0);
        correctOutput.clear();
        correctOutput.writeStream(&data, data.length());
        data.clear();
        data.writeStream(&unaes128CBC, unaes128CBC.length());

        if(data == correctOutput)
            Log::add(Log::INFO, NEXTCASH_ENCRYPT_LOG_NAME, "Passed Decrypt Inconteam AES 128 CBC");
        else
        {
            Log::add(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Failed Decrypt Inconteam AES 128 CBC");
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Correct Output : %s",
              correctOutput.readHexString(correctOutput.length()).text());
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Result Output  : %s",
              data.readHexString(data.length()).text());
            result = false;
        }

        key.clear();
        key.writeHex("8e73b0f7da0e6452c810f32b809079e562f8ead2522c6b7b");
        data.setReadOffset(0);
        encryptedData.clear();

        aes192CBC.setup(key.startPointer(), key.length(), initVector.startPointer(), initVector.length());
        aes192CBC.writeStream(&data, data.remaining());
        aes192CBC.finalize();

        correctOutput.clear();
        correctOutput.writeHex("4f021db243bc633d7178183a9fa071e8b4d9ada9ad7dedf4e5e738763f69145a571b242012fb7ae07fa9baac3df102e008b0e27988598881d920a9e64f5615cd");

        if(encryptedData == correctOutput)
            Log::add(Log::INFO, NEXTCASH_ENCRYPT_LOG_NAME, "Passed Inconteam AES 192 CBC");
        else
        {
            Log::add(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Failed Inconteam AES 192 CBC");
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Correct Output : %s",
              correctOutput.readHexString(correctOutput.length()).text());
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Result Output  : %s",
              encryptedData.readHexString(encryptedData.length()).text());
            result = false;
        }

        unaes192CBC.setup(key.startPointer(), key.length(), initVector.startPointer(), initVector.length());
        encryptedData.setReadOffset(0);
        data.setReadOffset(0);
        correctOutput.clear();
        correctOutput.writeStream(&data, data.length());
        data.clear();
        data.writeStream(&unaes192CBC, unaes192CBC.length());

        if(data == correctOutput)
            Log::add(Log::INFO, NEXTCASH_ENCRYPT_LOG_NAME, "Passed Decrypt Inconteam AES 192 CBC");
        else
        {
            Log::add(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Failed Decrypt Inconteam AES 192 CBC");
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Correct Output : %s",
              correctOutput.readHexString(correctOutput.length()).text());
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Result Output  : %s",
              data.readHexString(data.length()).text());
            result = false;
        }

        key.clear();
        key.writeHex("603deb1015ca71be2b73aef0857d77811f352c073b6108d72d9810a30914dff4");
        data.setReadOffset(0);
        encryptedData.clear();

        aes256CBC.setup(key.startPointer(), key.length(), initVector.startPointer(), initVector.length());
        aes256CBC.writeStream(&data, data.remaining());
        aes256CBC.finalize();

        correctOutput.clear();
        correctOutput.writeHex("f58c4c04d6e5f1ba779eabfb5f7bfbd69cfc4e967edb808d679f777bc6702c7d39f23369a9d9bacfa530e26304231461b2eb05e2c39be9fcda6c19078c6a9d1b");

        if(encryptedData == correctOutput)
            Log::add(Log::INFO, NEXTCASH_ENCRYPT_LOG_NAME, "Passed Inconteam AES 256 CBC");
        else
        {
            Log::add(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Failed Inconteam AES 256 CBC");
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Correct Output : %s",
              correctOutput.readHexString(correctOutput.length()).text());
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Result Output  : %s",
              encryptedData.readHexString(encryptedData.length()).text());
            result = false;
        }

        unaes256CBC.setup(key.startPointer(), key.length(), initVector.startPointer(), initVector.length());
        encryptedData.setReadOffset(0);
        data.setReadOffset(0);
        correctOutput.clear();
        correctOutput.writeStream(&data, data.length());
        data.clear();
        data.writeStream(&unaes256CBC, unaes256CBC.length());

        if(data == correctOutput)
            Log::add(Log::INFO, NEXTCASH_ENCRYPT_LOG_NAME, "Passed Decrypt Inconteam AES 256 CBC");
        else
        {
            Log::add(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Failed Decrypt Inconteam AES 256 CBC");
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Correct Output : %s",
              correctOutput.readHexString(correctOutput.length()).text());
            Log::addFormatted(Log::ERROR, NEXTCASH_ENCRYPT_LOG_NAME, "Result Output  : %s",
              data.readHexString(data.length()).text());
            result = false;
        }

        return result;
    }
}
