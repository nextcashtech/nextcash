/**************************************************************************
 * Copyright 2017 ArcMist, LLC                                            *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@arcmist.com>                                    *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#ifndef ARCMIST_DIGEST_HPP
#define ARCMIST_DIGEST_HPP

#include "arcmist/base/string.hpp"
#include "arcmist/io/stream.hpp"
#include "arcmist/io/buffer.hpp"

#include <cstdint>


namespace ArcMist
{
    class Digest : public OutputStream
    {
    public:

        enum Type { CRC32, SHA1, RIPEMD160, SHA256, SHA256_SHA256, SHA256_RIPEMD160 }; //TODO Not yet supported - MD5, SHA512

        Digest(Type pType);
        ~Digest();

        // Set data to initial state. Automatically called by constructor
        void initialize();

        // Note : Use write functions inherited from OutputStream to add data to the digest

        // Calculate result
        void getResult(RawOutputStream *pOutput);

        // Static digest calculation functions
        static void crc32(InputStream *pInput, unsigned int pInputLength, OutputStream *pOutput); // 32 bit(4 byte) result
        static uint32_t crc32(const char *pText);
        static uint32_t crc32(const uint8_t *pData, unsigned int pSize);
        static void md5(InputStream *pInput, unsigned int pInputLength, OutputStream *pOutput);  // 128 bit(16 byte) result
        static void sha1(InputStream *pInput, unsigned int pInputLength, OutputStream *pOutput);  // 160 bit(20 byte) result
        static void ripEMD160(InputStream *pInput, unsigned int pInputLength, OutputStream *pOutput);  // 160 bit(20 bytes) result
        static void sha256(InputStream *pInput, unsigned int pInputLength, OutputStream *pOutput);  // 256 bit(32 bytes) result
        //TODO fix endian issues //static void sha512(InputStream *pInput, unsigned int pInputLength, OutputStream *pOutput);  // 512 bit(64 bytes) result
        static uint64_t sipHash24(uint8_t *pData, unsigned int pLength, uint64_t pKey0, uint64_t pKey1);

        // Virtual overloaded functions
        unsigned int writeOffset() const { return mByteCount; }
        void write(const void *pInput, unsigned int pSize);

        static bool test();

    private:

        Type mType;
        unsigned int mByteCount;
        Buffer mInput;
        unsigned int mBlockSize;
        uint32_t *mBlockData, *mResultData;

        // Process as many blocks of data as possible
        void process();

    };
}

#endif
