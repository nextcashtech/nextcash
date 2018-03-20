/**************************************************************************
 * Copyright 2017 NextCash, LLC                                            *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@nextcash.com>                                    *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#include "stream.hpp"

#include "nextcash/base/endian.hpp"
#include "nextcash/base/math.hpp"
#include "nextcash/base/log.hpp"

#include <cstdio>
#include <vector>

#define STREAM_CHUNK_SIZE 16384
#define STREAM_LOG_NAME "Stream"


namespace NextCash
{
    Endian::Type InputStream::sDefaultInputEndian = Endian::sSystemType;
    Endian::Type OutputStream::sDefaultOutputEndian = Endian::sSystemType;

    uint8_t InputStream::readByte()
    {
        uint8_t result;
        read(&result, 1);
        return result;
    }

    uint16_t InputStream::readUnsignedShort()
    {
        uint16_t result;
        readEndian(&result, 2);
        return result;
    }

    uint32_t InputStream::readUnsignedInt()
    {
        uint32_t result;
        readEndian(&result, 4);
        return result;
    }

    uint64_t InputStream::readUnsignedLong()
    {
        uint64_t result;
        readEndian(&result, 8);
        return result;
    }

    int16_t InputStream::readShort()
    {
        int16_t result;
        readEndian(&result, 2);
        return result;
    }

    int32_t InputStream::readInt()
    {
        int32_t result;
        readEndian(&result, 4);
        return result;
    }

    int64_t InputStream::readLong()
    {
        int64_t result;
        readEndian(&result, 8);
        return result;
    }

    stream_size InputStream::readStream(OutputStream *pOutput, stream_size pMaxSize)
    {
        stream_size size = pMaxSize;

        if(size > remaining())
            size = remaining();

        uint8_t chunk[STREAM_CHUNK_SIZE];
        stream_size remainingBytes = size, chunkSize;

        while(remainingBytes)
        {
            if(remainingBytes > STREAM_CHUNK_SIZE)
                chunkSize = STREAM_CHUNK_SIZE;
            else
                chunkSize = remainingBytes;

            read(chunk, chunkSize);
            pOutput->write(chunk, chunkSize);

            remainingBytes -= chunkSize;
        }

        return size;
    }

    String InputStream::readString(stream_size pLength)
    {
        String result;
        if(pLength == 0)
            return result;
        char *data = result.writeAddress(pLength);
        read(data, pLength);
        return result;
    }

    String InputStream::readHexString(stream_size pSize)
    {
        String result;
        char *text = result.writeAddress(pSize * 2);

        for(stream_size i=0,offset=0;i<pSize;i++,offset+=2)
            std::memcpy(text + offset, Math::byteToHex[readByte()], 2);

        return result;
    }

    void InputStream::readAsHex(char *pOutput, stream_size pSize)
    {
        uint8_t byte;
        stream_size offset = 0;
        for(stream_size i=0;i<pSize;i++)
        {
            byte = readByte();
            pOutput[offset++] = Math::nibbleToHex((byte >> 4) & 0x0f);
            pOutput[offset++] = Math::nibbleToHex(byte & 0x0f);
        }
    }

    String InputStream::readBase58String(stream_size pSize)
    {
        uint8_t data[pSize];
        read(data, pSize);
        String result;
        result.writeBase58(data, pSize);
        return result;
    }

    String InputStream::readBase32String(stream_size pSize)
    {
        uint8_t data[pSize];
        read(data, pSize);
        String result;
        result.writeBase32(data, pSize);
        return result;
    }

    //void InputStream::readHexAsBinary(void *pOutput, stream_size pSize)
    //{
        // TODO
    //}

    stream_size OutputStream::writeByte(uint8_t pValue)
    {
        write(&pValue, 1);
        return 1;
    }

    stream_size OutputStream::writeUnsignedShort(uint16_t pValue)
    {
        writeEndian(&pValue, 2);
        return 2;
    }

    stream_size OutputStream::writeUnsignedInt(uint32_t pValue)
    {
        writeEndian(&pValue, 4);
        return 4;
    }

    stream_size OutputStream::writeUnsignedLong(uint64_t pValue)
    {
        writeEndian(&pValue, 8);
        return 8;
    }

    stream_size OutputStream::writeShort(int16_t pValue)
    {
        writeEndian(&pValue, 2);
        return 2;
    }

    stream_size OutputStream::writeInt(int32_t pValue)
    {
        writeEndian(&pValue, 4);
        return 1;
    }

    stream_size OutputStream::writeLong(int64_t pValue)
    {
        writeEndian(&pValue, 8);
        return 1;
    }

    stream_size RawOutputStream::writeStream(InputStream *pInput, stream_size pMaxSize)
    {
        if(pMaxSize == 0)
            return 0;

        stream_size size = pMaxSize;
        if(size > pInput->remaining())
            size = pInput->remaining();

        uint8_t chunk[STREAM_CHUNK_SIZE];
        stream_size remainingBytes = size, chunkSize;

        while(remainingBytes)
        {
            if(remainingBytes > STREAM_CHUNK_SIZE)
                chunkSize = STREAM_CHUNK_SIZE;
            else
                chunkSize = remainingBytes;

            pInput->read(chunk, chunkSize);
            write(chunk, chunkSize);

            remainingBytes -= chunkSize;
        }

        return size;
    }

    stream_size OutputStream::writeString(const char *pString, bool pWriteNull)
    {
        stream_size length = 0;
        const char *ptr = pString;
        while(*ptr)
        {
            length++;
            ptr++;
        }
        if(pWriteNull)
            length++;
        write((const uint8_t *)pString, length);
        return length;
    }

    stream_size OutputStream::writeAsHex(InputStream *pInput, stream_size pMaxSize, bool pWriteNull)
    {
        stream_size readCount = 0, writtenCount = 0;
        while(pInput->remaining() && readCount < pMaxSize)
        {
            readCount++;
            writtenCount += writeString(Math::byteToHex[pInput->readByte()]);
        }

        if(pWriteNull)
            writtenCount += writeByte(0);
        return writtenCount;
    }

    stream_size OutputStream::writeHex(const char *pString)
    {
        bool firstNibble = true;
        uint8_t byte;
        const char *ptr = pString;
        stream_size writtenCount = 0;
        while(*ptr)
        {
            if(firstNibble)
                byte = Math::hexToNibble(*ptr) << 4;
            else
            {
                byte += Math::hexToNibble(*ptr);
                writtenCount += writeByte(byte);
            }
            firstNibble = !firstNibble;
            ptr++;
        }

        return writtenCount;
    }

    //stream_size OutputStream::writeAsBase58(InputStream *pInput, stream_size pMaxSize, bool pWriteNull)
    //{
        //TODO
    //    return 0;
    //}

    stream_size OutputStream::writeBase58AsBinary(const char *pString)
    {
        // Skip leading white space.
        while(*pString && isWhiteSpace(*pString))
            ++pString;

        // Skip and count leading '1's.
        stream_size zeroes = 0;
        stream_size length = 0;
        while(*pString == '1')
        {
            ++zeroes;
            ++pString;
        }

        // Allocate enough space in big-endian base256 representation.
        stream_size i, carry, size = std::strlen(pString) * 733 /1000 + 1; // log(58) / log(256), rounded up.
        const char *match;
        std::vector<uint8_t> b256(size);
        // Process the characters.
        while(*pString && !isWhiteSpace(*pString))
        {
            // Decode base58 character
            match = std::strchr(Math::base58Codes, *pString);
            if(match == NULL)
                return 0;

            // Apply "b256 = b256 * 58 + match".
            i = 0;
            carry = match - Math::base58Codes;

            for(std::vector<uint8_t>::reverse_iterator it = b256.rbegin(); (carry != 0 || i < length) && (it != b256.rend()); ++it, ++i)
            {
                carry += 58 * (*it);
                *it = carry % 256;
                carry /= 256;
            }

            length = i;
            ++pString;
        }

        // Skip trailing spaces.
        while(isWhiteSpace(*pString))
            ++pString;

        if(*pString)
            return 0;

        // Skip leading zeroes in b256.
        std::vector<uint8_t>::iterator it = b256.begin() + (size - length);
        while(it != b256.end() && *it == 0)
            ++it;

        // Copy result into output vector.
        stream_size bytesWritten = 0;
        for(i=0;i<zeroes;i++)
            bytesWritten += writeByte(0);
        while (it != b256.end())
            bytesWritten += writeByte(*(it++));

        return bytesWritten;
    }

    stream_size OutputStream::writeBase32AsBinary(const char *pString)
    {
        // Skip leading white space.
        while(*pString && isWhiteSpace(*pString))
            ++pString;

        // Decode base32 characters into bits
        std::vector<bool> bits;
        uint8_t byteValue;
        const char *match;
        int bitOffset;
        while(*pString)
        {
            // Decode base32 character
            match = std::strchr(Math::base32Codes, *pString);
            if(match == NULL)
                return 0;
            byteValue = match - Math::base32Codes;

            // Put bits
            for(bitOffset=4;bitOffset>=0;--bitOffset)
                bits.push_back((byteValue >> bitOffset) & 0x01);

            ++pString;
        }

        // Write bits into stream
        stream_size bytesWritten = 0;
        byteValue = 0;
        bitOffset = 0;
        for(std::vector<bool>::iterator bit=bits.begin();bit!=bits.end();++bit)
        {
            byteValue <<= 1;
            if(*bit)
                byteValue |= 0x01;
            ++bitOffset;

            if(bitOffset == 8)
            {
                writeByte(byteValue);
                ++bytesWritten;
                byteValue = 0;
                bitOffset = 0;
            }
        }

        if(bitOffset > 0)
        {
            writeByte(byteValue << (8 - bitOffset));
            ++bytesWritten;
        }

        return bytesWritten;
    }

    stream_size OutputStream::writeFormatted(const char *pFormatting, ...)
    {
        va_list args;
        va_start(args, pFormatting);
        stream_size result = writeFormattedList(pFormatting, args);
        va_end(args);
        return result;
    }

    stream_size OutputStream::writeFormattedList(const char *pFormatting, va_list &pList)
    {
        va_list args;
        va_copy(args, pList);
        int size = vsnprintf(NULL, 0, pFormatting, args);
        va_end(args);

        if(size < 0)
        {
            Log::error(STREAM_LOG_NAME, "Text formatting failed");
            return 0;
        }

        char *entry = new char[size + 1];
        int actualSize = vsnprintf(entry, size + 1, pFormatting, pList);

        if(actualSize < 0)
        {
            Log::error(STREAM_LOG_NAME, "Text formatting failed");
            delete[] entry;
            return 0;
        }
        else
        {
            write((const uint8_t *)entry, actualSize);
            delete[] entry;
            return actualSize;
        }
    }
}
