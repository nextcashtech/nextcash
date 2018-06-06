/**************************************************************************
 * Copyright 2018 NextCash, LLC                                           *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@nextcash.tech>                                  *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#ifndef NEXTCASH_ENCRYPT_HPP
#define NEXTCASH_ENCRYPT_HPP

#include "string.hpp"
#include "stream.hpp"
#include "buffer.hpp"

#include <cstdint>
#include <vector>


namespace NextCash
{
    namespace Encryption
    {
        enum Type { AES_128,   // 128 bit, 16 byte
                    AES_192,   // 192 bit, 24 byte
                    AES_256 }; // 256 bit, 32 byte

        enum BlockMethod { NONE,
                           ECB,    // Electronic Codebook
                           CBC };  // Cipher Block Chaining

        bool test();
    }

    class AES;

    class Encryptor : public OutputStream
    {
    public:

        // pOutput is where the encrypted data should be written.
        Encryptor(OutputStream *pOutput, Encryption::Type pType = Encryption::AES_256,
          Encryption::BlockMethod pBlockMethod = Encryption::CBC);
        ~Encryptor();

        void setup(const uint8_t *pKey, int pKeyLength,
          const uint8_t *pInitializationVector, int pInitializationVectorLength);

        void finalize(); // Write any remaining data to output

        // Virtual overloaded functions
        stream_size writeOffset() const { return mByteCount; }
        void write(const void *pInput, stream_size pSize);

    private:

        Encryption::Type mType;
        Encryption::BlockMethod mBlockMethod;
        std::vector<uint8_t> mVector;
        stream_size mByteCount;
        unsigned int mBlockSize;
        uint8_t *mBlock;
        Buffer mData;
        AES *mAES;
        OutputStream *mOutput;

        void process();

    };

    class Decryptor : public InputStream
    {
    public:

        // pInput is where the encrypted data will be read from.
        Decryptor(InputStream *pInput, Encryption::Type pType = Encryption::AES_256,
          Encryption::BlockMethod pBlockMethod = Encryption::CBC);
        ~Decryptor();

        void setup(const uint8_t *pKey, int pKeyLength,
          const uint8_t *pInitializationVector, int pInitializationVectorLength);

        // Virtual overloaded functions
        stream_size readOffset() const { return mInput->readOffset(); }
        stream_size length() const { return mInput->length(); }
        stream_size remaining() const { return mInput->remaining(); }
        void read(void *pOutput, stream_size pSize);

    private:

        Encryption::Type mType;
        Encryption::BlockMethod mBlockMethod;
        std::vector<uint8_t> mVector;
        unsigned int mBlockSize;
        uint8_t *mBlock, *mEncryptedBlock;
        Buffer mData;
        AES *mAES;
        InputStream *mInput;

        void process();

    };
}

#endif
