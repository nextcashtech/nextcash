/**************************************************************************
 * Copyright 2017-2018 NextCash, LLC                                      *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@nextcash.tech>                                  *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#ifndef NEXTCASH_NETWORK_HPP
#define NEXTCASH_NETWORK_HPP

#include "stream.hpp"

#include <vector>
#include <netinet/in.h>

#define NETWORK_BUFFER_SIZE 4096
#define INET_ADDRLEN 4
#define INET6_ADDRLEN 16


namespace NextCash
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

            typedef std::vector<char *>::iterator iterator;
            typedef std::vector<char *>::const_iterator const_iterator;
        };

        // Lookup DNS and return IP addresses
        bool list(const char *pName, IPList &pList);

        // Parse IPv4/IPv6 text string into an IPv6 byte address
        uint8_t *parseIP(const char *pValue);

        class Connection
        {
        public:

            Connection() { mSocketID = -1; mBytesReceived = 0; mBytesSent = 0; }
            Connection(const char *pIP, const char *pPort, unsigned int pTimeout = 10);
            Connection(unsigned int pFamily, const uint8_t *pIP, uint16_t pPort, unsigned int pTimeout = 10);
            Connection(int pSocketID, struct sockaddr *pAddress);
            ~Connection();

            int socket() { return mSocketID; }

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

            // Tracking of bytes sent and received
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
            uint8_t mBuffer[NETWORK_BUFFER_SIZE];

            uint64_t mBytesReceived;
            uint64_t mBytesSent;
        };

        class Listener
        {
        public:

            Listener(sa_family_t pType, uint16_t pPort, unsigned int pListenBackLog = 5, unsigned int pTimeoutSeconds = 5);
            ~Listener() { close(); }

            bool isValid() const { return mSocketID >= 0; }
            uint16_t port() const { return mPort; }

            // Check for a new connection.
            // Returns NULL if there aren't any new connections
            Connection *accept();

            void close();

        private:

            void setTimeout(unsigned int pSeconds);

            bool processConnections();

            int mSocketID;
            uint16_t mPort;
            unsigned int mTimeoutSeconds;

            std::vector<Connection *> mPendingConnections;
        };
    }

    class IPAddress
    {
    public:

        IPAddress()
        {
            std::memset(ip, 0, INET6_ADDRLEN);
            port = 0;
        }
        IPAddress(const IPAddress &pCopy)
        {
            std::memcpy(ip, pCopy.ip, INET6_ADDRLEN);
            port = pCopy.port;
        }
        IPAddress(const uint8_t *pIP, uint16_t pPort)
        {
            std::memcpy(ip, pIP, INET6_ADDRLEN);
            port = pPort;
        }

        void write(OutputStream *pStream) const;
        bool read(InputStream *pStream);

        bool matches(const IPAddress &pOther) const
        {
            return std::memcmp(ip, pOther.ip, INET6_ADDRLEN) == 0 && port == pOther.port;
        }

        bool operator == (const IPAddress &pRight) const
        {
            return std::memcmp(ip, pRight.ip, INET6_ADDRLEN) == 0 && port == pRight.port;
        }

        bool operator != (const IPAddress &pRight) const
        {
            return std::memcmp(ip, pRight.ip, INET6_ADDRLEN) != 0 || port != pRight.port;
        }

        void operator = (Network::Connection &pConnection)
        {
            if(pConnection.ipv6Bytes())
                std::memcpy(ip, pConnection.ipv6Bytes(), INET6_ADDRLEN);
            port = pConnection.port();
        }

        int compare(const IPAddress &pRight) const
        {
            const uint8_t *left = ip;
            const uint8_t *right = pRight.ip;
            for(unsigned int i = 0; i < INET6_ADDRLEN; ++i, ++left, ++right)
            {
                if(*left < *right)
                    return -1;
                else if(*left > *right)
                    return 1;
            }

            if(port < pRight.port)
                return -1;
            else if(port > pRight.port)
                return 1;
            else
                return 0;
        }

        bool isValid() const
        {
            bool zeroes = true;
            for(int i=0;i<INET6_ADDRLEN;i++)
                if(ip[i] != 0)
                    zeroes = false;
            return !zeroes;
        }

        const IPAddress &operator = (const IPAddress &pRight)
        {
            port = pRight.port;
            std::memcpy(ip, pRight.ip, INET6_ADDRLEN);
            return *this;
        }

        void set(uint8_t *pIP, uint16_t pPort)
        {
            std::memcpy(ip, pIP, INET6_ADDRLEN);
            port = pPort;
        }

        bool setText(const char *pText);

        String text() const;

        uint8_t ip[INET6_ADDRLEN];
        uint16_t port;
    };
}

#endif
