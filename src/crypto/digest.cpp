#include "digest.hpp"

#include "arcmist/base/endian.hpp"
#include "arcmist/base/math.hpp"
#include "arcmist/base/log.hpp"
#include "arcmist/io/stream.hpp"
#include "arcmist/io/buffer.hpp"

#include <cstdint>
#include <cstring>
#include <vector>

#define DIGEST_LOG_NAME "Digest"


namespace ArcMist
{
    namespace CRC32
    {
        const uint32_t table[256] = {0x00000000, 0x77073096, 0xee0e612c, 0x990951ba, 0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3, 0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988, 0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
                                     0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de, 0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7, 0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec, 0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
                                     0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172, 0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b, 0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940, 0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
                                     0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116, 0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f, 0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924, 0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
                                     0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a, 0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433, 0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818, 0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
                                     0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e, 0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457, 0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c, 0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
                                     0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2, 0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb, 0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0, 0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
                                     0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086, 0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f, 0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4, 0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
                                     0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a, 0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683, 0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8, 0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
                                     0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe, 0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7, 0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc, 0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
                                     0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252, 0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b, 0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60, 0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
                                     0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236, 0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f, 0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04, 0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
                                     0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a, 0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713, 0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38, 0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
                                     0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e, 0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777, 0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c, 0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
                                     0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2, 0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db, 0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0, 0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
                                     0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6, 0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf, 0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94, 0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d};
    }

    void Digest::crc32(InputStream *pInput, unsigned int pInputLength, OutputStream *pOutput)
    {
        uint32_t result = 0xffffffff;
        unsigned int remaining = pInputLength;

        while(remaining--)
            result = (result >> 8) ^ CRC32::table[(result & 0xFF) ^ pInput->readByte()];

        result ^= 0xffffffff;
        pOutput->writeUnsignedInt(Endian::convert(result, Endian::BIG));
    }

    uint32_t Digest::crc32(const char *pText)
    {
        uint32_t result = 0xffffffff;
        unsigned int offset = 0;

        while(pText[offset])
            result = (result >> 8) ^ CRC32::table[(result & 0xFF) ^ pText[offset++]];

        return result ^ 0xffffffff;
    }

    uint32_t Digest::crc32(const uint8_t *pData, unsigned int pSize)
    {
        uint32_t result = 0xffffffff;
        unsigned int offset = 0;

        while(offset < pSize)
            result = (result >> 8) ^ CRC32::table[(result & 0xFF) ^ pData[offset++]];

        return result ^ 0xffffffff;
    }

    namespace MD5
    {
        inline int f(int pX, int pY, int pZ)
        {
            // XY v not(X) Z
            return (pX & pY) | (~pX & pZ);
        }
        
        inline int g(int pX, int pY, int pZ)
        {
            // XZ v Y not(Z)
            return (pX & pZ) |(pY & ~pZ);
        }
        
        inline int h(int pX, int pY, int pZ)
        {
            // X xor Y xor Z
            return (pX ^ pY) ^ pZ;
        }
        
        inline int i(int pX, int pY, int pZ)
        {
            // Y xor(X v not(Z))
            return pY ^ (pX | ~pZ);
        }

        inline void round1(int &pA, int pB, int pC, int pD, int pXK, int pS, int pTI)
        {
            int step1 = (pA + f(pB, pC, pD) + pXK + pTI);
            pA = pB + Math::rotateLeft(step1, pS);
        }

        inline void round2(int &pA, int pB, int pC, int pD, int pXK, int pS, int pTI)
        {
            int step1 = (pA + g(pB, pC, pD) + pXK + pTI);
            pA = pB + Math::rotateLeft(step1, pS);
        }

        inline void round3(int &pA, int pB, int pC, int pD, int pXK, int pS, int pTI)
        {
            int step1 = (pA + h(pB, pC, pD) + pXK + pTI);
            pA = pB + Math::rotateLeft(step1, pS);
        }

        inline void round4(int &pA, int pB, int pC, int pD, int pXK, int pS, int pTI)
        {
            int step1 = (pA + i(pB, pC, pD) + pXK + pTI);
            pA = pB + Math::rotateLeft(step1, pS);
        }

        // Encodes raw bytes into the 16 integer block
        inline void encode(uint8_t *pData, int *pBlock)
        {
            int i, j;

            for(i=0,j=0;j<64;i++,j+=4)
                pBlock[i] = ((int)pData[j]) | (((int)pData[j+1]) << 8) |
                 (((int)pData[j+2]) << 16) | (((int)pData[j+3]) << 24);
        }

        // Decodes 16 integer block into raw bytes
        inline void decode(int *pBlock, uint8_t *pData)
        {
            int i, j;

            for(i=0,j=0;j<64;i++,j+=4)
            {
                pData[j]   = (uint8_t)(pBlock[i] & 0xff);
                pData[j+1] = (uint8_t)((pBlock[i] >> 8) & 0xff);
                pData[j+2] = (uint8_t)((pBlock[i] >> 16) & 0xff);
                pData[j+3] = (uint8_t)((pBlock[i] >> 24) & 0xff);
            }
        }
    }

    void Digest::md5(InputStream *pInput, unsigned int pInputLength, OutputStream *pOutput) // 128 bit(16 byte) result
    {
        uint8_t *data;
        unsigned int dataSize = pInputLength + 9;

        // Make the data size is congruent to 448, modulo 512 bits(56, 64 modulo bytes)
        dataSize += (64 - (dataSize % 64));
        data = new uint8_t[dataSize];

        unsigned int offset = pInputLength;
        pInput->read(data, pInputLength);

        data[offset++] = 128; // 10000000 - append a one bit and the rest zero bits

        // Append the rest zero bits
        if(offset <(dataSize - 1))
            std::memset(data + offset, 0, dataSize - offset);

        // Append the data length as a 64 bit number low order word first
        int bitLength = pInputLength * 8;
        data[dataSize - 8] = (uint8_t)(bitLength & 0xff);
        data[dataSize - 7] = (uint8_t)((bitLength >> 8) & 0xff);
        data[dataSize - 6] = (uint8_t)((bitLength >> 16) & 0xff);
        data[dataSize - 5] = (uint8_t)((bitLength >> 24) & 0xff);
        std::memset((data + dataSize) - 4, 0, 4);

        // Setup buffers
        int state[4], a, b, c, d;

        state[0] = 0x67452301; // A
        state[1] = 0xEFCDAB89; // B
        state[2] = 0x98BADCFE; // C
        state[3] = 0x10325476; // D

        // Process 16 word(64 byte) blocks of data
        int blockIndex, blockCount = dataSize / 64;
        int blockX[16];

        for(blockIndex=0;blockIndex<blockCount;blockIndex++)
        {
            // Copy current block into blockX
            MD5::encode(data + (blockIndex * 64), blockX);

            a = state[0];
            b = state[1];
            c = state[2];
            d = state[3];

            // Round 1 - a = b +((a + F(b,c,d) + X[k] + T[i]) <<< s)
            MD5::round1(a, b, c, d, blockX[ 0],  7, 0xD76AA478);
            MD5::round1(d, a, b, c, blockX[ 1], 12, 0xE8C7B756);
            MD5::round1(c, d, a, b, blockX[ 2], 17, 0x242070DB);
            MD5::round1(b, c, d, a, blockX[ 3], 22, 0xC1BDCEEE);
            MD5::round1(a, b, c, d, blockX[ 4],  7, 0xF57C0FAF);
            MD5::round1(d, a, b, c, blockX[ 5], 12, 0x4787C62A);
            MD5::round1(c, d, a, b, blockX[ 6], 17, 0xA8304613);
            MD5::round1(b, c, d, a, blockX[ 7], 22, 0xFD469501);
            MD5::round1(a, b, c, d, blockX[ 8],  7, 0x698098D8);
            MD5::round1(d, a, b, c, blockX[ 9], 12, 0x8B44F7AF);
            MD5::round1(c, d, a, b, blockX[10], 17, 0xFFFF5BB1);
            MD5::round1(b, c, d, a, blockX[11], 22, 0x895CD7BE);
            MD5::round1(a, b, c, d, blockX[12],  7, 0x6B901122);
            MD5::round1(d, a, b, c, blockX[13], 12, 0xFD987193);
            MD5::round1(c, d, a, b, blockX[14], 17, 0xA679438E);
            MD5::round1(b, c, d, a, blockX[15], 22, 0x49B40821);

            // Round 2 - a = b +((a + G(b,,d) + X[k] + T[i]) <<< s)
            MD5::round2(a, b, c, d, blockX[ 1],  5, 0xF61E2562);
            MD5::round2(d, a, b, c, blockX[ 6],  9, 0xC040B340);
            MD5::round2(c, d, a, b, blockX[11], 14, 0x265E5A51);
            MD5::round2(b, c, d, a, blockX[ 0], 20, 0xE9B6C7AA);
            MD5::round2(a, b, c, d, blockX[ 5],  5, 0xD62F105D);
            MD5::round2(d, a, b, c, blockX[10],  9, 0x02441453);
            MD5::round2(c, d, a, b, blockX[15], 14, 0xD8A1E681);
            MD5::round2(b, c, d, a, blockX[ 4], 20, 0xE7D3FBC8);
            MD5::round2(a, b, c, d, blockX[ 9],  5, 0x21E1CDE6);
            MD5::round2(d, a, b, c, blockX[14],  9, 0xC33707D6);
            MD5::round2(c, d, a, b, blockX[ 3], 14, 0xF4D50D87);
            MD5::round2(b, c, d, a, blockX[ 8], 20, 0x455A14ED);
            MD5::round2(a, b, c, d, blockX[13],  5, 0xA9E3E905);
            MD5::round2(d, a, b, c, blockX[ 2],  9, 0xFCEFA3F8);
            MD5::round2(c, d, a, b, blockX[ 7], 14, 0x676F02D9);
            MD5::round2(b, c, d, a, blockX[12], 20, 0x8D2A4C8A);

            // Round 3 - a = b +((a + H(b,,d) + X[k] + T[i]) <<< s)
            MD5::round3(a, b, c, d, blockX[ 5],  4, 0xFFFA3942);
            MD5::round3(d, a, b, c, blockX[ 8], 11, 0x8771F681);
            MD5::round3(c, d, a, b, blockX[11], 16, 0x6D9D6122);
            MD5::round3(b, c, d, a, blockX[14], 23, 0xFDE5380C);
            MD5::round3(a, b, c, d, blockX[ 1],  4, 0xA4BEEA44);
            MD5::round3(d, a, b, c, blockX[ 4], 11, 0x4BDECFA9);
            MD5::round3(c, d, a, b, blockX[ 7], 16, 0xF6BB4B60);
            MD5::round3(b, c, d, a, blockX[10], 23, 0xBEBFBC70);
            MD5::round3(a, b, c, d, blockX[13],  4, 0x289B7EC6);
            MD5::round3(d, a, b, c, blockX[ 0], 11, 0xEAA127FA);
            MD5::round3(c, d, a, b, blockX[ 3], 16, 0xD4EF3085);
            MD5::round3(b, c, d, a, blockX[ 6], 23, 0x04881D05);
            MD5::round3(a, b, c, d, blockX[ 9],  4, 0xD9D4D039);
            MD5::round3(d, a, b, c, blockX[12], 11, 0xE6DB99E5);
            MD5::round3(c, d, a, b, blockX[15], 16, 0x1FA27CF8);
            MD5::round3(b, c, d, a, blockX[ 2], 23, 0xC4AC5665);

            // Round 4 - a = b +((a + I(b,,d) + X[k] + T[i]) <<< s)
            MD5::round4(a, b, c, d, blockX[ 0],  6, 0xF4292244);
            MD5::round4(d, a, b, c, blockX[ 7], 10, 0x432AFF97);
            MD5::round4(c, d, a, b, blockX[14], 15, 0xAB9423A7);
            MD5::round4(b, c, d, a, blockX[ 5], 21, 0xFC93A039);
            MD5::round4(a, b, c, d, blockX[12],  6, 0x655B59C3);
            MD5::round4(d, a, b, c, blockX[ 3], 10, 0x8F0CCC92);
            MD5::round4(c, d, a, b, blockX[10], 15, 0xFFEFF47D);
            MD5::round4(b, c, d, a, blockX[ 1], 21, 0x85845DD1);
            MD5::round4(a, b, c, d, blockX[ 8],  6, 0x6FA87E4F);
            MD5::round4(d, a, b, c, blockX[15], 10, 0xFE2CE6E0);
            MD5::round4(c, d, a, b, blockX[ 6], 15, 0xA3014314);
            MD5::round4(b, c, d, a, blockX[13], 21, 0x4E0811A1);
            MD5::round4(a, b, c, d, blockX[ 4],  6, 0xF7537E82);
            MD5::round4(d, a, b, c, blockX[11], 10, 0xBD3AF235);
            MD5::round4(c, d, a, b, blockX[ 2], 15, 0x2AD7D2BB);
            MD5::round4(b, c, d, a, blockX[ 9], 21, 0xEB86D391);

            // Increment each state by it's previous value
            state[0] += a;
            state[1] += b;
            state[2] += c;
            state[3] += d;
        }

        pOutput->write((const uint8_t *)state, 16);

        // Zeroize sensitive information
        std::memset(blockX, 0, sizeof(blockX));
        std::memset(data, 0, dataSize);
        delete[] data;
    }

    void Digest::sha1(InputStream *pInput, unsigned int pInputLength, OutputStream *pOutput) // 160 bit(20 byte) result
    {
        // Note 1: All variables are unsigned 32 bits and wrap modulo 232 when calculating
        // Note 2: All constants in this pseudo code are in big endian. 
        // Within each word, the most significant bit is stored in the leftmost bit position

        unsigned int dataSize = pInputLength + (64 - (pInputLength % 64));
        uint64_t bitLength = (uint64_t)pInputLength * 8;

        Buffer message;
        message.setEndian(Endian::BIG);
        message.writeStream(pInput, pInputLength);

        // Pre-processing:
        // append the bit '1' to the message
        message.writeByte(0x80);

        // append k bits '0', where k is the minimum number ? 0 such that the resulting message
        //     length(in bits) is congruent to 448(mod 512)
        // Append the rest zero bits
        while(message.length() < dataSize - 8)
            message.writeByte(0);

        // append length of message(before pre-processing), in bits, as 64-bit big-endian integer
        // Append the data length as a 64 bit number low order word first
        message.writeUnsignedLong(bitLength);

        uint32_t state[5] = { 0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0 };
        uint32_t a, b, c, d, e;

        // Process the message in successive 512-bit chunks:
        // break message into 512-bit chunks
        // for each chunk
        unsigned int i, blockIndex, blockCount = dataSize / 64;
        uint32_t extended[80];
        uint32_t f, k, step;

        for(blockIndex=0;blockIndex<blockCount;blockIndex++)
        {
            // break chunk into sixteen 32-bit big-endian words w[i], 0 ? i ? 15
            for(i=0;i<16;i++)
                extended[i] = message.readUnsignedInt();

            // Extend the sixteen 32-bit words into eighty 32-bit words:
            for(i=16;i<80;i++)
                extended[i] = Math::rotateLeft(extended[i-3] ^ extended[i-8] ^ extended[i-14] ^ extended[i-16], 1);

            // Initialize hash value for this chunk:
            a = state[0];
            b = state[1];
            c = state[2];
            d = state[3];
            e = state[4];

            // Main loop:
            for(i=0;i<80;i++)
            {
                if(i < 20)
                {
                    f = (b & c) |((~b) & d);
                    k = 0x5A827999;
                }
                else if(i < 40)
                {
                    f = b ^ c ^ d;
                    k = 0x6ED9EBA1;
                }
                else if(i < 60)
                {
                    f = (b & c) |(b & d) |(c & d);
                    k = 0x8F1BBCDC;
                }
                else // if(i < 80)
                {
                    f = b ^ c ^ d;
                    k = 0xCA62C1D6;
                }

                step = Math::rotateLeft(a, 5) + f + e + k + extended[i];
                e = d;
                d = c;
                c = Math::rotateLeft(b, 30);
                b = a;
                a = step;
            }

            // Add this chunk's hash to result so far:
            state[0] += a;
            state[1] += b;
            state[2] += c;
            state[3] += d;
            state[4] += e;
        }

        for(unsigned int i=0;i<5;i++)
            pOutput->writeUnsignedInt(Endian::convert(state[i], Endian::BIG));

        // Zeroize possible sensitive data
        message.zeroize();
        std::memset(extended, 0, sizeof(int) * 80);
        std::memset(state, 0, sizeof(int) * 5);
    }

    namespace RIPEMD160
    {
        inline uint32_t f(uint32_t pX, uint32_t pY, uint32_t pZ)
        {
            return pX ^ pY ^ pZ;
        }

        inline uint32_t g(uint32_t pX, uint32_t pY, uint32_t pZ)
        {
            return (pX & pY) | (~pX & pZ);
        }

        inline uint32_t h(uint32_t pX, uint32_t pY, uint32_t pZ)
        {
            return (pX | ~pY) ^ pZ;
        }

        inline uint32_t i(uint32_t pX, uint32_t pY, uint32_t pZ)
        {
            return (pX & pZ) | (pY & ~pZ);
        }

        inline uint32_t j(uint32_t pX, uint32_t pY, uint32_t pZ)
        {
            return pX ^ (pY | ~pZ);
        }

        inline void ff(uint32_t &pA, uint32_t &pB, uint32_t &pC, uint32_t &pD, uint32_t &pE, uint32_t &pX, uint32_t pS)
        {
            pA += f(pB, pC, pD) + pX;
            pA = Math::rotateLeft(pA, pS) + pE;
            pC = Math::rotateLeft(pC, 10);
        }

        inline void gg(uint32_t &pA, uint32_t &pB, uint32_t &pC, uint32_t &pD, uint32_t &pE, uint32_t &pX, uint32_t pS)
        {
            pA += g(pB, pC, pD) + pX + 0x5a827999;
            pA = Math::rotateLeft(pA, pS) + pE;
            pC = Math::rotateLeft(pC, 10);
        }

        inline void hh(uint32_t &pA, uint32_t &pB, uint32_t &pC, uint32_t &pD, uint32_t &pE, uint32_t &pX, uint32_t pS)
        {
            pA += h(pB, pC, pD) + pX + 0x6ed9eba1;
            pA = Math::rotateLeft(pA, pS) + pE;
            pC = Math::rotateLeft(pC, 10);
        }

        inline void ii(uint32_t &pA, uint32_t &pB, uint32_t &pC, uint32_t &pD, uint32_t &pE, uint32_t &pX, uint32_t pS)
        {
            pA += i(pB, pC, pD) + pX + 0x8f1bbcdc;
            pA = Math::rotateLeft(pA, pS) + pE;
            pC = Math::rotateLeft(pC, 10);
        }

        inline void jj(uint32_t &pA, uint32_t &pB, uint32_t &pC, uint32_t &pD, uint32_t &pE, uint32_t &pX, uint32_t pS)
        {
            pA += j(pB, pC, pD) + pX + 0xa953fd4e;
            pA = Math::rotateLeft(pA, pS) + pE;
            pC = Math::rotateLeft(pC, 10);
        }

        inline void fff(uint32_t &pA, uint32_t &pB, uint32_t &pC, uint32_t &pD, uint32_t &pE, uint32_t &pX, uint32_t pS)
        {
            pA += f(pB, pC, pD) + pX;
            pA = Math::rotateLeft(pA, pS) + pE;
            pC = Math::rotateLeft(pC, 10);
        }

        inline void ggg(uint32_t &pA, uint32_t &pB, uint32_t &pC, uint32_t &pD, uint32_t &pE, uint32_t &pX, uint32_t pS)
        {
            pA += g(pB, pC, pD) + pX + 0x7a6d76e9;
            pA = Math::rotateLeft(pA, pS) + pE;
            pC = Math::rotateLeft(pC, 10);
        }

        inline void hhh(uint32_t &pA, uint32_t &pB, uint32_t &pC, uint32_t &pD, uint32_t &pE, uint32_t &pX, uint32_t pS)
        {
            pA += h(pB, pC, pD) + pX + 0x6d703ef3;
            pA = Math::rotateLeft(pA, pS) + pE;
            pC = Math::rotateLeft(pC, 10);
        }

        inline void iii(uint32_t &pA, uint32_t &pB, uint32_t &pC, uint32_t &pD, uint32_t &pE, uint32_t &pX, uint32_t pS)
        {
            pA += i(pB, pC, pD) + pX + 0x5c4dd124;
            pA = Math::rotateLeft(pA, pS) + pE;
            pC = Math::rotateLeft(pC, 10);
        }

        inline void jjj(uint32_t &pA, uint32_t &pB, uint32_t &pC, uint32_t &pD, uint32_t &pE, uint32_t &pX, uint32_t pS)
        {
            pA += j(pB, pC, pD) + pX + 0x50a28be6;
            pA = Math::rotateLeft(pA, pS) + pE;
            pC = Math::rotateLeft(pC, 10);
        }

        void init(uint32_t *pResult)
        {
            pResult[0] = 0x67452301;
            pResult[1] = 0xefcdab89;
            pResult[2] = 0x98badcfe;
            pResult[3] = 0x10325476;
            pResult[4] = 0xc3d2e1f0;
        }

        void process(uint32_t *pResult, uint32_t *pBlock)
        {
            uint32_t aa = pResult[0],  bb = pResult[1],  cc = pResult[2], dd = pResult[3],  ee = pResult[4];
            uint32_t aaa = pResult[0], bbb = pResult[1], ccc = pResult[2], ddd = pResult[3], eee = pResult[4];

            // Round 1
            ff(aa, bb, cc, dd, ee, pBlock[ 0], 11);
            ff(ee, aa, bb, cc, dd, pBlock[ 1], 14);
            ff(dd, ee, aa, bb, cc, pBlock[ 2], 15);
            ff(cc, dd, ee, aa, bb, pBlock[ 3], 12);
            ff(bb, cc, dd, ee, aa, pBlock[ 4],  5);
            ff(aa, bb, cc, dd, ee, pBlock[ 5],  8);
            ff(ee, aa, bb, cc, dd, pBlock[ 6],  7);
            ff(dd, ee, aa, bb, cc, pBlock[ 7],  9);
            ff(cc, dd, ee, aa, bb, pBlock[ 8], 11);
            ff(bb, cc, dd, ee, aa, pBlock[ 9], 13);
            ff(aa, bb, cc, dd, ee, pBlock[10], 14);
            ff(ee, aa, bb, cc, dd, pBlock[11], 15);
            ff(dd, ee, aa, bb, cc, pBlock[12],  6);
            ff(cc, dd, ee, aa, bb, pBlock[13],  7);
            ff(bb, cc, dd, ee, aa, pBlock[14],  9);
            ff(aa, bb, cc, dd, ee, pBlock[15],  8);
                                     
            // Round 2
            gg(ee, aa, bb, cc, dd, pBlock[ 7],  7);
            gg(dd, ee, aa, bb, cc, pBlock[ 4],  6);
            gg(cc, dd, ee, aa, bb, pBlock[13],  8);
            gg(bb, cc, dd, ee, aa, pBlock[ 1], 13);
            gg(aa, bb, cc, dd, ee, pBlock[10], 11);
            gg(ee, aa, bb, cc, dd, pBlock[ 6],  9);
            gg(dd, ee, aa, bb, cc, pBlock[15],  7);
            gg(cc, dd, ee, aa, bb, pBlock[ 3], 15);
            gg(bb, cc, dd, ee, aa, pBlock[12],  7);
            gg(aa, bb, cc, dd, ee, pBlock[ 0], 12);
            gg(ee, aa, bb, cc, dd, pBlock[ 9], 15);
            gg(dd, ee, aa, bb, cc, pBlock[ 5],  9);
            gg(cc, dd, ee, aa, bb, pBlock[ 2], 11);
            gg(bb, cc, dd, ee, aa, pBlock[14],  7);
            gg(aa, bb, cc, dd, ee, pBlock[11], 13);
            gg(ee, aa, bb, cc, dd, pBlock[ 8], 12);

            // Round 3
            hh(dd, ee, aa, bb, cc, pBlock[ 3], 11);
            hh(cc, dd, ee, aa, bb, pBlock[10], 13);
            hh(bb, cc, dd, ee, aa, pBlock[14],  6);
            hh(aa, bb, cc, dd, ee, pBlock[ 4],  7);
            hh(ee, aa, bb, cc, dd, pBlock[ 9], 14);
            hh(dd, ee, aa, bb, cc, pBlock[15],  9);
            hh(cc, dd, ee, aa, bb, pBlock[ 8], 13);
            hh(bb, cc, dd, ee, aa, pBlock[ 1], 15);
            hh(aa, bb, cc, dd, ee, pBlock[ 2], 14);
            hh(ee, aa, bb, cc, dd, pBlock[ 7],  8);
            hh(dd, ee, aa, bb, cc, pBlock[ 0], 13);
            hh(cc, dd, ee, aa, bb, pBlock[ 6],  6);
            hh(bb, cc, dd, ee, aa, pBlock[13],  5);
            hh(aa, bb, cc, dd, ee, pBlock[11], 12);
            hh(ee, aa, bb, cc, dd, pBlock[ 5],  7);
            hh(dd, ee, aa, bb, cc, pBlock[12],  5);

            // Round 4
            ii(cc, dd, ee, aa, bb, pBlock[ 1], 11);
            ii(bb, cc, dd, ee, aa, pBlock[ 9], 12);
            ii(aa, bb, cc, dd, ee, pBlock[11], 14);
            ii(ee, aa, bb, cc, dd, pBlock[10], 15);
            ii(dd, ee, aa, bb, cc, pBlock[ 0], 14);
            ii(cc, dd, ee, aa, bb, pBlock[ 8], 15);
            ii(bb, cc, dd, ee, aa, pBlock[12],  9);
            ii(aa, bb, cc, dd, ee, pBlock[ 4],  8);
            ii(ee, aa, bb, cc, dd, pBlock[13],  9);
            ii(dd, ee, aa, bb, cc, pBlock[ 3], 14);
            ii(cc, dd, ee, aa, bb, pBlock[ 7],  5);
            ii(bb, cc, dd, ee, aa, pBlock[15],  6);
            ii(aa, bb, cc, dd, ee, pBlock[14],  8);
            ii(ee, aa, bb, cc, dd, pBlock[ 5],  6);
            ii(dd, ee, aa, bb, cc, pBlock[ 6],  5);
            ii(cc, dd, ee, aa, bb, pBlock[ 2], 12);

            // Round 5
            jj(bb, cc, dd, ee, aa, pBlock[ 4],  9);
            jj(aa, bb, cc, dd, ee, pBlock[ 0], 15);
            jj(ee, aa, bb, cc, dd, pBlock[ 5],  5);
            jj(dd, ee, aa, bb, cc, pBlock[ 9], 11);
            jj(cc, dd, ee, aa, bb, pBlock[ 7],  6);
            jj(bb, cc, dd, ee, aa, pBlock[12],  8);
            jj(aa, bb, cc, dd, ee, pBlock[ 2], 13);
            jj(ee, aa, bb, cc, dd, pBlock[10], 12);
            jj(dd, ee, aa, bb, cc, pBlock[14],  5);
            jj(cc, dd, ee, aa, bb, pBlock[ 1], 12);
            jj(bb, cc, dd, ee, aa, pBlock[ 3], 13);
            jj(aa, bb, cc, dd, ee, pBlock[ 8], 14);
            jj(ee, aa, bb, cc, dd, pBlock[11], 11);
            jj(dd, ee, aa, bb, cc, pBlock[ 6],  8);
            jj(cc, dd, ee, aa, bb, pBlock[15],  5);
            jj(bb, cc, dd, ee, aa, pBlock[13],  6);

            // Parallel round 1
            jjj(aaa, bbb, ccc, ddd, eee, pBlock[ 5],  8);
            jjj(eee, aaa, bbb, ccc, ddd, pBlock[14],  9);
            jjj(ddd, eee, aaa, bbb, ccc, pBlock[ 7],  9);
            jjj(ccc, ddd, eee, aaa, bbb, pBlock[ 0], 11);
            jjj(bbb, ccc, ddd, eee, aaa, pBlock[ 9], 13);
            jjj(aaa, bbb, ccc, ddd, eee, pBlock[ 2], 15);
            jjj(eee, aaa, bbb, ccc, ddd, pBlock[11], 15);
            jjj(ddd, eee, aaa, bbb, ccc, pBlock[ 4],  5);
            jjj(ccc, ddd, eee, aaa, bbb, pBlock[13],  7);
            jjj(bbb, ccc, ddd, eee, aaa, pBlock[ 6],  7);
            jjj(aaa, bbb, ccc, ddd, eee, pBlock[15],  8);
            jjj(eee, aaa, bbb, ccc, ddd, pBlock[ 8], 11);
            jjj(ddd, eee, aaa, bbb, ccc, pBlock[ 1], 14);
            jjj(ccc, ddd, eee, aaa, bbb, pBlock[10], 14);
            jjj(bbb, ccc, ddd, eee, aaa, pBlock[ 3], 12);
            jjj(aaa, bbb, ccc, ddd, eee, pBlock[12],  6);

            // Parallel round 2
            iii(eee, aaa, bbb, ccc, ddd, pBlock[ 6],  9); 
            iii(ddd, eee, aaa, bbb, ccc, pBlock[11], 13);
            iii(ccc, ddd, eee, aaa, bbb, pBlock[ 3], 15);
            iii(bbb, ccc, ddd, eee, aaa, pBlock[ 7],  7);
            iii(aaa, bbb, ccc, ddd, eee, pBlock[ 0], 12);
            iii(eee, aaa, bbb, ccc, ddd, pBlock[13],  8);
            iii(ddd, eee, aaa, bbb, ccc, pBlock[ 5],  9);
            iii(ccc, ddd, eee, aaa, bbb, pBlock[10], 11);
            iii(bbb, ccc, ddd, eee, aaa, pBlock[14],  7);
            iii(aaa, bbb, ccc, ddd, eee, pBlock[15],  7);
            iii(eee, aaa, bbb, ccc, ddd, pBlock[ 8], 12);
            iii(ddd, eee, aaa, bbb, ccc, pBlock[12],  7);
            iii(ccc, ddd, eee, aaa, bbb, pBlock[ 4],  6);
            iii(bbb, ccc, ddd, eee, aaa, pBlock[ 9], 15);
            iii(aaa, bbb, ccc, ddd, eee, pBlock[ 1], 13);
            iii(eee, aaa, bbb, ccc, ddd, pBlock[ 2], 11);

            // Parallel round 3
            hhh(ddd, eee, aaa, bbb, ccc, pBlock[15],  9);
            hhh(ccc, ddd, eee, aaa, bbb, pBlock[ 5],  7);
            hhh(bbb, ccc, ddd, eee, aaa, pBlock[ 1], 15);
            hhh(aaa, bbb, ccc, ddd, eee, pBlock[ 3], 11);
            hhh(eee, aaa, bbb, ccc, ddd, pBlock[ 7],  8);
            hhh(ddd, eee, aaa, bbb, ccc, pBlock[14],  6);
            hhh(ccc, ddd, eee, aaa, bbb, pBlock[ 6],  6);
            hhh(bbb, ccc, ddd, eee, aaa, pBlock[ 9], 14);
            hhh(aaa, bbb, ccc, ddd, eee, pBlock[11], 12);
            hhh(eee, aaa, bbb, ccc, ddd, pBlock[ 8], 13);
            hhh(ddd, eee, aaa, bbb, ccc, pBlock[12],  5);
            hhh(ccc, ddd, eee, aaa, bbb, pBlock[ 2], 14);
            hhh(bbb, ccc, ddd, eee, aaa, pBlock[10], 13);
            hhh(aaa, bbb, ccc, ddd, eee, pBlock[ 0], 13);
            hhh(eee, aaa, bbb, ccc, ddd, pBlock[ 4],  7);
            hhh(ddd, eee, aaa, bbb, ccc, pBlock[13],  5);

            // Parallel round 4
            ggg(ccc, ddd, eee, aaa, bbb, pBlock[ 8], 15);
            ggg(bbb, ccc, ddd, eee, aaa, pBlock[ 6],  5);
            ggg(aaa, bbb, ccc, ddd, eee, pBlock[ 4],  8);
            ggg(eee, aaa, bbb, ccc, ddd, pBlock[ 1], 11);
            ggg(ddd, eee, aaa, bbb, ccc, pBlock[ 3], 14);
            ggg(ccc, ddd, eee, aaa, bbb, pBlock[11], 14);
            ggg(bbb, ccc, ddd, eee, aaa, pBlock[15],  6);
            ggg(aaa, bbb, ccc, ddd, eee, pBlock[ 0], 14);
            ggg(eee, aaa, bbb, ccc, ddd, pBlock[ 5],  6);
            ggg(ddd, eee, aaa, bbb, ccc, pBlock[12],  9);
            ggg(ccc, ddd, eee, aaa, bbb, pBlock[ 2], 12);
            ggg(bbb, ccc, ddd, eee, aaa, pBlock[13],  9);
            ggg(aaa, bbb, ccc, ddd, eee, pBlock[ 9], 12);
            ggg(eee, aaa, bbb, ccc, ddd, pBlock[ 7],  5);
            ggg(ddd, eee, aaa, bbb, ccc, pBlock[10], 15);
            ggg(ccc, ddd, eee, aaa, bbb, pBlock[14],  8);

            // Parallel round 5
            fff(bbb, ccc, ddd, eee, aaa, pBlock[12] ,  8);
            fff(aaa, bbb, ccc, ddd, eee, pBlock[15] ,  5);
            fff(eee, aaa, bbb, ccc, ddd, pBlock[10] , 12);
            fff(ddd, eee, aaa, bbb, ccc, pBlock[ 4] ,  9);
            fff(ccc, ddd, eee, aaa, bbb, pBlock[ 1] , 12);
            fff(bbb, ccc, ddd, eee, aaa, pBlock[ 5] ,  5);
            fff(aaa, bbb, ccc, ddd, eee, pBlock[ 8] , 14);
            fff(eee, aaa, bbb, ccc, ddd, pBlock[ 7] ,  6);
            fff(ddd, eee, aaa, bbb, ccc, pBlock[ 6] ,  8);
            fff(ccc, ddd, eee, aaa, bbb, pBlock[ 2] , 13);
            fff(bbb, ccc, ddd, eee, aaa, pBlock[13] ,  6);
            fff(aaa, bbb, ccc, ddd, eee, pBlock[14] ,  5);
            fff(eee, aaa, bbb, ccc, ddd, pBlock[ 0] , 15);
            fff(ddd, eee, aaa, bbb, ccc, pBlock[ 3] , 13);
            fff(ccc, ddd, eee, aaa, bbb, pBlock[ 9] , 11);
            fff(bbb, ccc, ddd, eee, aaa, pBlock[11] , 11);

            // Combine results
            ddd += cc + pResult[1];

            pResult[1] = pResult[2] + dd + eee;
            pResult[2] = pResult[3] + ee + aaa;
            pResult[3] = pResult[4] + aa + bbb;
            pResult[4] = pResult[0] + bb + ccc;
            pResult[0] = ddd;
        }

        // BlockLength must be less than 64
        void finish(uint32_t *pResult, uint32_t *pBlock, unsigned int pBlockLength, uint64_t pTotalLength)
        {
            // Zeroize the end of the block
            std::memset(((uint8_t *)pBlock) + pBlockLength, 0, 64 - pBlockLength);

            // Add 0x80 byte (basically a 1 bit at the end of the data)
            ((uint8_t *)pBlock)[pBlockLength] = 0x80;

            // Check if there is enough room for the total length in this block
            if(pBlockLength > 55) // 55 because of the 0x80 byte
            {
                process(pResult, pBlock); // Process this block
                std::memset(pBlock, 0, 64); // Clear the block
            }

            // Put bit length in last 8 bytes of block
            pTotalLength *= 8;
            pBlock[14] = pTotalLength & 0xffffffff;
            pBlock[15] = (pTotalLength >> 32) & 0xffffffff;

            // Process last block
            process(pResult, pBlock);
        }
    }

    void Digest::ripEMD160(InputStream *pInput, unsigned int pInputLength, OutputStream *pOutput)
    {
        uint32_t result[5];
        uint32_t block[16];
        unsigned int remaining = pInputLength;

        RIPEMD160::init(result);

        // Process all full (64 byte) blocks
        while(remaining >= 64)
        {
            pInput->read(block, 64);
            remaining -= 64;
            RIPEMD160::process(result, block);
        }

        // Get last partial block (must be less than 64 bytes)
        pInput->read(block, remaining);

        // Process last block
        RIPEMD160::finish(result, block, remaining, pInputLength);

        // Write output
        pOutput->write(result, 20);
    }

    namespace SHA256
    {
        const uint32_t table[64] = {0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
                                    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
                                    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
                                    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
                                    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
                                    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
                                    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
                                    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

        void initialize(uint32_t *pResult)
        {
            pResult[0] = 0x6a09e667;
            pResult[1] = 0xbb67ae85;
            pResult[2] = 0x3c6ef372;
            pResult[3] = 0xa54ff53a;
            pResult[4] = 0x510e527f;
            pResult[5] = 0x9b05688c;
            pResult[6] = 0x1f83d9ab;
            pResult[7] = 0x5be0cd19;
        }

        void process(uint32_t *pResult, uint32_t *pBlock)
        {
            unsigned int i;
            uint32_t s0, s1, t1, t2;
            uint32_t extendedBlock[64];

            std::memcpy(extendedBlock, pBlock, 64);

            // Adjust for big endian
            if(Endian::sSystemType != Endian::BIG)
                for(i=0;i<16;i++)
                    extendedBlock[i] = Endian::convert(extendedBlock[i], Endian::BIG);

            // Extend the block to 64 32-bit words:
            for(i=16;i<64;i++)
            {
                s0 = extendedBlock[i-15];
                s0 = Math::rotateRight(s0, 7) ^ Math::rotateRight(s0, 18) ^ (s0 >> 3);

                s1 = extendedBlock[i-2];
                s1 = Math::rotateRight(s1, 17) ^ Math::rotateRight(s1, 19) ^ (s1 >> 10);

                extendedBlock[i] = extendedBlock[i-16] + extendedBlock[i-7] + s0 + s1;
            }

            uint32_t state[8];
            std::memcpy(state, pResult, 32);

            for(i=0;i<64;i++)
            {
                s0 = Math::rotateRight(state[0], 2) ^ Math::rotateRight(state[0], 13) ^ Math::rotateRight(state[0], 22);
                t2 = s0 +((state[0] & state[1]) ^(state [0] & state[2]) ^(state[1] & state[2]));
                s1 = Math::rotateRight(state[4], 6) ^ Math::rotateRight(state[4], 11) ^ Math::rotateRight(state[4], 25);
                t1 = state[7] + s1 +((state[4] & state[5]) ^(~state[4] & state[6])) + table[i] + extendedBlock[i];

                state[7] = state[6];
                state[6] = state[5];
                state[5] = state[4];
                state[4] = state[3] + t1;
                state[3] = state[2];
                state[2] = state[1];
                state[1] = state[0];
                state[0] = t1 + t2;
            }

            for(i=0;i<8;i++)
                pResult[i] += state[i];
        }

        void finish(uint32_t *pResult, uint32_t *pBlock, unsigned int pBlockLength, uint64_t pTotalLength)
        {
            // Zeroize the end of the block
            std::memset(((uint8_t *)pBlock) + pBlockLength, 0, 64 - pBlockLength);

            // Add 0x80 byte (basically a 1 bit at the end of the data)
            ((uint8_t *)pBlock)[pBlockLength] = 0x80;

            // Check if there is enough room for the total length in this block
            if(pBlockLength > 55) // 55 because of the 0x80 byte already added
            {
                process(pResult, pBlock); // Process this block
                std::memset(pBlock, 0, 64); // Clear the block
            }

            // Put bit length in last 8 bytes of block (Big Endian)
            pTotalLength *= 8;
            uint32_t lower = pTotalLength & 0xffffffff;
            pBlock[15] = Endian::convert(lower, Endian::BIG);
            uint32_t upper = (pTotalLength >> 32) & 0xffffffff;
            pBlock[14] = Endian::convert(upper, Endian::BIG);

            // Process last block
            process(pResult, pBlock);

            // Adjust for big endian
            if(Endian::sSystemType != Endian::BIG)
                for(unsigned int i=0;i<8;i++)
                    pResult[i] = Endian::convert(pResult[i], Endian::BIG);
        }
    }

    void Digest::sha256(InputStream *pInput, unsigned int pInputLength, OutputStream *pOutput) // 256 bit(32 byte) result
    {
        unsigned int remaining = pInputLength;
        uint32_t result[8];
        uint32_t block[16];

        SHA256::initialize(result);

        while(remaining >= 64)
        {
            pInput->read(block, 64);
            remaining -= 64;
            SHA256::process(result, block);
        }

        pInput->read(block, remaining);
        SHA256::finish(result, block, remaining, pInputLength);

        pOutput->write(result, 32);
        return;
    }

    uint64_t SHA512_K[80] = {0x428a2f98d728ae22, 0x7137449123ef65cd, 0xb5c0fbcfec4d3b2f, 0xe9b5dba58189dbbc,
                             0x3956c25bf348b538, 0x59f111f1b605d019, 0x923f82a4af194f9b, 0xab1c5ed5da6d8118,
                             0xd807aa98a3030242, 0x12835b0145706fbe, 0x243185be4ee4b28c, 0x550c7dc3d5ffb4e2,
                             0x72be5d74f27b896f, 0x80deb1fe3b1696b1, 0x9bdc06a725c71235, 0xc19bf174cf692694,
                             0xe49b69c19ef14ad2, 0xefbe4786384f25e3, 0x0fc19dc68b8cd5b5, 0x240ca1cc77ac9c65,
                             0x2de92c6f592b0275, 0x4a7484aa6ea6e483, 0x5cb0a9dcbd41fbd4, 0x76f988da831153b5,
                             0x983e5152ee66dfab, 0xa831c66d2db43210, 0xb00327c898fb213f, 0xbf597fc7beef0ee4,
                             0xc6e00bf33da88fc2, 0xd5a79147930aa725, 0x06ca6351e003826f, 0x142929670a0e6e70,
                             0x27b70a8546d22ffc, 0x2e1b21385c26c926, 0x4d2c6dfc5ac42aed, 0x53380d139d95b3df,
                             0x650a73548baf63de, 0x766a0abb3c77b2a8, 0x81c2c92e47edaee6, 0x92722c851482353b,
                             0xa2bfe8a14cf10364, 0xa81a664bbc423001, 0xc24b8b70d0f89791, 0xc76c51a30654be30,
                             0xd192e819d6ef5218, 0xd69906245565a910, 0xf40e35855771202a, 0x106aa07032bbd1b8,
                             0x19a4c116b8d2d0c8, 0x1e376c085141ab53, 0x2748774cdf8eeb99, 0x34b0bcb5e19b48a8,
                             0x391c0cb3c5c95a63, 0x4ed8aa4ae3418acb, 0x5b9cca4f7763e373, 0x682e6ff3d6b2b8a3,
                             0x748f82ee5defb2fc, 0x78a5636f43172f60, 0x84c87814a1f0ab72, 0x8cc702081a6439ec,
                             0x90befffa23631e28, 0xa4506cebde82bde9, 0xbef9a3f7b2c67915, 0xc67178f2e372532b,
                             0xca273eceea26619c, 0xd186b8c721c0c207, 0xeada7dd6cde0eb1e, 0xf57d4f7fee6ed178,
                             0x06f067aa72176fba, 0x0a637dc5a2c898a6, 0x113f9804bef90dae, 0x1b710b35131c471b,
                             0x28db77f523047d84, 0x32caab7b40c72493, 0x3c9ebe0a15c9bebc, 0x431d67c49c100d4c,
                             0x4cc5d4becb3e42b6, 0x597f299cfc657e2a, 0x5fcb6fab3ad6faec, 0x6c44198c4a475817};

    /*void Digest::sha512(InputStream *pInput, unsigned int pInputLength, OutputStream *pOutput) // 512 bit(64 byte) result
    {
        Buffer message;
        message.setEndian(Endian::LITTLE);
        message.writeStream(pInput, pInputLength);

        // Append a one bit
        message.writeByte(0x80);

        // Make message size congruent to 896(mod 1024) bits(56(mod 64) bytes)
        while((message.length() + 16) % 128)
            message.writeByte(0);

        // Append message size
        message.writeUnsignedLong(0);
        message.writeUnsignedLong(pInputLength * 8);

        std::vector<uint64_t> hash, chunk, state;
        unsigned int i;
        uint64_t s0, s1, t1, t2;

        hash.push_back(0x6a09e667f3bcc908);
        hash.push_back(0xbb67ae8584caa73b);
        hash.push_back(0x3c6ef372fe94f82b);
        hash.push_back(0xa54ff53a5f1d36f1);
        hash.push_back(0x510e527fade682d1);
        hash.push_back(0x9b05688c2b3e6c1f);
        hash.push_back(0x1f83d9abfb41bd6b);
        hash.push_back(0x5be0cd19137e2179);

        while(message.remaining())
        {
            // Get a chunk of 16(64 bit integers) 128 bytes(1024 bits)
            chunk.clear();

            while(chunk.size() < 16)
                chunk.push_back(message.readUnsignedLong());

            // Extend the sixteen 64-bit words into sixty-four 64-bit words:
            for(i=16;i<80;i++)
            {
                s0 = chunk[i-15];
                s0 = Math::rotateRight64(s0, 1) ^ Math::rotateRight64(s0, 8) ^ Math::shiftRight64(s0, 7);

                s1 = chunk[i-2];
                s1 = Math::rotateRight64(s1, 19) ^ Math::rotateRight64(s1, 61) ^ Math::shiftRight64(s1, 6);

                chunk.push_back(chunk[i-16] + chunk[i-7] + s0 + s1);
            }

            state = hash;

            for(i=0;i<80;i++)
            {
                s0 = Math::rotateRight64(state[0], 28) ^ Math::rotateRight64(state[0], 34) ^ Math::rotateRight64(state[0], 39);
                t2 = s0 +((state[0] & state[1]) ^(state[0] & state[2]) ^(state[1] & state[2]));
                s1 = Math::rotateRight64(state[4], 14) ^ Math::rotateRight64(state [4], 18) ^ Math::rotateRight64(state[4], 41);
                t1 = state[7] + s1 +((state[4] & state[5]) ^(~state[4] & state[6])) + SHA512_K[i] + chunk[i];

                state[7] = state[6];
                state[6] = state[5];
                state[5] = state[4];
                state[4] = state[3] + t1;
                state[3] = state[2];
                state[2] = state[1];
                state[1] = state[0];
                state[0] = t1 + t2;
            }

            for(i=0;i<8;i++)
                hash[i] += state[i];
        }

        for(i=0;i<8;i++)
            pOutput->writeUnsignedLong(hash[i]);
    }*/

    Digest::Digest(Type pType)
    {
        mType = pType;
        mByteCount= 0;
        setOutputEndian(Endian::BIG);

        switch(mType)
        {
        case CRC32:
            mBlockSize = 1;
            mResultData = new uint32_t[1];
            mBlockData = NULL;
            break;
        //case MD5:
        //    mBlockSize = 64;
        //    break;
        //case SHA1:
        //    mBlockSize = 64;
        //    break;
        case RIPEMD160:
            mBlockSize = 64;
            mResultData = new uint32_t[5];
            mBlockData  = new uint32_t[mBlockSize / 4];
            break;
        case SHA256:
        case SHA256_SHA256:
        case SHA256_RIPEMD160:
            mBlockSize = 64;
            mResultData = new uint32_t[8];
            mBlockData  = new uint32_t[mBlockSize / 4];
            break;
        //case SHA512:
        //    mBlockSize = 128;
        //    break;
        default:
            mBlockSize = 0;
            mResultData = NULL;
            mBlockData = NULL;
            break;
        }

        initialize();
    }

    Digest::~Digest()
    {
        if(mBlockData != NULL)
            delete[] mBlockData;
        if(mResultData != NULL)
            delete[] mResultData;
    }

    void Digest::write(const void *pInput, unsigned int pSize)
    {
        mInput.write(pInput, pSize);
        mByteCount += pSize;
        process();
    }

    void Digest::initialize()
    {
        switch(mType)
        {
        case CRC32:
            mResultData[0] = 0xffffffff;
            break;
        //case MD5:
        //    break;
        //case SHA1:
        //    break;
        case RIPEMD160:
            RIPEMD160::init(mResultData);
            break;
        case SHA256:
        case SHA256_SHA256:
        case SHA256_RIPEMD160:
            SHA256::initialize(mResultData);
            break;
        //case SHA512:
        //    break;
        }
    }

    void Digest::process()
    {
        switch(mType)
        {
        case CRC32:
            while(mInput.remaining() >= mBlockSize)
                *mResultData = (*mResultData >> 8) ^ CRC32::table[(*mResultData & 0xFF) ^ mInput.readByte()];
            break;
        //case MD5:
        //    break;
        //case SHA1:
        //    break;
        case RIPEMD160:
            while(mInput.remaining() >= mBlockSize)
            {
                mInput.read(mBlockData, mBlockSize);
                RIPEMD160::process(mResultData, mBlockData);
            }
            break;
        case SHA256:
        case SHA256_SHA256:
        case SHA256_RIPEMD160:
            while(mInput.remaining() >= mBlockSize)
            {
                mInput.read(mBlockData, mBlockSize);
                SHA256::process(mResultData, mBlockData);
            }
            break;
        //case SHA512:
        //    break;
        }

        mInput.flush();
    }

    void Digest::getResult(RawOutputStream *pOutput)
    {
        switch(mType)
        {
        case CRC32:
            // Finalize result
            *mResultData ^= 0xffffffff;
            *mResultData = Endian::convert(*mResultData, Endian::BIG);

            // Output result
            pOutput->write(mResultData, 4);
            break;
        //case MD5:
        //    break;
        //case SHA1:
        //    break;
        case RIPEMD160:
        {
            // Get last partial block (must be less than 64 bytes)
            unsigned int lastBlockSize = mInput.remaining();
            mInput.read(mBlockData, lastBlockSize);

            RIPEMD160::finish(mResultData, mBlockData, lastBlockSize, mByteCount);

            // Output result
            pOutput->write(mResultData, 20);
            break;
        }
        case SHA256:
        {
            // Get last partial block (must be less than 64 bytes)
            unsigned int lastBlockSize = mInput.remaining();
            mInput.read(mBlockData, lastBlockSize);

            SHA256::finish(mResultData, mBlockData, lastBlockSize, mByteCount);

            // Output result
            pOutput->write(mResultData, 32);
            break;
        }
        case SHA256_SHA256:
        {
            // Get last partial block (must be less than 64 bytes)
            unsigned int lastBlockSize = mInput.remaining();
            mInput.read(mBlockData, lastBlockSize);

            SHA256::finish(mResultData, mBlockData, lastBlockSize, mByteCount);
            
            // Compute SHA256 of result
            Digest secondSHA256(SHA256);
            secondSHA256.write(mResultData, 32);
            Buffer secondSHA256Data(32);
            secondSHA256.getResult(&secondSHA256Data);

            // Output result
            pOutput->writeStream(&secondSHA256Data, 32);
            break;
        }
        case SHA256_RIPEMD160:
        {
            // Get last partial block (must be less than 64 bytes)
            unsigned int lastBlockSize = mInput.remaining();
            mInput.read(mBlockData, lastBlockSize);

            SHA256::finish(mResultData, mBlockData, lastBlockSize, mByteCount);

            // Compute SHA256 of result
            Digest ripEMD160(RIPEMD160);
            ripEMD160.write(mResultData, 32);
            Buffer ripEMD160Data(20);
            ripEMD160.getResult(&ripEMD160Data);

            // Output result
            pOutput->writeStream(&ripEMD160Data, 20);
            break;
        }
        //case SHA512:
        //    break;
        }
    }

    bool buffersMatch(Buffer &pLeft, Buffer &pRight)
    {
        if(pLeft.length() != pRight.length())
            return false;

        while(pLeft.remaining())
            if(pLeft.readByte() != pRight.readByte())
                return false;

        return true;
    }

    void logResults(const char *pDescription, Buffer &pBuffer)
    {
        Buffer hex;
        pBuffer.setReadOffset(0);
        hex.writeAsHex(&pBuffer, pBuffer.length(), true);
        char *hexText = new char[hex.length()];
        hex.read((uint8_t *)hexText, hex.length());
        Log::addFormatted(Log::VERBOSE, DIGEST_LOG_NAME, "%s : %s", pDescription, hexText);
        delete[] hexText;
    }

    bool Digest::test()
    {
        bool result = true;
        Buffer input, correctDigest, resultDigest;

        /******************************************************************************************
         * Test empty (All confirmed from outside sources)
         ******************************************************************************************/
        input.clear();

        /*****************************************************************************************
         * CRC32 empty
         *****************************************************************************************/
        input.setReadOffset(0);
        crc32(&input, input.length(), &resultDigest);
        correctDigest.writeHex("00000000");

        if(buffersMatch(correctDigest, resultDigest))
            Log::add(Log::INFO, DIGEST_LOG_NAME, "Passed CRC32 empty");
        else
        {
            Log::add(Log::ERROR, DIGEST_LOG_NAME, "Failed CRC32 empty");
            logResults("Correct Digest", correctDigest);
            logResults("Result Digest ", resultDigest);
            result = false;
        }

        correctDigest.clear();
        resultDigest.clear();

        /*****************************************************************************************
         * MD5 empty
         *****************************************************************************************/
        input.setReadOffset(0);
        md5(&input, input.length(), &resultDigest);
        correctDigest.writeHex("d41d8cd98f00b204e9800998ecf8427e");

        if(buffersMatch(correctDigest, resultDigest))
            Log::add(Log::INFO, DIGEST_LOG_NAME, "Passed MD5 empty");
        else
        {
            Log::add(Log::ERROR, DIGEST_LOG_NAME, "Failed MD5 empty");
            logResults("Correct Digest", correctDigest);
            logResults("Result Digest ", resultDigest);
            result = false;
        }

        correctDigest.clear();
        resultDigest.clear();

        /*****************************************************************************************
         * SHA1 empty
         *****************************************************************************************/
        input.setReadOffset(0);
        sha1(&input, input.length(), &resultDigest);
        correctDigest.writeHex("da39a3ee5e6b4b0d3255bfef95601890afd80709");

        if(buffersMatch(correctDigest, resultDigest))
            Log::add(Log::INFO, DIGEST_LOG_NAME, "Passed SHA1 empty");
        else
        {
            Log::add(Log::ERROR, DIGEST_LOG_NAME, "Failed SHA1 empty");
            logResults("Correct Digest", correctDigest);
            logResults("Result Digest ", resultDigest);
            result = false;
        }

        correctDigest.clear();
        resultDigest.clear();

        /*****************************************************************************************
         * RIPEMD160 empty
         *****************************************************************************************/
        input.setReadOffset(0);
        ripEMD160(&input, input.length(), &resultDigest);
        correctDigest.writeHex("9c1185a5c5e9fc54612808977ee8f548b2258d31");

        if(buffersMatch(correctDigest, resultDigest))
            Log::add(Log::INFO, DIGEST_LOG_NAME, "Passed RIPEMD160 empty");
        else
        {
            Log::add(Log::ERROR, DIGEST_LOG_NAME, "Failed RIPEMD160 empty");
            logResults("Correct Digest", correctDigest);
            logResults("Result Digest ", resultDigest);
            result = false;
        }

        correctDigest.clear();
        resultDigest.clear();

        /*****************************************************************************************
         * RIPEMD160 digest empty
         *****************************************************************************************/
        input.setReadOffset(0);
        Digest ripemd160EmptyDigest(RIPEMD160);
        ripemd160EmptyDigest.writeStream(&input, input.length());
        ripemd160EmptyDigest.getResult(&resultDigest);
        correctDigest.writeHex("9c1185a5c5e9fc54612808977ee8f548b2258d31");

        if(buffersMatch(correctDigest, resultDigest))
            Log::add(Log::INFO, DIGEST_LOG_NAME, "Passed RIPEMD160 digest empty");
        else
        {
            Log::add(Log::ERROR, DIGEST_LOG_NAME, "Failed RIPEMD160 digest empty");
            logResults("Correct Digest", correctDigest);
            logResults("Result Digest ", resultDigest);
            result = false;
        }

        correctDigest.clear();
        resultDigest.clear();

        /*****************************************************************************************
         * SHA256 empty
         *****************************************************************************************/
        input.setReadOffset(0);
        sha256(&input, input.length(), &resultDigest);
        correctDigest.writeHex("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");

        if(buffersMatch(correctDigest, resultDigest))
            Log::add(Log::INFO, DIGEST_LOG_NAME, "Passed SHA256 empty");
        else
        {
            Log::add(Log::ERROR, DIGEST_LOG_NAME, "Failed SHA256 empty");
            logResults("Correct Digest", correctDigest);
            logResults("Result Digest ", resultDigest);
            result = false;
        }

        correctDigest.clear();
        resultDigest.clear();

        /*****************************************************************************************
         * SHA256 digest empty
         *****************************************************************************************/
        input.setReadOffset(0);
        Digest sha256EmptyDigest(SHA256);
        sha256EmptyDigest.writeStream(&input, input.length());
        sha256EmptyDigest.getResult(&resultDigest);
        correctDigest.writeHex("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");

        if(buffersMatch(correctDigest, resultDigest))
            Log::add(Log::INFO, DIGEST_LOG_NAME, "Passed SHA256 digest empty");
        else
        {
            Log::add(Log::ERROR, DIGEST_LOG_NAME, "Failed SHA256 digest empty");
            logResults("Correct Digest", correctDigest);
            logResults("Result Digest ", resultDigest);
            result = false;
        }

        correctDigest.clear();
        resultDigest.clear();

        /*****************************************************************************************
         * SHA512 empty
         *****************************************************************************************/
        // input.setReadOffset(0);
        // sha512(&input, input.length(), &resultDigest);
        // correctDigest.writeHex("cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e");

        // if(buffersMatch(correctDigest, resultDigest))
            // Log::add(Log::INFO, DIGEST_LOG_NAME, "Passed SHA512 empty");
        // else
        // {
            // Log::add(Log::ERROR, DIGEST_LOG_NAME, "Failed SHA512 empty");
            // logResults("Correct Digest", correctDigest);
            // logResults("Result Digest ", resultDigest);
            // result = false;
        // }

        // correctDigest.clear();
        // resultDigest.clear();

        /******************************************************************************************
         * Test febooti.com (All confirmed from outside sources)
         ******************************************************************************************/
        input.clear();
        input.writeString("Test vector from febooti.com");

        /*****************************************************************************************
         * CRC32 febooti.com
         *****************************************************************************************/
        input.setReadOffset(0);
        crc32(&input, input.length(), &resultDigest);
        correctDigest.writeHex("0c877f61");

        if(buffersMatch(correctDigest, resultDigest))
            Log::add(Log::INFO, DIGEST_LOG_NAME, "Passed CRC32 febooti.com");
        else
        {
            Log::add(Log::ERROR, DIGEST_LOG_NAME, "Failed CRC32 febooti.com");
            logResults("Correct Digest", correctDigest);
            logResults("Result Digest ", resultDigest);
            result = false;
        }

        correctDigest.clear();
        resultDigest.clear();

        /*****************************************************************************************
         * CRC32 text febooti.com
         *****************************************************************************************/
        unsigned int crc32Text = crc32("Test vector from febooti.com");
        unsigned int crc32Result = Endian::convert(0x0c877f61, Endian::BIG);
        if(crc32Text != crc32Result)
            Log::addFormatted(Log::INFO, DIGEST_LOG_NAME, "Passed CRC32 text febooti.com");
        else
        {
            Log::addFormatted(Log::ERROR, DIGEST_LOG_NAME, "Failed CRC32 text febooti.com : 0x0c877f61 != 0x%08x", crc32Text);
            result = false;
        }

        /*****************************************************************************************
         * CRC32 binary febooti.com
         *****************************************************************************************/
        unsigned int crc32Binary = crc32((uint8_t *)"Test vector from febooti.com", 28);
        if(crc32Binary != crc32Result)
            Log::addFormatted(Log::INFO, DIGEST_LOG_NAME, "Passed CRC32 binary febooti.com");
        else
        {
            Log::addFormatted(Log::ERROR, DIGEST_LOG_NAME, "Failed CRC32 binary febooti.com : 0x0c877f61 != 0x%08x", crc32Binary);
            result = false;
        }

        /*****************************************************************************************
         * MD5 febooti.com
         *****************************************************************************************/
        input.setReadOffset(0);
        md5(&input, input.length(), &resultDigest);
        correctDigest.writeHex("500ab6613c6db7fbd30c62f5ff573d0f");

        if(buffersMatch(correctDigest, resultDigest))
            Log::add(Log::INFO, DIGEST_LOG_NAME, "Passed MD5 febooti.com");
        else
        {
            Log::add(Log::ERROR, DIGEST_LOG_NAME, "Failed MD5 febooti.com");
            logResults("Correct Digest", correctDigest);
            logResults("Result Digest ", resultDigest);
            result = false;
        }

        correctDigest.clear();
        resultDigest.clear();

        /*****************************************************************************************
         * SHA1 febooti.com
         *****************************************************************************************/
        input.setReadOffset(0);
        sha1(&input, input.length(), &resultDigest);
        correctDigest.writeHex("a7631795f6d59cd6d14ebd0058a6394a4b93d868");

        if(buffersMatch(correctDigest, resultDigest))
            Log::add(Log::INFO, DIGEST_LOG_NAME, "Passed SHA1 febooti.com");
        else
        {
            Log::add(Log::ERROR, DIGEST_LOG_NAME, "Failed SHA1 febooti.com");
            logResults("Correct Digest", correctDigest);
            logResults("Result Digest ", resultDigest);
            result = false;
        }

        correctDigest.clear();
        resultDigest.clear();

        /*****************************************************************************************
         * RIPEMD160 febooti.com
         *****************************************************************************************/
        input.setReadOffset(0);
        ripEMD160(&input, input.length(), &resultDigest);
        correctDigest.writeHex("4e1ff644ca9f6e86167ccb30ff27e0d84ceb2a61");

        if(buffersMatch(correctDigest, resultDigest))
            Log::add(Log::INFO, DIGEST_LOG_NAME, "Passed RIPEMD160 febooti.com");
        else
        {
            Log::add(Log::ERROR, DIGEST_LOG_NAME, "Failed RIPEMD160 febooti.com");
            logResults("Correct Digest", correctDigest);
            logResults("Result Digest ", resultDigest);
            result = false;
        }

        correctDigest.clear();
        resultDigest.clear();

        /*****************************************************************************************
         * SHA256 febooti.com
         *****************************************************************************************/
        input.setReadOffset(0);
        sha256(&input, input.length(), &resultDigest);
        correctDigest.writeHex("077b18fe29036ada4890bdec192186e10678597a67880290521df70df4bac9ab");

        if(buffersMatch(correctDigest, resultDigest))
            Log::add(Log::INFO, DIGEST_LOG_NAME, "Passed SHA256 febooti.com");
        else
        {
            Log::add(Log::ERROR, DIGEST_LOG_NAME, "Failed SHA256 febooti.com");
            logResults("Correct Digest", correctDigest);
            logResults("Result Digest ", resultDigest);
            result = false;
        }

        correctDigest.clear();
        resultDigest.clear();

        /*****************************************************************************************
         * SHA512 febooti.com
         *****************************************************************************************/
        // input.setReadOffset(0);
        // sha512(&input, input.length(), &resultDigest);
        // correctDigest.writeHex("09fb898bc97319a243a63f6971747f8e102481fb8d5346c55cb44855adc2e0e98f304e552b0db1d4eeba8a5c8779f6a3010f0e1a2beb5b9547a13b6edca11e8a");

        // if(buffersMatch(correctDigest, resultDigest))
            // Log::add(Log::INFO, DIGEST_LOG_NAME, "Passed SHA512 febooti.com");
        // else
        // {
            // Log::add(Log::ERROR, DIGEST_LOG_NAME, "Failed SHA512 febooti.com");
            // logResults("Correct Digest", correctDigest);
            // logResults("Result Digest ", resultDigest);
            // result = false;
        // }

        // correctDigest.clear();
        // resultDigest.clear();

        /******************************************************************************************
         * Test quick brown fox (All confirmed from outside sources)
         ******************************************************************************************/
        input.clear();
        input.writeString("The quick brown fox jumps over the lazy dog");

        /*****************************************************************************************
         * CRC32 quick brown fox
         *****************************************************************************************/
        input.setReadOffset(0);
        crc32(&input, input.length(), &resultDigest);
        correctDigest.writeHex("414FA339");

        if(buffersMatch(correctDigest, resultDigest))
            Log::add(Log::INFO, DIGEST_LOG_NAME, "Passed CRC32 quick brown fox");
        else
        {
            Log::add(Log::ERROR, DIGEST_LOG_NAME, "Failed CRC32 quick brown fox");
            logResults("Correct Digest", correctDigest);
            logResults("Result Digest ", resultDigest);
            result = false;
        }

        correctDigest.clear();
        resultDigest.clear();

        /*****************************************************************************************
         * MD5 quick brown fox
         *****************************************************************************************/
        input.setReadOffset(0);
        md5(&input, input.length(), &resultDigest);
        correctDigest.writeHex("9e107d9d372bb6826bd81d3542a419d6");

        if(buffersMatch(correctDigest, resultDigest))
            Log::add(Log::INFO, DIGEST_LOG_NAME, "Passed MD5 quick brown fox");
        else
        {
            Log::add(Log::ERROR, DIGEST_LOG_NAME, "Failed MD5 quick brown fox");
            logResults("Correct Digest", correctDigest);
            logResults("Result Digest ", resultDigest);
            result = false;
        }

        correctDigest.clear();
        resultDigest.clear();

        /*****************************************************************************************
         * SHA1 quick brown fox
         *****************************************************************************************/
        input.setReadOffset(0);
        sha1(&input, input.length(), &resultDigest);
        correctDigest.writeHex("2fd4e1c67a2d28fced849ee1bb76e7391b93eb12");

        if(buffersMatch(correctDigest, resultDigest))
            Log::add(Log::INFO, DIGEST_LOG_NAME, "Passed SHA1 quick brown fox");
        else
        {
            Log::add(Log::ERROR, DIGEST_LOG_NAME, "Failed SHA1 quick brown fox");
            logResults("Correct Digest", correctDigest);
            logResults("Result Digest ", resultDigest);
            result = false;
        }

        correctDigest.clear();
        resultDigest.clear();

        /*****************************************************************************************
         * RIPEMD160 quick brown fox
         *****************************************************************************************/
        input.setReadOffset(0);
        ripEMD160(&input, input.length(), &resultDigest);
        correctDigest.writeHex("37f332f68db77bd9d7edd4969571ad671cf9dd3b");

        if(buffersMatch(correctDigest, resultDigest))
            Log::add(Log::INFO, DIGEST_LOG_NAME, "Passed RIPEMD160 quick brown fox");
        else
        {
            Log::add(Log::ERROR, DIGEST_LOG_NAME, "Failed RIPEMD160 quick brown fox");
            logResults("Correct Digest", correctDigest);
            logResults("Result Digest ", resultDigest);
            result = false;
        }

        correctDigest.clear();
        resultDigest.clear();

        /*****************************************************************************************
         * SHA256 quick brown fox
         *****************************************************************************************/
        input.setReadOffset(0);
        sha256(&input, input.length(), &resultDigest);
        correctDigest.writeHex("d7a8fbb307d7809469ca9abcb0082e4f8d5651e46d3cdb762d02d0bf37c9e592");

        if(buffersMatch(correctDigest, resultDigest))
            Log::add(Log::INFO, DIGEST_LOG_NAME, "Passed SHA256 quick brown fox");
        else
        {
            Log::add(Log::ERROR, DIGEST_LOG_NAME, "Failed SHA256 quick brown fox");
            logResults("Correct Digest", correctDigest);
            logResults("Result Digest ", resultDigest);
            result = false;
        }

        correctDigest.clear();
        resultDigest.clear();

        /*****************************************************************************************
         * SHA512 quick brown fox
         *****************************************************************************************/
        // input.setReadOffset(0);
        // sha512(&input, input.length(), &resultDigest);
        // correctDigest.writeHex("07e547d9586f6a73f73fbac0435ed76951218fb7d0c8d788a309d785436bbb642e93a252a954f23912547d1e8a3b5ed6e1bfd7097821233fa0538f3db854fee6");

        // if(buffersMatch(correctDigest, resultDigest))
            // Log::add(Log::INFO, DIGEST_LOG_NAME, "Passed SHA512 quick brown fox");
        // else
        // {
            // Log::add(Log::ERROR, DIGEST_LOG_NAME, "Failed SHA512 quick brown fox");
            // logResults("Correct Digest", correctDigest);
            // logResults("Result Digest ", resultDigest);
            // result = false;
        // }

        // correctDigest.clear();
        // resultDigest.clear();

        /******************************************************************************************
         * Test random data 1024 (All confirmed from outside sources)
         ******************************************************************************************/
        input.clear();
        input.writeHex("9cd248ed860b10bbc7cd5f0ef18f81291a90307c91296dc67d3a1759f02e2a34db020eb9c1f401c1");
        input.writeHex("1820349dc9401246bab85810989136420a49830fb96a28e22247ec1073536862e6c2ce82adb93b1d");
        input.writeHex("b3193a938dfefe8db8aef7eefb784e6af35191b7bf79dd96da777d7b2423fd49c255839232934344");
        input.writeHex("41c94a6e2aa84e926a40ff9e640e224d0241a89565feb539791b2dcb31185b3ce463d638c99db0ed");
        input.writeHex("e615dded590af0c89e093d0db3637aac61cd052c776409d992ddfc0249221121909bea8085871db2");
        input.writeHex("a00011dc46d159b5ce630efff43117e379d7bb105142f4ef6e3af41ff0284624d16987b7ee6187e7");
        input.writeHex("3df4761d2710414f216310b8193c530568ce423b76cc0342ad0ed86a3e7c15530c54ab4022ebaeed");
        input.writeHex("df4e7996fec005e2d62b3ec1097af9b29443d45531399490cd763a78d58682fcf3bb483aa7448a44");
        input.writeHex("9ac089cf695a9285954751f4c139904d10512c3e1adb00de962f4912f5fd160ade61c7e8ba45c5b9");
        input.writeHex("4a763c943bf30249026f1c9eaf9be10f3f47ac2f001f633f5df3774bbcbc6cb85738a0d74778a07e");
        input.writeHex("736adfd769c99509d2aad922d49b6b1c67fe886578e95988961e20c64ed6b7e4e080bbda3ce24ce6");
        input.writeHex("741c51cacf401cc8b373ed6170d7b70a033f553eaef18d94065f06699d6bcd0bf5d845e09fd364e3");
        input.writeHex("98e96d3ed54f78dc5d6200560001a3c0f721ccba58eaf9fde2760e937b820e0c41c161530d1f6b25");
        input.writeHex("6b30463fd1dfe3e9d293afd5f278bf21e2b8bfc8860f7c86f4575cd7a922e4d9dbb8857815ede9a7");
        input.writeHex("628af97c7ecadf59d385de8f2a3b3d114344fd9429f15a4aabe42c3934347bc039121dd666c6cef7");
        input.writeHex("a81822889f394b82458f4016ed947fb86d8ef15b40d2a36b751f983339eeb7d4880554c5feebf6a6");
        input.writeHex("59467f9716afc92ad05b41aab06e958f5874d431c836419ed2c595282c6804c600e97ce3929380d9");
        input.writeHex("7f2687cc210890f95b3cf428ed66cb4e853505ba380bad5bda6c89b835c711a980ea946279051ea8");
        input.writeHex("d12d002a52e40b0b162e7ff1464a9474450980ff3354a04522dc869905573ee0418adbe5938e87f2");
        input.writeHex("0c3761960bf64c21de39ff305420a2127de03afdc5d117489271671219ccd538c0944ecd9ea869dc");
        input.writeHex("135246b03b5ac5474b8d7c1741f68bbe616048c53ebc49814c757435a0f82c48bee85c339bbfb134");
        input.writeHex("d4b64f561ca82ca1413eef619966d1e34bffb2771d069f795682e9559991d6239713fca03975d8fd");
        input.writeHex("e0c2fd4cfe37daf274a3298fdfb9637191524505aa608573b819b0271b97a76328130c0ad8b60d3d");
        input.writeHex("e53272e3e3b49760bfd3d20e5fc57bc5baa4b070c91f4eedd5e398405ac47a4bfa307747449ce0ad");
        input.writeHex("7b9e9e6e1cc3b4bdef0be7b773af02b590626c92e3a85e97e0726ac1f7061e149c550a8d1b17360d");
        input.writeHex("b22d56251b4fb0a6bb40595d1001d87d799d8544fdc54dfc");

        /*****************************************************************************************
         * CRC32 random data 1024
         *****************************************************************************************/
        input.setReadOffset(0);
        crc32(&input, input.length(), &resultDigest);
        correctDigest.writeHex("1f483b3f");

        if(buffersMatch(correctDigest, resultDigest))
            Log::add(Log::INFO, DIGEST_LOG_NAME, "Passed CRC32 random data 1024");
        else
        {
            Log::add(Log::ERROR, DIGEST_LOG_NAME, "Failed CRC32 random data 1024");
            logResults("Correct Digest", correctDigest);
            logResults("Result Digest ", resultDigest);
            result = false;
        }

        correctDigest.clear();
        resultDigest.clear();

        /*****************************************************************************************
         * CRC32 digest random data 1024
         *****************************************************************************************/
        input.setReadOffset(0);
        Digest crc32Digest(CRC32);
        crc32Digest.writeStream(&input, input.length());
        crc32Digest.getResult(&resultDigest);
        correctDigest.writeHex("1f483b3f");

        if(buffersMatch(correctDigest, resultDigest))
            Log::add(Log::INFO, DIGEST_LOG_NAME, "Passed CRC32 digest random data 1024");
        else
        {
            Log::add(Log::ERROR, DIGEST_LOG_NAME, "Failed CRC32 digest random data 1024");
            logResults("Correct Digest", correctDigest);
            logResults("Result Digest ", resultDigest);
            result = false;
        }

        correctDigest.clear();
        resultDigest.clear();

        /*****************************************************************************************
         * MD5 random data 1024
         *****************************************************************************************/
        input.setReadOffset(0);
        md5(&input, input.length(), &resultDigest);
        correctDigest.writeHex("6950a08814ee1e774314c28bce8707b0");

        if(buffersMatch(correctDigest, resultDigest))
            Log::add(Log::INFO, DIGEST_LOG_NAME, "Passed MD5 random data 1024");
        else
        {
            Log::add(Log::ERROR, DIGEST_LOG_NAME, "Failed MD5 random data 1024");
            logResults("Correct Digest", correctDigest);
            logResults("Result Digest ", resultDigest);
            result = false;
        }

        correctDigest.clear();
        resultDigest.clear();

        /*****************************************************************************************
         * SHA1 random data 1024
         *****************************************************************************************/
        input.setReadOffset(0);
        sha1(&input, input.length(), &resultDigest);
        correctDigest.writeHex("2F7A0D349F1B6ABD7354965E94800BDC3D6463AC");

        if(buffersMatch(correctDigest, resultDigest))
            Log::add(Log::INFO, DIGEST_LOG_NAME, "Passed SHA1 random data 1024");
        else
        {
            Log::add(Log::ERROR, DIGEST_LOG_NAME, "Failed SHA1 random data 1024");
            logResults("Correct Digest", correctDigest);
            logResults("Result Digest ", resultDigest);
            result = false;
        }

        correctDigest.clear();
        resultDigest.clear();

        /*****************************************************************************************
         * RIPEMD160 random data 1024
         *****************************************************************************************/
        input.setReadOffset(0);
        ripEMD160(&input, input.length(), &resultDigest);
        correctDigest.writeHex("0dae1c4a362242d2ffa49c26204ed5ac2f88c454");

        if(buffersMatch(correctDigest, resultDigest))
            Log::add(Log::INFO, DIGEST_LOG_NAME, "Passed RIPEMD160 random data 1024");
        else
        {
            Log::add(Log::ERROR, DIGEST_LOG_NAME, "Failed RIPEMD160 random data 1024");
            logResults("Correct Digest", correctDigest);
            logResults("Result Digest ", resultDigest);
            result = false;
        }

        correctDigest.clear();
        resultDigest.clear();

        /*****************************************************************************************
         * RIPEMD160 digest random data 1024
         *****************************************************************************************/
        input.setReadOffset(0);
        Digest ripemd160Digest(RIPEMD160);
        ripemd160Digest.writeStream(&input, input.length());
        ripemd160Digest.getResult(&resultDigest);
        correctDigest.writeHex("0dae1c4a362242d2ffa49c26204ed5ac2f88c454");

        if(buffersMatch(correctDigest, resultDigest))
            Log::add(Log::INFO, DIGEST_LOG_NAME, "Passed RIPEMD160 digest random data 1024");
        else
        {
            Log::add(Log::ERROR, DIGEST_LOG_NAME, "Failed RIPEMD160 digest random data 1024");
            logResults("Correct Digest", correctDigest);
            logResults("Result Digest ", resultDigest);
            result = false;
        }

        correctDigest.clear();
        resultDigest.clear();

        /*****************************************************************************************
         * SHA256 random data 1024
         *****************************************************************************************/
        input.setReadOffset(0);
        sha256(&input, input.length(), &resultDigest);
        correctDigest.writeHex("2baef0b3638abc90b17f2895e3cb24b6bbe7ff6ba7c291345102ea4eec785730");

        if(buffersMatch(correctDigest, resultDigest))
            Log::add(Log::INFO, DIGEST_LOG_NAME, "Passed SHA256 random data 1024");
        else
        {
            Log::add(Log::ERROR, DIGEST_LOG_NAME, "Failed SHA256 random data 1024");
            logResults("Correct Digest", correctDigest);
            logResults("Result Digest ", resultDigest);
            result = false;
        }

        correctDigest.clear();
        resultDigest.clear();

        /*****************************************************************************************
         * SHA256 digest random data 1024
         *****************************************************************************************/
        input.setReadOffset(0);
        Digest sha256Digest(SHA256);
        sha256Digest.writeStream(&input, input.length());
        sha256Digest.getResult(&resultDigest);
        correctDigest.writeHex("2baef0b3638abc90b17f2895e3cb24b6bbe7ff6ba7c291345102ea4eec785730");

        if(buffersMatch(correctDigest, resultDigest))
            Log::add(Log::INFO, DIGEST_LOG_NAME, "Passed SHA256 digest random data 1024");
        else
        {
            Log::add(Log::ERROR, DIGEST_LOG_NAME, "Failed SHA256 digest random data 1024");
            logResults("Correct Digest", correctDigest);
            logResults("Result Digest ", resultDigest);
            result = false;
        }

        correctDigest.clear();
        resultDigest.clear();

        /*****************************************************************************************
         * SHA512 random data 1024
         *****************************************************************************************/
        // input.setReadOffset(0);
        // sha512(&input, input.length(), &resultDigest);
        // correctDigest.writeHex("8c63c499586f24f3209acad229b043f02eddfc19ec04d41c2f0aeee60b3a95e87297b2de4cfaaaca9a6691bbc5f63a0453fa98b02742da313fa9075ef633a94c");

        // if(buffersMatch(correctDigest, resultDigest))
            // Log::add(Log::INFO, DIGEST_LOG_NAME, "Passed SHA512 random data 1024");
        // else
        // {
            // Log::add(Log::ERROR, DIGEST_LOG_NAME, "Failed SHA512 random data 1024");
            // logResults("Correct Digest", correctDigest);
            // logResults("Result Digest ", resultDigest);
            // result = false;
        // }

        // correctDigest.clear();
        // resultDigest.clear();

        /******************************************************************************************
         * Test 56 letters (All confirmed from outside sources)
         ******************************************************************************************/
        input.clear();
        input.writeString("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq");

        /*****************************************************************************************
         * RIPEMD160 56 letters
         *****************************************************************************************/
        input.setReadOffset(0);
        ripEMD160(&input, input.length(), &resultDigest);
        correctDigest.writeHex("12a053384a9c0c88e405a06c27dcf49ada62eb2b");

        if(buffersMatch(correctDigest, resultDigest))
            Log::add(Log::INFO, DIGEST_LOG_NAME, "Passed RIPEMD160 56 letters");
        else
        {
            Log::add(Log::ERROR, DIGEST_LOG_NAME, "Failed RIPEMD160 56 letters");
            logResults("Correct Digest", correctDigest);
            logResults("Result Digest ", resultDigest);
            result = false;
        }

        correctDigest.clear();
        resultDigest.clear();

        /******************************************************************************************
         * Test 8 times "1234567890" (All confirmed from outside sources)
         ******************************************************************************************/
        input.clear();
        input.writeString("12345678901234567890123456789012345678901234567890123456789012345678901234567890");

        /*****************************************************************************************
         * RIPEMD160 8 times "1234567890"
         *****************************************************************************************/
        input.setReadOffset(0);
        ripEMD160(&input, input.length(), &resultDigest);
        correctDigest.writeHex("9b752e45573d4b39f4dbd3323cab82bf63326bfb");

        if(buffersMatch(correctDigest, resultDigest))
            Log::add(Log::INFO, DIGEST_LOG_NAME, "Passed RIPEMD160 8 times \"1234567890\"");
        else
        {
            Log::add(Log::ERROR, DIGEST_LOG_NAME, "Failed RIPEMD160 8 times \"1234567890\"");
            logResults("Correct Digest", correctDigest);
            logResults("Result Digest ", resultDigest);
            result = false;
        }

        correctDigest.clear();
        resultDigest.clear();

        /******************************************************************************************
         * Test million a (All confirmed from outside sources)
         ******************************************************************************************/
        input.clear();
        for(unsigned int i=0;i<1000000;i++)
            input.writeByte('a');

        /*****************************************************************************************
         * RIPEMD160 million a
         *****************************************************************************************/
        input.setReadOffset(0);
        ripEMD160(&input, input.length(), &resultDigest);
        correctDigest.writeHex("52783243c1697bdbe16d37f97f68f08325dc1528");

        if(buffersMatch(correctDigest, resultDigest))
            Log::add(Log::INFO, DIGEST_LOG_NAME, "Passed RIPEMD160 million a");
        else
        {
            Log::add(Log::ERROR, DIGEST_LOG_NAME, "Failed RIPEMD160 million a");
            logResults("Correct Digest", correctDigest);
            logResults("Result Digest ", resultDigest);
            result = false;
        }

        correctDigest.clear();
        resultDigest.clear();

        /******************************************************************************************
         * Test random data 150 (All confirmed from outside sources)
         ******************************************************************************************/
        input.clear();
        input.writeHex("182d86ed47df1c1673558e3d1ed08dcc7de3a906615589084f6316e71cabd18e7c37451d514d9ede653b03d047345a522ef1c55f27ac8bff3564635116855d862bac06d21f8abb3026746b5c74dd46e9679bd30137bf6b143b67249931ff3f0a3f50426a4479871d15603aa4151ef4b9321762553df9946f5bc194ac4a44e94205d8b0854f1722ea6915770f03bc598c306cabf23878");

        /*****************************************************************************************
         * RIPEMD160 random data 150
         *****************************************************************************************/
        input.setReadOffset(0);
        ripEMD160(&input, input.length(), &resultDigest);
        correctDigest.writeHex("de4c02fe629897e3a2658c042f260a96ccfccac9");

        if(buffersMatch(correctDigest, resultDigest))
            Log::add(Log::INFO, DIGEST_LOG_NAME, "Passed RIPEMD160 random data 150");
        else
        {
            Log::add(Log::ERROR, DIGEST_LOG_NAME, "Failed RIPEMD160 random data 150");
            logResults("Correct Digest", correctDigest);
            logResults("Result Digest ", resultDigest);
            result = false;
        }

        correctDigest.clear();
        resultDigest.clear();

        /******************************************************************************************
         * Test hello
         ******************************************************************************************/
        input.clear();
        input.writeString("hello");

        /*****************************************************************************************
         * SHA256_SHA256 hello
         *****************************************************************************************/
        input.setReadOffset(0);
        Digest doubleSHA256(SHA256_SHA256);
        doubleSHA256.writeStream(&input, input.length());
        doubleSHA256.getResult(&resultDigest);
        correctDigest.writeHex("9595c9df90075148eb06860365df33584b75bff782a510c6cd4883a419833d50");

        if(buffersMatch(correctDigest, resultDigest))
            Log::add(Log::INFO, DIGEST_LOG_NAME, "Passed SHA256_SHA256 hello");
        else
        {
            Log::add(Log::ERROR, DIGEST_LOG_NAME, "Failed SHA256_SHA256 hello");
            logResults("Correct Digest", correctDigest);
            logResults("Result Digest ", resultDigest);
            result = false;
        }

        correctDigest.clear();
        resultDigest.clear();

        /*****************************************************************************************
         * SHA256_RIPEMD160 hello
         *****************************************************************************************/
        input.setReadOffset(0);
        Digest sha256RIPEMD160(SHA256_RIPEMD160);
        sha256RIPEMD160.writeStream(&input, input.length());
        sha256RIPEMD160.getResult(&resultDigest);
        correctDigest.writeHex("b6a9c8c230722b7c748331a8b450f05566dc7d0f");

        if(buffersMatch(correctDigest, resultDigest))
            Log::add(Log::INFO, DIGEST_LOG_NAME, "Passed SHA256_RIPEMD160 hello");
        else
        {
            Log::add(Log::ERROR, DIGEST_LOG_NAME, "Failed SHA256_RIPEMD160 hello");
            logResults("Correct Digest", correctDigest);
            logResults("Result Digest ", resultDigest);
            result = false;
        }

        correctDigest.clear();
        resultDigest.clear();

        return result;
    }
}