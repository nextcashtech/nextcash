/**************************************************************************
 * Copyright 2017-2018 NextCash, LLC                                       *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@nextcash.com>                                    *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#ifndef NEXTCASH_DIGEST_HPP
#define NEXTCASH_DIGEST_HPP

#include "nextcash/base/string.hpp"
#include "nextcash/io/stream.hpp"
#include "nextcash/io/buffer.hpp"

#include <cstdint>


namespace NextCash
{
    class Digest : public OutputStream
    {
    public:

        enum Type { CRC32, SHA1, RIPEMD160, SHA256, SHA256_SHA256, SHA256_RIPEMD160, SHA512, MURMUR3 }; //TODO Not yet supported - MD5

        Digest(Type pType);
        ~Digest();

        // Set data to initial state. Automatically called by constructor
        void initialize(uint32_t pSeed = 0); // Seed only used for MURMUR3

        // Note : Use write functions inherited from OutputStream to add data to the digest

        // Calculate result
        void getResult(RawOutputStream *pOutput);
        unsigned int getResult();

        // Static digest calculation functions
        static void crc32(InputStream *pInput, stream_size pInputLength, OutputStream *pOutput); // 32 bit(4 byte) result
        static uint32_t crc32(const char *pText);
        static uint32_t crc32(const uint8_t *pData, stream_size pSize);
        static void md5(InputStream *pInput, stream_size pInputLength, OutputStream *pOutput);  // 128 bit(16 byte) result
        static void sha1(InputStream *pInput, stream_size pInputLength, OutputStream *pOutput);  // 160 bit(20 byte) result
        static void ripEMD160(InputStream *pInput, stream_size pInputLength, OutputStream *pOutput);  // 160 bit(20 bytes) result
        static void sha256(InputStream *pInput, stream_size pInputLength, OutputStream *pOutput);  // 256 bit(32 bytes) result
        static void sha512(InputStream *pInput, stream_size pInputLength, OutputStream *pOutput);  // 512 bit(64 bytes) result
        static uint64_t sipHash24(uint8_t *pData, stream_size pLength, uint64_t pKey0, uint64_t pKey1);

        // Virtual overloaded functions
        stream_size writeOffset() const { return mByteCount; }
        void write(const void *pInput, stream_size pSize);

        static bool test();

    protected:
        unsigned int mBlockSize;

    private:
        Type mType;
        stream_size mByteCount;
        Buffer mInput;
        uint32_t *mBlockData, *mResultData;

        // Process as many blocks of data as possible
        void process();

    };

    class HMACDigest : public Digest
    {
    public:

        HMACDigest(Type pType) : Digest(pType) {} // Must still be initialized with a key
        HMACDigest(Type pType, InputStream *pKey);

        void initialize(InputStream *pKey);
        void getResult(RawOutputStream *pOutput);

    private:
        Buffer mOuterPaddedKey;
    };
}

#endif
