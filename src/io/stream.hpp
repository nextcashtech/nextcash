/**************************************************************************
 * Copyright 2017 NextCash, LLC                                           *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@nextcash.tech>                                  *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#ifndef NEXTCASH_STREAM_HPP
#define NEXTCASH_STREAM_HPP

#include "endian.hpp"
#include "string.hpp"

#include <cstdarg>
#include <cstdint>
#include <limits>


namespace NextCash
{
    class OutputStream;
    class InputStream;

    typedef size_t stream_size;
    static const stream_size INVALID_STREAM_SIZE = std::numeric_limits<size_t>::max(); // 0xffffffffffffffff;

    // Basic abstract for a class that can be written to
    class RawOutputStream
    {
    public:
        virtual ~RawOutputStream() {}
        virtual void write(const void *pInput, stream_size pSize) = 0;

        stream_size writeStream(InputStream *pInput, stream_size pMaxSize);
    };

    // Basic abstract for a class that can be read from
    class RawInputStream
    {
    public:
        virtual ~RawInputStream() {}
        virtual bool read(void *pOutput, stream_size pSize) = 0;
    };

    // Abstract for a class that can have different primitive types read from it
    class InputStream : public RawInputStream
    {
    public:

        InputStream() { mInputEndian = sDefaultInputEndian; }

        uint8_t readByte();
        uint16_t readUnsignedShort();
        uint32_t readUnsignedInt();
        uint64_t readUnsignedInt6(); // Read 6 byte unsigned integer
        uint64_t readUnsignedLong();
        int16_t readShort();
        int32_t readInt();
        uint64_t readInt6(); // Read 6 byte integer
        int64_t readLong();
        stream_size readStream(OutputStream *pOutput, stream_size pMaxSize);
        String readString(stream_size pLength);
        String readString();
        // Create hex string from binary data
        String readHexString(stream_size pSize);

        // Read binary data into hex text. Note: pOutput has to contain twice as many bytes as pByteCount
        void readAsHex(char *pOutput, stream_size pSize);

        // Create base 58 string from binary data
        String readBase58String(stream_size pSize);

        // Create base 32 string from binary data
        String readBase32String(stream_size pSize);

        // Read hex text into binary output
        //void readHexAsBinary(void *pOutput, stream_size pSize);

        // Offset into data where next byte will be read
        virtual stream_size readOffset() const = 0;
        virtual bool setReadOffset(stream_size pOffset) { return false; }

        // Number of bytes in data
        virtual stream_size length() const = 0;

        // Number of bytes remaining to be read
        virtual stream_size remaining() const { return length() - readOffset(); }

        virtual operator bool() const { return true; }
        virtual bool operator !() const { return false; }

        // Endian defaulted on new InputStreams
        static Endian::Type sDefaultInputEndian;
        static void setDefaultInputEndian(Endian::Type pEndian) { sDefaultInputEndian = pEndian; }

        // Endian for this InputStream
        Endian::Type inputEndian() { return mInputEndian; }
        void setInputEndian(Endian::Type pEndian) { mInputEndian = pEndian; }
        bool readEndian(void *pOutput, stream_size pSize)
        {
            if(Endian::sSystemType != mInputEndian)
            {
                // Read in reverse
                //uint8_t *ptr = ((uint8_t *)pOutput) + pSize - 1;
                //for(stream_size i=0;i<pSize;i++)
                //    read(ptr--, 1);
                uint8_t data[pSize];
                if(read(data, pSize))
                {
                    for(int i = pSize - 1, j = 0; i >= 0; --i, ++j)
                        ((uint8_t *)pOutput)[i] = data[j];
                    return true;
                }
                else
                    return false;
            }
            else
                return read(pOutput, pSize);
        }

    private:

        Endian::Type mInputEndian;

    };

    // Abstract for a class that can have different primitive types written to it
    class OutputStream : public RawOutputStream
    {
    public:

        OutputStream() { mOutputEndian = sDefaultOutputEndian; }

        stream_size writeByte(uint8_t pValue);
        stream_size writeUnsignedShort(uint16_t pValue);
        stream_size writeUnsignedInt(uint32_t pValue);
        stream_size writeUnsignedInt6(uint64_t pValue); // write 6 byte unsigned integer
        stream_size writeUnsignedLong(uint64_t pValue);
        stream_size writeShort(int16_t pValue);
        stream_size writeInt(int32_t pValue);
        stream_size writeLong(int64_t pValue);

        // Write character string until null character
        stream_size writeString(const char *pString, bool pWriteNull = false);

        // Write binary from input as hex text
        stream_size writeAsHex(InputStream *pInput, stream_size pMaxSize, bool pWriteNull = false);

        // Write hex text as binary
        stream_size writeHex(const char *pString);

        // Write binary from input as base58 text
        //stream_size writeAsBase58(InputStream *pInput, stream_size pMaxSize, bool pWriteNull = false);

        // Write base58 text as binary
        stream_size writeBase58AsBinary(const char *pString);

        // Write base32 text as binary
        stream_size writeBase32AsBinary(const char *pString);

        // Use printf string formatting to write
        stream_size writeFormatted(const char *pFormatting, ...);
        stream_size writeFormattedList(const char *pFormatting, va_list &pList);

        virtual stream_size length() const { return 0; }
        virtual stream_size writeOffset() const = 0;
        virtual bool setWriteOffset(stream_size pOffset) { return false; }
        virtual void flush() {}

        // Endian defaulted on new OutputStreams
        static Endian::Type sDefaultOutputEndian;
        static void setDefaultOutputEndian(Endian::Type pEndian) { sDefaultOutputEndian = pEndian; }

        // Endian for this OutputStream
        Endian::Type outputEndian() { return mOutputEndian; }
        void setOutputEndian(Endian::Type pEndian) { mOutputEndian = pEndian; }
        void writeEndian(const void *pInput, stream_size pSize)
        {
            if(Endian::sSystemType != mOutputEndian)
            {
                // Write in reverse
                //uint8_t *ptr = ((uint8_t *)pInput) + pSize - 1;
                //for(stream_size i=0;i<pSize;i++)
                //    write(ptr--, 1);
                uint8_t data[pSize];
                for(int i=pSize-1, j=0;i>=0;i--, j++)
                    data[j] = ((uint8_t *)pInput)[i];
                write(data, pSize);
            }
            else
                write(pInput, pSize);
        }

    private:

        Endian::Type mOutputEndian;

    };

    class Stream : public InputStream, public OutputStream
    {
    public:

        void setEndian(Endian::Type pEndian)
        {
            setInputEndian(pEndian);
            setOutputEndian(pEndian);
        }

    };
}

#endif
