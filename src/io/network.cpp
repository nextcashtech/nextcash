#include "network.hpp"

#include "arcmist/base/log.hpp"

#include <cstdlib>
#include <cstring>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define NETWORK_LOG_NAME "Network"


namespace ArcMist
{
    namespace Network
    {
        bool list(const char *pName, IPList &pList)
        {
            pList.clear();

            struct addrinfo hintAddress, *addressInfo, *testAddress;
            int errorCode;

            std::memset(&hintAddress, 0, sizeof hintAddress);
            hintAddress.ai_family = AF_UNSPEC;
            hintAddress.ai_socktype = SOCK_STREAM;

            Log::addFormatted(Log::VERBOSE, NETWORK_LOG_NAME, "Looking up %s", pName);

            if((errorCode = getaddrinfo(pName, 0, &hintAddress, &addressInfo)) != 0)
            {
                Log::addFormatted(Log::ERROR, NETWORK_LOG_NAME, "Get Address Info : %s", gai_strerror(errorCode));
                return false;
            }

            uint8_t address[INET6_ADDRLEN];
            char ip[INET6_ADDRSTRLEN];
            char *newIP;

            for(testAddress=addressInfo;testAddress!=NULL;testAddress=testAddress->ai_next)
            {
                if(testAddress->ai_addr->sa_family == AF_INET6)
                {
                    std::memcpy(address, (((struct sockaddr_in6*)testAddress->ai_addr)->sin6_addr.s6_addr), INET6_ADDRLEN);
                    inet_ntop(testAddress->ai_family, address, ip, INET6_ADDRSTRLEN);
                    Log::addFormatted(Log::VERBOSE, NETWORK_LOG_NAME, "Address found IPv6 %s", ip);
                }
                else if(testAddress->ai_addr->sa_family == AF_INET)
                {
                    std::memcpy(address, &(((struct sockaddr_in*)testAddress->ai_addr)->sin_addr.s_addr), INET_ADDRLEN);
                    inet_ntop(testAddress->ai_family, address, ip, INET_ADDRSTRLEN);
                    Log::addFormatted(Log::VERBOSE, NETWORK_LOG_NAME, "Address found IPv4 %s", ip);
                }
                else
                    continue;

                newIP = new char[std::strlen(ip)+1];
                std::strcpy(newIP, ip);
                pList.push_back(newIP);
            }

            return true;
        }

        uint8_t *parseIPv6(const char *pValue)
        {
            struct in6_addr address;

            if(inet_pton(AF_INET6, pValue, &address) != 1)
                return NULL;

            uint8_t *result = new uint8_t[INET6_ADDRLEN];
            std::memcpy(result, address.s6_addr, INET6_ADDRLEN);

            return result;
        }
    }

    Connection::Connection(const char *pIPAddress, const char *pPort)
    {
        mSocketID = -1;
        open(pIPAddress, pPort);
    }
    
    bool Connection::isIPv4MappedIPv6(const uint8_t *pIP)
    {
        // First 10 bytes are zeroes
        for(int i=0;i<10;i++)
            if(pIP[i] != 0)
                return false;;

        // Next 2 bytes are 0xFF
        if(pIP[10] != 0xff || pIP[11] != 0xff)
            return false;

        return true;
    }

    bool Connection::open(unsigned int pFamily, const uint8_t *pIP, uint16_t pPort)
    {
        close(); // Previous connection

        std::memset(mIPv4Address, 0, INET_ADDRSTRLEN);
        std::memset(mIPv6Address, 0, INET6_ADDRSTRLEN);

        if(pFamily == AF_INET || (pFamily == AF_INET6 && isIPv4MappedIPv6(pIP)))
        {
            mPort = pPort;
            if(pFamily == AF_INET6)
                std::memcpy(mIPv4, pIP + 12, INET_ADDRLEN);
            else
                std::memcpy(mIPv4, pIP, INET_ADDRLEN);
            inet_ntop(AF_INET, mIPv4, mIPv4Address, INET_ADDRSTRLEN);

            // Copy to IPv6 bytes as IPv4 mapped to IPv6
            std::memset(mIPv6 + INET6_ADDRLEN - INET_ADDRLEN - 2, 0xff, 2);
            std::memcpy(mIPv6 + INET6_ADDRLEN - INET_ADDRLEN, mIPv4, INET_ADDRLEN);

            Log::addFormatted(Log::INFO, NETWORK_LOG_NAME, "Attempting IPv4 connection to %s : %d", mIPv4Address, mPort);

            struct sockaddr_in address;

            address.sin_family = AF_INET;
            address.sin_port = mPort;
            std::memcpy(&address.sin_addr.s_addr, mIPv4, INET_ADDRLEN);

            if(Endian::sSystemType != Endian::BIG)
                Endian::reverse(&address.sin_port, 2);

            if((mSocketID = socket(AF_INET, SOCK_STREAM, 6)) == -1)
            {
                Log::addFormatted(Log::ERROR, NETWORK_LOG_NAME, "Socket failed : %s", std::strerror(errno));
                return false;
            }

            if(connect(mSocketID, (const sockaddr *)&address, sizeof(address)) == -1)
            {
                Log::addFormatted(Log::ERROR, NETWORK_LOG_NAME, "Connect failed : %s", std::strerror(errno));
                ::close(mSocketID);
                mSocketID = -1;
                return false;
            }

            setTimeout(10);

            Log::addFormatted(Log::INFO, NETWORK_LOG_NAME, "Connected to IPv4 %s : %d", mIPv4Address, mPort);
            return true;
        }
        else if(pFamily == AF_INET6)
        {
            mPort = pPort;
            std::memcpy(mIPv6, pIP, INET6_ADDRLEN);
            inet_ntop(AF_INET6, mIPv6, mIPv6Address, INET6_ADDRSTRLEN);
            Log::addFormatted(Log::INFO, NETWORK_LOG_NAME, "Attempting IPv6 connection to %s : %d", mIPv6Address, mPort);

            struct sockaddr_in6 address;

            address.sin6_family = AF_INET6;
            address.sin6_port = mPort;
            address.sin6_flowinfo = 0;
            std::memcpy(address.sin6_addr.s6_addr, mIPv6, INET6_ADDRLEN);
            address.sin6_scope_id = 0;

            if(Endian::sSystemType != Endian::BIG)
                Endian::reverse(&address.sin6_port, 2);

            if((mSocketID = socket(AF_INET6, SOCK_STREAM, 6)) == -1)
            {
                Log::addFormatted(Log::ERROR, NETWORK_LOG_NAME, "Socket failed : %s", std::strerror(errno));
                return false;
            }

            if(connect(mSocketID, (const sockaddr *)&address, sizeof(address)) == -1)
            {
                Log::addFormatted(Log::ERROR, NETWORK_LOG_NAME, "Connect failed : %s", std::strerror(errno));
                ::close(mSocketID);
                mSocketID = -1;
                return false;
            }

            setTimeout(10);

            Log::addFormatted(Log::INFO, NETWORK_LOG_NAME, "Connected to IPv6 %s : %d", mIPv6Address, mPort);
            return true;
        }
        else
        {
            Log::add(Log::ERROR, NETWORK_LOG_NAME, "Unknown network family");
            return false;
        }
    }

    bool Connection::open(const char *pIPAddress, const char *pPort)
    {
        close(); // Previous connection

        std::memset(mIPv4Address, 0, INET_ADDRSTRLEN);
        std::memset(mIPv6Address, 0, INET6_ADDRSTRLEN);

        struct addrinfo hintAddress, *addressInfo, *testAddress;
        int errorCode;

        std::memset(&hintAddress, 0, sizeof hintAddress);
        hintAddress.ai_family = AF_UNSPEC;
        hintAddress.ai_socktype = SOCK_STREAM;

        Log::addFormatted(Log::VERBOSE, NETWORK_LOG_NAME, "Looking up %s : %s", pIPAddress, pPort);

        if((errorCode = getaddrinfo(pIPAddress, pPort, &hintAddress, &addressInfo)) != 0)
        {
            Log::addFormatted(Log::ERROR, NETWORK_LOG_NAME, "Get Address Info : %s", gai_strerror(errorCode));
            return false;
        }

        for(testAddress=addressInfo;testAddress!=NULL;testAddress=testAddress->ai_next)
        {
            if(testAddress->ai_addr->sa_family == AF_INET6)
            {
                mPort = ((struct sockaddr_in6*)testAddress->ai_addr)->sin6_port;
                if(Endian::sSystemType != Endian::BIG)
                    Endian::reverse(&mPort, 2);
                std::memcpy(mIPv6, (((struct sockaddr_in6*)testAddress->ai_addr)->sin6_addr.s6_addr), INET6_ADDRLEN);
                inet_ntop(testAddress->ai_family, mIPv6, mIPv6Address, INET6_ADDRSTRLEN);
                Log::addFormatted(Log::VERBOSE, NETWORK_LOG_NAME, "Address found IPv6 %s : %d", mIPv6Address, mPort);
            }
            else if(testAddress->ai_addr->sa_family == AF_INET)
            {
                mPort = ((struct sockaddr_in*)testAddress->ai_addr)->sin_port;
                if(Endian::sSystemType != Endian::BIG)
                    Endian::reverse(&mPort, 2);
                std::memcpy(mIPv4, &(((struct sockaddr_in*)testAddress->ai_addr)->sin_addr.s_addr), INET_ADDRLEN);
                inet_ntop(testAddress->ai_family, mIPv4, mIPv4Address, INET_ADDRSTRLEN);
                Log::addFormatted(Log::VERBOSE, NETWORK_LOG_NAME, "Address found IPv4 %s : %d", mIPv4Address, mPort);
            }
        }

        // Loop through all the results and connect to the first we can
        for(testAddress=addressInfo;testAddress!=NULL;testAddress=testAddress->ai_next)
        {
            if((mSocketID = socket(testAddress->ai_family, testAddress->ai_socktype, testAddress->ai_protocol)) == -1)
            {
                Log::addFormatted(Log::VERBOSE, NETWORK_LOG_NAME, "Socket failed : %s", std::strerror(errno));
                continue;
            }

            if(connect(mSocketID, testAddress->ai_addr, testAddress->ai_addrlen) == -1)
            {
                ::close(mSocketID);
                continue;
            }

            if(testAddress->ai_addr->sa_family == AF_INET6)
            {
                mPort = ((struct sockaddr_in6*)testAddress->ai_addr)->sin6_port;
                if(Endian::sSystemType != Endian::BIG)
                    Endian::reverse(&mPort, sizeof(mPort));
                std::memcpy(mIPv6, (((struct sockaddr_in6*)testAddress->ai_addr)->sin6_addr.s6_addr), INET6_ADDRLEN);
                inet_ntop(testAddress->ai_family, mIPv6, mIPv6Address, INET6_ADDRSTRLEN);
            }
            else if(testAddress->ai_addr->sa_family == AF_INET)
            {
                mPort = ((struct sockaddr_in*)testAddress->ai_addr)->sin_port;
                if(Endian::sSystemType != Endian::BIG)
                    Endian::reverse(&mPort, sizeof(mPort));
                std::memcpy(mIPv4, &(((struct sockaddr_in*)testAddress->ai_addr)->sin_addr.s_addr), INET_ADDRLEN);
                inet_ntop(testAddress->ai_family, mIPv4, mIPv4Address, INET_ADDRSTRLEN);
            }

            type = testAddress->ai_addr->sa_family;
            break;
        }

        bool zeroes = true;
        for(int i=0;i<INET6_ADDRLEN;i++)
            if(mIPv6[i] != 0)
                zeroes = false;
        if(zeroes)
        {
            std::memset(mIPv6 + INET6_ADDRLEN - INET_ADDRLEN - 2, 0xff, 2);
            std::memcpy(mIPv6 + INET6_ADDRLEN - INET_ADDRLEN, mIPv4, INET_ADDRLEN);
        }

        freeaddrinfo(addressInfo);

        if(testAddress == NULL)
        {
            mSocketID = -1;
            if(type == AF_INET)
                Log::addFormatted(Log::ERROR, NETWORK_LOG_NAME, "Failed to connect to IPv4 %s : %d - %s", mIPv4Address,
                  mPort, std::strerror(errno));
            else
                Log::addFormatted(Log::ERROR, NETWORK_LOG_NAME, "Failed to connect to IPv6 %s : %d - %s", mIPv6Address,
                  mPort, std::strerror(errno));
            return false;
        }

        setTimeout(10);

        if(type == AF_INET)
            Log::addFormatted(Log::INFO, NETWORK_LOG_NAME, "Connected to IPv4 %s : %d", mIPv4Address, mPort);
        else
            Log::addFormatted(Log::INFO, NETWORK_LOG_NAME, "Connected to IPv6 %s : %d", mIPv6Address, mPort);

        return true;
    }

    Connection::~Connection()
    {
        close();
    }
    
    void Connection::setTimeout(unsigned int pSeconds)
    {
        // Set send/receive timeout
        struct timeval timeout;      
        timeout.tv_sec = pSeconds;
        timeout.tv_usec = 0;

        if(setsockopt(mSocketID, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) == -1)
        {
            if(type == AF_INET)
                Log::addFormatted(Log::ERROR, NETWORK_LOG_NAME, "Failed to set receive timeout on IPv4 %s : %d - %s", mIPv4Address,
                  mPort, std::strerror(errno));
            else
                Log::addFormatted(Log::ERROR, NETWORK_LOG_NAME, "Failed to set receive timeout on IPv6 %s : %d - %s", mIPv6Address,
                  mPort, std::strerror(errno));
        }

        if(setsockopt(mSocketID, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) == -1)
        {
            if(type == AF_INET)
                Log::addFormatted(Log::ERROR, NETWORK_LOG_NAME, "Failed to set send timeout on IPv4 %s : %d - %s", mIPv4Address,
                  mPort, std::strerror(errno));
            else
                Log::addFormatted(Log::ERROR, NETWORK_LOG_NAME, "Failed to set send timeout on IPv6 %s : %d - %s", mIPv6Address,
                  mPort, std::strerror(errno));
        }
    }

    unsigned int Connection::receive(OutputStream *pStream, bool pWait)
    {
        if(mSocketID == -1)
        {
            Log::add(Log::ERROR, NETWORK_LOG_NAME, "Receive failed : socket invalid");
            return false;
        }

        int flags = 0;
        if(!pWait)
            flags |= MSG_DONTWAIT;

        int bytesReceived;
        //Log::add(Log::VERBOSE, NETWORK_LOG_NAME, "Starting Receive");
        if((bytesReceived = recv(mSocketID, mBuffer, NETWORK_BUFFER_SIZE-1, flags)) == -1)
        {
            //Log::add(Log::VERBOSE, NETWORK_LOG_NAME, "Failed Receive");
            if(errno != 11) // Resource temporarily unavailable because of MSG_DONTWAIT
                Log::addFormatted(Log::ERROR, NETWORK_LOG_NAME, "Receive failed : %s", std::strerror(errno));
            return 0;
        }
        //Log::add(Log::VERBOSE, NETWORK_LOG_NAME, "Finished Receive");

        pStream->write(mBuffer, bytesReceived);
        if(bytesReceived > 0)
            Log::addFormatted(Log::DEBUG, NETWORK_LOG_NAME, "Received %d bytes", bytesReceived);
        return bytesReceived;
    }

    bool Connection::send(InputStream *pStream)
    {
        if(mSocketID == -1)
        {
            Log::add(Log::ERROR, NETWORK_LOG_NAME, "Send failed : socket invalid");
            return false;
        }

        unsigned int length;

        while(pStream->remaining() > 0)
        {
            if(pStream->remaining() > NETWORK_BUFFER_SIZE)
                length = NETWORK_BUFFER_SIZE;
            else
                length = pStream->remaining();
            pStream->read(mBuffer, length);

            //Log::add(Log::VERBOSE, NETWORK_LOG_NAME, "Starting Send");
            if(::send(mSocketID, (char *)mBuffer, length, 0) == -1)
            {
                //Log::add(Log::VERBOSE, NETWORK_LOG_NAME, "Failed Send");
                Log::addFormatted(Log::ERROR, NETWORK_LOG_NAME, "Send of %d bytes failed : %s", length, std::strerror(errno));
                return false;
            }
            //Log::add(Log::VERBOSE, NETWORK_LOG_NAME, "Finished Send");

            Log::addFormatted(Log::DEBUG, NETWORK_LOG_NAME, "Sent %d bytes", length);
        }

        return true;
    }

    void Connection::close()
    {
        if(mSocketID != -1)
            ::close(mSocketID);

        mSocketID = -1;
    }
}
