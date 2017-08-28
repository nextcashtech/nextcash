#include "stream.hpp"

#include "arcmist/base/endian.hpp"
#include "arcmist/base/math.hpp"
#include "arcmist/base/log.hpp"

#include <cstdio>
#include <vector>

#define STREAM_CHUNK_SIZE 16384
#define STREAM_LOG_NAME "Stream"


namespace ArcMist
{
    Endian::Type InputStream::sDefaultInputEndian = Endian::systemType();
    Endian::Type OutputStream::sDefaultOutputEndian = Endian::systemType();

    uint8_t InputStream::readByte()
    {
        uint8_t result;
        read(&result, 1);
        return result;
    }

    uint16_t InputStream::readUnsignedShort()
    {
        uint16_t result;
        readEndian((uint8_t *)&result, 2);
        return result;
    }

    uint32_t InputStream::readUnsignedInt()
    {
        uint32_t result;
        readEndian((uint8_t *)&result, 4);
        return result;
    }

    uint64_t InputStream::readUnsignedLong()
    {
        uint64_t result;
        readEndian((uint8_t *)&result, 8);
        return result;
    }

    int16_t InputStream::readShort()
    {
        int16_t result;
        readEndian((uint8_t *)&result, 2);
        return result;
    }

    int32_t InputStream::readInt()
    {
        int32_t result;
        readEndian((uint8_t *)&result, 4);
        return result;
    }

    int64_t InputStream::readLong()
    {
        int64_t result;
        readEndian((uint8_t *)&result, 8);
        return result;
    }

    unsigned int InputStream::readStream(OutputStream *pOutput, unsigned int pMaxSize)
    {
        unsigned int size = pMaxSize;

        if(size > remaining())
            size = remaining();

        uint8_t chunk[STREAM_CHUNK_SIZE];
        unsigned int remainingBytes = size, chunkSize;

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

    String InputStream::readString(unsigned int pLength)
    {
        String result;
        if(pLength == 0)
            return result;
        char *data = result.writeAddress(pLength);
        read(data, pLength);
        return result;
    }

    String InputStream::readHexString(unsigned int pSize)
    {
        String result;
        char *text = result.writeAddress(pSize * 2);

        for(unsigned int i=0,offset=0;i<pSize;i++,offset+=2)
            std::memcpy(text + offset, Math::byteToHex[readByte()], 2);

        return result;
    }

    void InputStream::readAsHex(char *pOutput, unsigned int pSize)
    {
        uint8_t byte;
        unsigned int offset = 0;
        for(unsigned int i=0;i<pSize;i++)
        {
            byte = readByte();
            pOutput[offset++] = Math::nibbleToHex((byte >> 4) & 0x0f);
            pOutput[offset++] = Math::nibbleToHex(byte & 0x0f);
        }
    }

    String InputStream::readBase58String(unsigned int pSize)
    {
        uint8_t data[pSize];
        read(data, pSize);
        String result;
        result.writeBase58(data, pSize);
        return result;
    }

    //void InputStream::readHexAsBinary(void *pOutput, unsigned int pSize)
    //{
        // TODO
    //}

    unsigned int OutputStream::writeByte(uint8_t pValue)
    {
        write(&pValue, 1);
        return 1;
    }

    unsigned int OutputStream::writeUnsignedShort(uint16_t pValue)
    {
        writeEndian((uint8_t *)&pValue, 2);
        return 2;
    }

    unsigned int OutputStream::writeUnsignedInt(uint32_t pValue)
    {
        writeEndian((uint8_t *)&pValue, 4);
        return 4;
    }

    unsigned int OutputStream::writeUnsignedLong(uint64_t pValue)
    {
        writeEndian((uint8_t *)&pValue, 8);
        return 8;
    }

    unsigned int OutputStream::writeShort(int16_t pValue)
    {
        writeEndian((uint8_t *)&pValue, 2);
        return 2;
    }

    unsigned int OutputStream::writeInt(int32_t pValue)
    {
        writeEndian((uint8_t *)&pValue, 4);
        return 1;
    }

    unsigned int OutputStream::writeLong(int64_t pValue)
    {
        writeEndian((uint8_t *)&pValue, 8);
        return 1;
    }

    unsigned int OutputStream::writeStream(InputStream *pInput, unsigned int pMaxSize)
    {
        unsigned int size = pMaxSize;

        if(size > pInput->remaining())
            size = pInput->remaining();

        uint8_t chunk[STREAM_CHUNK_SIZE];
        unsigned int remainingBytes = size, chunkSize;

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

    unsigned int OutputStream::writeString(const char *pString, bool pWriteNull)
    {
        unsigned int length = 0;
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

    unsigned int OutputStream::writeAsHex(InputStream *pInput, unsigned int pMaxSize, bool pWriteNull)
    {
        unsigned int readCount = 0, writtenCount = 0;
        while(pInput->remaining() && readCount < pMaxSize)
        {
            readCount++;
            writtenCount += writeString(Math::byteToHex[pInput->readByte()]);
        }

        if(pWriteNull)
            writtenCount += writeByte(0);
        return writtenCount;
    }

    unsigned int OutputStream::writeHexAsBinary(const char *pString)
    {
        bool firstNibble = true;
        uint8_t byte;
        const char *ptr = pString;
        unsigned int writtenCount = 0;
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

    //unsigned int OutputStream::writeAsBase58(InputStream *pInput, unsigned int pMaxSize, bool pWriteNull)
    //{
        //TODO
    //    return 0;
    //}

    unsigned int OutputStream::writeBase58AsBinary(const char *pString)
    {
        // Skip leading white space.
        while(*pString && isWhiteSpace(*pString))
            pString++;

        // Skip and count leading '1's.
        unsigned int zeroes = 0;
        unsigned int length = 0;
        while(*pString == '1')
        {
            zeroes++;
            pString++;
        }

        // Allocate enough space in big-endian base256 representation.
        unsigned int i, carry, size = std::strlen(pString) * 733 /1000 + 1; // log(58) / log(256), rounded up.
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
            pString++;
        }

        // Skip trailing spaces.
        while(isWhiteSpace(*pString))
            pString++;

        if(*pString)
            return 0;

        // Skip leading zeroes in b256.
        std::vector<uint8_t>::iterator it = b256.begin() + (size - length);
        while(it != b256.end() && *it == 0)
            it++;

        // Copy result into output vector.
        unsigned int bytesWritten = 0;
        for(i=0;i<zeroes;i++)
            bytesWritten += writeByte(0);
        while (it != b256.end())
            bytesWritten += writeByte(*(it++));

        return bytesWritten;
    }

    unsigned int OutputStream::writeFormatted(const char *pFormatting, ...)
    {
        va_list args;
        va_start(args, pFormatting);
        unsigned int result = writeFormattedList(pFormatting, args);
        va_end(args);
        return result;
    }

    unsigned int OutputStream::writeFormattedList(const char *pFormatting, va_list &pList)
    {
        char *entry = new char[512];
        int actualSize = vsnprintf(entry, 512, pFormatting, pList);
        unsigned int result = 0;

        if(actualSize > 512)
        {
            delete[] entry;
            entry = new char[actualSize + 1];
            actualSize = vsnprintf(entry, actualSize + 1, pFormatting, pList);
        }

        if(actualSize < 0)
            Log::error(STREAM_LOG_NAME, "Text formatting failed");
        else
        {
            write((const uint8_t *)entry, actualSize);
            result = actualSize;
        }
        delete[] entry;
        return result;
    }
}
