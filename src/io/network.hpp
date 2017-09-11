#ifndef ARCMIST_NETWORK_HPP
#define ARCMIST_NETWORK_HPP

#include "stream.hpp"

#include <vector>
#include <netinet/in.h>

#define NETWORK_BUFFER_SIZE 4096
#define INET_ADDRLEN 4
#define INET6_ADDRLEN 16


namespace ArcMist
{
    namespace Network
    {
        class IPList : public std::vector<char *>
        {
        public:

            IPList() {}
            ~IPList()
            {
                for(unsigned int i=0;i<size();i++)
                    delete[] (*this)[i];
            }
        };

        // Lookup DNS and return IP addresses
        bool list(const char *pName, IPList &pList);

        // Parse IPv6 text string into a byte address
        uint8_t *parseIPv6(const char *pValue);

        class Connection
        {
        public:

            Connection() { mSocketID = -1; mBytesReceived = 0; mBytesSent = 0; }
            Connection(const char *pIP, const char *pPort, unsigned int pTimeout = 10);
            Connection(unsigned int pFamily, const uint8_t *pIP, uint16_t pPort, unsigned int pTimeout = 10);
            Connection(int pSocketID, struct sockaddr *pAddress);
            ~Connection();

            bool open(const char *pIP, const char *pPort, unsigned int pTimeout = 10);
            bool open(unsigned int pFamily, const uint8_t *pIP, uint16_t pPort, unsigned int pTimeout = 10);
            bool isOpen() { return mSocketID != -1; }

            bool set(int pSocketID, struct sockaddr *pAddress);

            static bool isIPv4MappedIPv6(const uint8_t *pIP);

            const char *ipAddress() { return mIPv4Address; }
            const uint8_t *ipBytes() { return mIPv4; }
            uint16_t port() { return mPort; }

            const char *ipv6Address() { return mIPv6Address; }
            const uint8_t *ipv6Bytes() { return mIPv6; }

            // Returns number of bytes received
            unsigned int receive(OutputStream *pStream, bool pWait = false);
            bool send(InputStream *pStream);

            // Tracking of bytes sent and recieved
            uint64_t bytesReceived() const { return mBytesReceived; }
            uint64_t bytesSent() const { return mBytesSent; }
            void resetByteCounts() { mBytesReceived = 0; mBytesSent = 0; }

            void close();

        protected:

            void setTimeout(unsigned int pSeconds);

            int mSocketID;
            int mType;
            char mIPv4Address[INET_ADDRSTRLEN];
            char mIPv6Address[INET6_ADDRSTRLEN];
            uint8_t mIPv4[INET_ADDRLEN];
            uint8_t mIPv6[INET6_ADDRLEN];
            uint16_t mPort;
            unsigned char mBuffer[NETWORK_BUFFER_SIZE];

            uint64_t mBytesReceived;
            uint64_t mBytesSent;
        };

        class Listener
        {
        public:

            Listener(sa_family_t pType, uint16_t pPort, unsigned int pListenBackLog = 5, unsigned int pTimeoutSeconds = 5);
            ~Listener();

            bool isValid() const { return mSocketID >= 0; }
            uint16_t port() const { return mPort; }

            bool accept(Connection &pConnection);

            void close();

        private:

            void setTimeout(unsigned int pSeconds);

            int mSocketID;
            uint16_t mPort;
            fd_set mSet;
            unsigned int mTimeoutSeconds;
            struct timeval mTimeout;
        };
    }
}

#endif
