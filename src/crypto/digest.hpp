#ifndef ARCMIST_DIGEST_HPP
#define ARCMIST_DIGEST_HPP

#include "arcmist/base/string.hpp"
#include "arcmist/io/stream.hpp"
#include "arcmist/io/buffer.hpp"

#include <cstdint>


namespace ArcMist
{
    namespace Digest
    {
        void crc32(InputStream *pInput, unsigned int pInputLength, OutputStream *pOutput); // 32 bit(4 byte) result
        uint32_t crc32(const char *pText);
        uint32_t crc32(const uint8_t *pData, unsigned int pSize);
        void md5(InputStream *pInput, unsigned int pInputLength, OutputStream *pOutput);  // 128 bit(16 byte) result
        void sha1(InputStream *pInput, unsigned int pInputLength, OutputStream *pOutput);  // 160 bit(20 byte) result
        void ripEMD160(InputStream *pInput, unsigned int pInputLength, OutputStream *pOutput);  // 160 bit(20 bytes) result
        void sha256(InputStream *pInput, unsigned int pInputLength, OutputStream *pOutput);  // 256 bit(32 bytes) result
        void sha512(InputStream *pInput, unsigned int pInputLength, OutputStream *pOutput);  // 512 bit(64 bytes) result

        bool test();
    }
}

#endif
