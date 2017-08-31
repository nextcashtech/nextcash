#ifndef ARCMIST_STREAM_HPP
#define ARCMIST_STREAM_HPP

#include "arcmist/base/endian.hpp"
#include "arcmist/base/string.hpp"

#include <cstdarg>
#include <cstdint>


namespace ArcMist
{
    class OutputStream;
    class InputStream;

    // Basic abstract for a class that can be written to
    class RawOutputStream
    {
    public:
        virtual ~RawOutputStream() {}
        virtual void write(const void *pInput, unsigned int pSize) = 0;

        unsigned int writeStream(InputStream *pInput, unsigned int pMaxSize);
    };

    // Basic abstract for a class that can be read from
    class RawInputStream
    {
    public:
        virtual ~RawInputStream() {}
        virtual void read(void *pOutput, unsigned int pSize) = 0;
    };

    // Abstract for a class that can have different primitive types read from it
    class InputStream : public RawInputStream
    {
    public:

        InputStream() { mInputEndian = sDefaultInputEndian; }

        uint8_t readByte();
        uint16_t readUnsignedShort();
        uint32_t readUnsignedInt();
        uint64_t readUnsignedLong();
        int16_t readShort();
        int32_t readInt();
        int64_t readLong();
        unsigned int readStream(OutputStream *pOutput, unsigned int pMaxSize);
        String readString(unsigned int pLength);
        // Create hex string from binary data
        String readHexString(unsigned int pSize);

        // Read binary data into hex text. Note: pOutput has to contain twice as many bytes as pByteCount
        void readAsHex(char *pOutput, unsigned int pSize);
        
        // Create base 58 string from binary data
        String readBase58String(unsigned int pSize);

        // Read hex text into binary output
        //void readHexAsBinary(void *pOutput, unsigned int pSize);

        // Offset into data where next byte will be read
        virtual unsigned int readOffset() const = 0;

        // Number of bytes in data
        virtual unsigned int length() const = 0;

        // Number of bytes remaining to be read
        virtual unsigned int remaining() const { return length() - readOffset(); }

        virtual operator bool() const { return true; }
        virtual bool operator !() const { return false; }

        // Endian defaulted on new InputStreams
        static Endian::Type sDefaultInputEndian;
        static void setDefaultInputEndian(Endian::Type pEndian) { sDefaultInputEndian = pEndian; }

        // Endian for this InputStream
        Endian::Type inputEndian() { return mInputEndian; }
        void setInputEndian(Endian::Type pEndian) { mInputEndian = pEndian; }
        void readEndian(void *pOutput, unsigned int pSize)
        {
            if(Endian::sSystemType != mInputEndian)
            {
                // Read in reverse
                //uint8_t *ptr = ((uint8_t *)pOutput) + pSize - 1;
                //for(unsigned int i=0;i<pSize;i++)
                //    read(ptr--, 1);
                uint8_t data[pSize];
                read(data, pSize);
                for(int i=pSize-1,j=0;i>=0;i--,j++)
                    ((uint8_t *)pOutput)[i] = data[j];
            }
            else
                read(pOutput, pSize);
        }

    private:

        Endian::Type mInputEndian;

    };

    // Abstract for a class that can have different primitive types written to it
    class OutputStream : public RawOutputStream
    {
    public:

        OutputStream() { mOutputEndian = sDefaultOutputEndian; }

        unsigned int writeByte(uint8_t pValue);
        unsigned int writeUnsignedShort(uint16_t pValue);
        unsigned int writeUnsignedInt(uint32_t pValue);
        unsigned int writeUnsignedLong(uint64_t pValue);
        unsigned int writeShort(int16_t pValue);
        unsigned int writeInt(int32_t pValue);
        unsigned int writeLong(int64_t pValue);

        // Write character string until null character
        unsigned int writeString(const char *pString, bool pWriteNull = false);

        // Write binary from input as hex text
        unsigned int writeAsHex(InputStream *pInput, unsigned int pMaxSize, bool pWriteNull = false);

        // Write hex text as binary
        unsigned int writeHex(const char *pString);

        // Write binary from input as base58 text
        //unsigned int writeAsBase58(InputStream *pInput, unsigned int pMaxSize, bool pWriteNull = false);

        // Write base58 text as binary
        unsigned int writeBase58AsBinary(const char *pString);

        // Use printf string formatting to write
        unsigned int writeFormatted(const char *pFormatting, ...);
        unsigned int writeFormattedList(const char *pFormatting, va_list &pList);

        virtual unsigned int writeOffset() const = 0;
        virtual void flush() {}

        // Endian defaulted on new OutputStreams
        static Endian::Type sDefaultOutputEndian;
        static void setDefaultOutputEndian(Endian::Type pEndian) { sDefaultOutputEndian = pEndian; }

        // Endian for this OutputStream
        Endian::Type outputEndian() { return mOutputEndian; }
        void setOutputEndian(Endian::Type pEndian) { mOutputEndian = pEndian; }
        void writeEndian(const void *pInput, unsigned int pSize)
        {
            if(Endian::sSystemType != mOutputEndian)
            {
                // Write in reverse
                //uint8_t *ptr = ((uint8_t *)pInput) + pSize - 1;
                //for(unsigned int i=0;i<pSize;i++)
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