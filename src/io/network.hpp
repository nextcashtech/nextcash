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
                for(unsigned int i = 0; i < size(); ++i)
                    delete[] (*this)[i];
            }

            typedef std::vector<char *>::iterator iterator;
            typedef std::vector<char *>::const_iterator const_iterator;
        };

        // Lookup DNS and return IP addresses
        bool list(const char *pName, IPList &pList);

        // Parse IPv4/IPv6 text string into an IPv6 byte address
        uint8_t *parseIP(const char *pValue);

        bool isIPv4MappedIPv6(const uint8_t *pIP);

        class IPAddress
        {
        public:

            enum Type { IPV4, IPV6 };

            IPAddress();
            IPAddress(const IPAddress &pCopy);
            IPAddress(Type pType, const uint8_t *pIP, uint16_t pPort);

            const IPAddress &operator = (const IPAddress &pRight);

            Type type() const { return mType; }
            const uint8_t *ipv4Bytes() const { return mIP + INET6_ADDRLEN - INET_ADDRLEN; }
            const uint8_t *ipv6Bytes() const { return mIP; }
            uint16_t port() const { return mPort; }

            void clear();

            bool matches(const IPAddress &pOther) const;
            bool operator == (const IPAddress &pRight) const;
            bool operator != (const IPAddress &pRight) const;

            int compare(const IPAddress &pRight) const;

            bool isValid() const;

            void set(Type pType, const uint8_t *pIP, uint16_t pPort);

            bool setText(const char *pText);
            void setPort(uint16_t pPort) { mPort = pPort; }

            String text() const;

            void write(OutputStream *pStream) const;
            bool read(InputStream *pStream);

        private:

            Type mType;
            uint8_t mIP[INET6_ADDRLEN];
            uint16_t mPort;
        };

        class Connection
        {
        public:

            Connection() { mSocketID = -1; mBytesReceived = 0; mBytesSent = 0; }
            Connection(IPAddress &pIP, unsigned int pTimeout = 10);
            Connection(IPAddress::Type pType, const uint8_t *pIP, uint16_t pPort, unsigned int pTimeout = 10);
            Connection(int pSocketID, struct sockaddr *pAddress);
            ~Connection();

            int socket() { return mSocketID; }

            bool open(const IPAddress &pIP, unsigned int pTimeout = 10);
            bool open(IPAddress::Type pType, const uint8_t *pIP, uint16_t pPort, unsigned int pTimeout = 10);

            bool isOpen() { return mSocketID != -1; }

            bool set(int pSocketID, struct sockaddr *pAddress);

            const IPAddress &ip() const { return mIP; }

            // Returns number of bytes received
            unsigned int receive(OutputStream *pStream, bool pWait = false);
            bool send(InputStream *pStream);

            // Tracking of bytes sent and received
            uint64_t bytesReceived() const { return mBytesReceived; }
            uint64_t bytesSent() const { return mBytesSent; }
            void resetByteCounts() { mBytesReceived = 0; mBytesSent = 0; }

            void close();

        protected:

            bool open(unsigned int pTimeout = 10);

            void setTimeout(unsigned int pSeconds);

            int mSocketID;
            IPAddress mIP;
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
}

#endif
