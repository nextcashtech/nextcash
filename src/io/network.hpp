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

        bool list(const char *pName, IPList &pList);
        uint8_t *parseIPv6(const char *pValue);
    }

    class Connection
    {
    public:

        Connection() { mSocketID = -1; }
        Connection(const char *pIP, const char *pPort);
        ~Connection();

        bool open(const char *pIP, const char *pPort);
        bool open(unsigned int pFamily, const uint8_t *pIP, uint16_t pPort);
        bool isOpen() { return mSocketID != -1; }

        static bool isIPv4MappedIPv6(const uint8_t *pIP);

        const char *ipAddress() { return mIPv4Address; }
        const uint8_t *ipBytes() { return mIPv4; }
        uint16_t port() { return mPort; }

        const char *ipv6Address() { return mIPv6Address; }
        const uint8_t *ipv6Bytes() { return mIPv6; }

        // Returns number of bytes received
        unsigned int receive(OutputStream *pStream, bool pWait = false);
        bool send(InputStream *pStream);

        void close();

    protected:

        void setTimeout(unsigned int pSeconds);

        int mSocketID;
        int type;
        char mIPv4Address[INET_ADDRSTRLEN];
        char mIPv6Address[INET6_ADDRSTRLEN];
        uint8_t mIPv4[INET_ADDRLEN];
        uint8_t mIPv6[INET6_ADDRLEN];
        uint16_t mPort;
        unsigned char mBuffer[NETWORK_BUFFER_SIZE];
    };
}

#endif
