/**************************************************************************
 * Copyright 2017-2018 NextCash, LLC                                      *
 * Contributors :                                                         *
 *   Curtis Ellis <curtis@nextcash.tech>                                  *
 * Distributed under the MIT software license, see the accompanying       *
 * file license.txt or http://www.opensource.org/licenses/mit-license.php *
 **************************************************************************/
#include "network.hpp"

#include "log.hpp"

#include <cstdlib>
#include <cstring>
#include <netdb.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define NETWORK_LOG_NAME "Network"


namespace NextCash
{
    namespace Network
    {
        uint8_t *parseIP(const char *pValue)
        {
            struct in6_addr address6;

            if(inet_pton(AF_INET6, pValue, &address6) == 1)
            {
                uint8_t *result = new uint8_t[INET6_ADDRLEN];
                std::memcpy(result, address6.s6_addr, INET6_ADDRLEN);
            }

            struct in_addr address4;
            if(inet_pton(AF_INET, pValue, &address4) != 1)
                return NULL;

            uint8_t *result = new uint8_t[INET6_ADDRLEN];
            std::memset(result, 0, INET6_ADDRLEN);
            std::memset(result + INET6_ADDRLEN - INET_ADDRLEN - 2, 0xff, 2);
            std::memcpy(result + INET6_ADDRLEN - INET_ADDRLEN, (uint8_t *)&address4.s_addr,
              INET_ADDRLEN);

            return result;
        }

        bool isIPv4MappedIPv6(const uint8_t *pIP)
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

        IPAddress::IPAddress()
        {
            mType = IPV6;
            std::memset(mIP, 0, INET6_ADDRLEN);
            mPort = 0;
        }

        IPAddress::IPAddress(const IPAddress &pCopy)
        {
            mType = pCopy.mType;
            std::memcpy(mIP, pCopy.mIP, INET6_ADDRLEN);
            mPort = pCopy.mPort;
        }

        IPAddress::IPAddress(Type pType, const uint8_t *pIP, uint16_t pPort)
        {
            if(pType == IPV6)
            {
                if(isIPv4MappedIPv6(pIP))
                    mType = IPV4;
                else
                    mType = IPV6;
                std::memcpy(mIP, pIP, INET6_ADDRLEN);
            }
            else
            {
                mType = IPV4;

                // Map to IPV6 format for internal storage
                std::memset(mIP, 0, 10);
                mIP[10] = 0xff;
                mIP[11] = 0xff;
                std::memcpy(mIP + INET6_ADDRLEN - INET_ADDRLEN, pIP, INET_ADDRLEN);
            }

            mPort = pPort;
        }

        void IPAddress::clear()
        {
            mType = IPV6;
            std::memset(mIP, 0, INET6_ADDRLEN);
            mPort = 0;
        }

        bool IPAddress::matches(const IPAddress &pOther) const
        {
            return std::memcmp(mIP, pOther.mIP, INET6_ADDRLEN) == 0 && mPort == pOther.mPort;
        }

        bool IPAddress::operator == (const IPAddress &pRight) const
        {
            return std::memcmp(mIP, pRight.mIP, INET6_ADDRLEN) == 0 && mPort == pRight.mPort;
        }

        bool IPAddress::operator != (const IPAddress &pRight) const
        {
            return std::memcmp(mIP, pRight.mIP, INET6_ADDRLEN) != 0 || mPort != pRight.mPort;
        }

        int IPAddress::compare(const IPAddress &pRight) const
        {
            const uint8_t *left = mIP;
            const uint8_t *right = pRight.mIP;
            for(unsigned int i = 0; i < INET6_ADDRLEN; ++i, ++left, ++right)
            {
                if(*left < *right)
                    return -1;
                else if(*left > *right)
                    return 1;
            }

            if(mPort < pRight.mPort)
                return -1;
            else if(mPort > pRight.mPort)
                return 1;
            else
                return 0;
        }

        bool IPAddress::isValid() const
        {
            const uint8_t *byte = mIP;
            for(unsigned int i = 0; i < INET6_ADDRLEN; ++i, ++byte)
                if(*byte != 0)
                    return true;
            return false;
        }

        const IPAddress &IPAddress::operator = (const IPAddress &pRight)
        {
            mType = pRight.mType;
            mPort = pRight.mPort;
            std::memcpy(mIP, pRight.mIP, INET6_ADDRLEN);
            return *this;
        }

        void IPAddress::set(Type pType, const uint8_t *pIP, uint16_t pPort)
        {
            if(pType == IPV6)
            {
                if(isIPv4MappedIPv6(pIP))
                    mType = IPV4;
                else
                    mType = IPV6;
                std::memcpy(mIP, pIP, INET6_ADDRLEN);
            }
            else
            {
                mType = IPV4;

                // Map to IPV6 format for internal storage
                std::memset(mIP, 0, 10);
                mIP[10] = 0xff;
                mIP[11] = 0xff;
                std::memcpy(mIP + INET6_ADDRLEN - INET_ADDRLEN, pIP, INET_ADDRLEN);
            }

            mPort = pPort;
        }

        String IPAddress::text() const
        {
            char ipv6Text[INET6_ADDRSTRLEN];
            std::memset(ipv6Text, 0, INET6_ADDRSTRLEN);
            inet_ntop(AF_INET6, mIP, ipv6Text, INET6_ADDRSTRLEN);

            String result;
            result.writeFormatted("%s::%d", ipv6Text, mPort);
            return result;
        }

        bool IPAddress::setText(const char *pText)
        {
            struct in6_addr address6;
            if(inet_pton(AF_INET6, pText, &address6) == 1)
            {
                mType = IPV6;
                std::memcpy(mIP, address6.s6_addr, INET6_ADDRLEN);
                return true;
            }

            struct in_addr address4;
            if(inet_pton(AF_INET, pText, &address4) != 1)
                return false;

            mType = IPV4;
            std::memset(mIP, 0, 10);
            mIP[10] = 0xff;
            mIP[11] = 0xff;
            std::memcpy(mIP + INET6_ADDRLEN - INET_ADDRLEN, (uint8_t *)&address4.s_addr,
              INET_ADDRLEN);
            return true;
        }

        void IPAddress::write(OutputStream *pStream) const
        {
            // IP
            pStream->write(mIP, 16);

            // Port
            Endian::Type previousType = pStream->outputEndian();
            pStream->setOutputEndian(Endian::BIG);
            pStream->writeUnsignedShort(mPort);
            pStream->setOutputEndian(previousType);
        }

        bool IPAddress::read(InputStream *pStream)
        {
            // IP
            pStream->read(mIP, 16);

            // Port
            Endian::Type previousType = pStream->inputEndian();
            pStream->setInputEndian(Endian::BIG);
            mPort = pStream->readUnsignedShort();
            pStream->setInputEndian(previousType);

            return true;
        }

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
                Log::addFormatted(Log::VERBOSE, NETWORK_LOG_NAME, "Get Address Info : %s",
                  gai_strerror(errorCode));
                return false;
            }

            uint8_t address[INET6_ADDRLEN];
            char ip[INET6_ADDRSTRLEN];
            char *newIP;

            for(testAddress = addressInfo; testAddress != NULL; testAddress = testAddress->ai_next)
            {
                if(testAddress->ai_addr->sa_family == AF_INET6)
                {
                    std::memcpy(address,
                      (((struct sockaddr_in6*)testAddress->ai_addr)->sin6_addr.s6_addr),
                      INET6_ADDRLEN);
                    inet_ntop(testAddress->ai_family, address, ip, INET6_ADDRSTRLEN);
                    Log::addFormatted(Log::VERBOSE, NETWORK_LOG_NAME, "Address found IPv6 %s", ip);
                }
                else if(testAddress->ai_addr->sa_family == AF_INET)
                {
                    std::memcpy(address,
                      &(((struct sockaddr_in*)testAddress->ai_addr)->sin_addr.s_addr),
                      INET_ADDRLEN);
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

        Connection::Connection(IPAddress &pIP, unsigned int pTimeout) : mIP(pIP)
        {
            mSocketID = -1;
            mBytesReceived = 0;
            mBytesSent = 0;
            open(pTimeout);
        }

        Connection::Connection(IPAddress::Type pType, const uint8_t *pIP, uint16_t pPort,
          unsigned int pTimeout) : mIP(pType, pIP, pPort)
        {
            mSocketID = -1;
            mBytesReceived = 0;
            mBytesSent = 0;
            open(pTimeout);
        }

        Connection::Connection(int pSocketID, struct sockaddr *pAddress)
        {
            mSocketID = -1;
            mBytesReceived = 0;
            mBytesSent = 0;
            set(pSocketID, pAddress);
        }

        bool Connection::set(int pSocketID, struct sockaddr *pAddress)
        {
            close();

            mSocketID = pSocketID;
            mBytesReceived = 0;
            mBytesSent = 0;

            struct sockaddr_in *address = (struct sockaddr_in *)pAddress;

            if(address->sin_family == AF_INET6)
                mIP.set(IPAddress::IPV6, ((struct sockaddr_in6*)address)->sin6_addr.s6_addr,
                  Endian::convert(((struct sockaddr_in6*)address)->sin6_port, Endian::BIG));
            else if(address->sin_family == AF_INET)
                mIP.set(IPAddress::IPV4, (uint8_t *)&(((struct sockaddr_in*)address)->sin_addr.s_addr),
                  Endian::convert(((struct sockaddr_in*)address)->sin_port, Endian::BIG));
            else
                return false;

            return true;
        }

        bool Connection::open(const IPAddress &pIP, unsigned int pTimeout)
        {
            close(); // Previous connection
            mBytesReceived = 0;
            mBytesSent = 0;

            mIP = pIP;
            return open(pTimeout);
        }

        bool Connection::open(IPAddress::Type pType, const uint8_t *pIP, uint16_t pPort,
          unsigned int pTimeout)
        {
            close(); // Previous connection
            mBytesReceived = 0;
            mBytesSent = 0;

            mIP.set(pType, pIP, pPort);

            return open(pTimeout);
        }

        bool Connection::open(unsigned int pTimeout)
        {
            if(mIP.type() == IPAddress::IPV6)
            {
                Log::addFormatted(Log::DEBUG, NETWORK_LOG_NAME,
                  "Attempting IPv6 connection to %s", mIP.text().text());

                struct sockaddr_in6 address;
                address.sin6_family = AF_INET6;
                address.sin6_port = Endian::convert(mIP.port(), Endian::BIG);
                address.sin6_flowinfo = 0;
                std::memcpy(address.sin6_addr.s6_addr, mIP.ipv6Bytes(), INET6_ADDRLEN);
                address.sin6_scope_id = 0;

                if((mSocketID = ::socket(AF_INET6, SOCK_STREAM, 6)) == -1)
                {
                    Log::addFormatted(Log::DEBUG, NETWORK_LOG_NAME, "Socket failed : %s",
                      std::strerror(errno));
                    return false;
                }

                setTimeout(pTimeout);

                if(connect(mSocketID, (const sockaddr *)&address, sizeof(address)) == -1)
                {
                    Log::addFormatted(Log::DEBUG, NETWORK_LOG_NAME, "Connect failed : %s",
                      std::strerror(errno));
                    ::close(mSocketID);
                    mSocketID = -1;
                    return false;
                }

                Log::addFormatted(Log::VERBOSE, NETWORK_LOG_NAME, "Connected to IPv6 %s",
                  mIP.text().text());
                return true;
            }
            else
            {
                Log::addFormatted(Log::DEBUG, NETWORK_LOG_NAME,
                  "Attempting IPv4 connection to %s", mIP.text().text());

                struct sockaddr_in address;
                address.sin_family = AF_INET;
                address.sin_port = Endian::convert(mIP.port(), Endian::BIG);
                std::memcpy(&address.sin_addr.s_addr, mIP.ipv4Bytes(), INET_ADDRLEN);

                if((mSocketID = ::socket(AF_INET, SOCK_STREAM, 6)) == -1)
                {
                    Log::addFormatted(Log::DEBUG, NETWORK_LOG_NAME, "Socket failed : %s",
                      std::strerror(errno));
                    return false;
                }

                setTimeout(pTimeout);

                if(connect(mSocketID, (const sockaddr *)&address, sizeof(address)) == -1)
                {
                    Log::addFormatted(Log::DEBUG, NETWORK_LOG_NAME, "Connect failed : %s",
                      std::strerror(errno));
                    ::close(mSocketID);
                    mSocketID = -1;
                    return false;
                }

                Log::addFormatted(Log::VERBOSE, NETWORK_LOG_NAME, "Connected to IPv4 %s",
                  mIP.text().text());
                return true;
            }
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

            if(setsockopt(mSocketID, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
              sizeof(timeout)) == -1)
            {
                if(mIP.type() == IPAddress::IPV6)
                    Log::addFormatted(Log::ERROR, NETWORK_LOG_NAME,
                      "Failed to set receive timeout on IPv6 %s - %s", mIP.text().text(),
                      std::strerror(errno));
                else
                    Log::addFormatted(Log::ERROR, NETWORK_LOG_NAME,
                      "Failed to set receive timeout on IPv4 %s - %s", mIP.text().text(),
                      std::strerror(errno));
            }

            if(setsockopt(mSocketID, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,
              sizeof(timeout)) == -1)
            {
                if(mIP.type() == IPAddress::IPV6)
                    Log::addFormatted(Log::ERROR, NETWORK_LOG_NAME,
                      "Failed to set send timeout on IPv6 %s - %s", mIP.text().text(),
                      std::strerror(errno));
                else
                    Log::addFormatted(Log::ERROR, NETWORK_LOG_NAME,
                      "Failed to set send timeout on IPv4 %s - %s", mIP.text().text(),
                      std::strerror(errno));
            }
        }

        unsigned int Connection::receive(OutputStream *pStream, bool pWait)
        {
            if(mSocketID == -1)
            {
                Log::add(Log::VERBOSE, NETWORK_LOG_NAME, "Receive failed : socket closed");
                return 0;
            }

            int flags = 0;
            if(!pWait)
                flags |= MSG_DONTWAIT;

            unsigned int result = 0;
            int bytesCount;
            while(true)
            {
                //Log::add(Log::VERBOSE, NETWORK_LOG_NAME, "Starting Receive");
                bytesCount = recv(mSocketID, mBuffer, NETWORK_BUFFER_SIZE, flags);
                if(bytesCount < 0)
                {
                    //Log::add(Log::VERBOSE, NETWORK_LOG_NAME, "Failed Receive");
                    if(errno != 11) // Resource temporarily unavailable because of MSG_DONTWAIT
                    {
                        Log::addFormatted(Log::VERBOSE, NETWORK_LOG_NAME,
                          "[%d] Receive failed : %s", mSocketID, std::strerror(errno));
                        close();
                    }
                    break;
                }
                else if(bytesCount == 0)
                    break;
                //Log::add(Log::VERBOSE, NETWORK_LOG_NAME, "Finished Receive");

                result += (unsigned int)bytesCount;
                mBytesReceived += bytesCount;
                pStream->write(mBuffer, bytesCount);
                if(bytesCount < NETWORK_BUFFER_SIZE-1)
                    break;
            }

            //if(result > 0)
            //    Log::addFormatted(Log::DEBUG, NETWORK_LOG_NAME, "Received %d bytes", result);
            return result;
        }

        bool Connection::send(InputStream *pStream)
        {
            if(mSocketID == -1)
            {
                Log::add(Log::VERBOSE, NETWORK_LOG_NAME, "Send failed : socket closed");
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
                    Log::addFormatted(Log::VERBOSE, NETWORK_LOG_NAME,
                      "[%d] Send of %d bytes failed : %s", mSocketID, length,
                      std::strerror(errno));
                    close();
                    return false;
                }
                //Log::add(Log::VERBOSE, NETWORK_LOG_NAME, "Finished Send");

                mBytesSent += length;

                //Log::addFormatted(Log::DEBUG, NETWORK_LOG_NAME, "Sent %d bytes", length);
            }

            return true;
        }

        void Connection::close()
        {
            if(mSocketID != -1)
                ::close(mSocketID);

            mSocketID = -1;
        }

        Listener::Listener(sa_family_t pType, uint16_t pPort, unsigned int pListenBackLog,
          unsigned int pTimeoutSeconds)
        {
            mTimeoutSeconds = pTimeoutSeconds;
            mSocketID = ::socket(pType, SOCK_STREAM, 6);

            if(mSocketID == -1)
            {
                Log::addFormatted(Log::ERROR, NETWORK_LOG_NAME,
                  "Listener socket create failed : %s", std::strerror(errno));
                return;
            }

            setTimeout(mTimeoutSeconds);

            //  Set listening socket to allow multiple connections
            int optionValue = -1;
            if(::setsockopt(mSocketID, SOL_SOCKET, SO_REUSEADDR, (char *)&optionValue,
              sizeof(optionValue)) < 0)
            {
                Log::addFormatted(Log::ERROR, NETWORK_LOG_NAME,
                  "Listener socket failed to set options : %s", std::strerror(errno));
                ::close(mSocketID);
                mSocketID = -1;
                return;
            }

            mPort = pPort;

            if(pType == AF_INET6)
            {
                struct sockaddr_in6 address;
                std::memset(&address, 0, sizeof(address));
                address.sin6_family = AF_INET6;
                address.sin6_port = Endian::convert(mPort, Endian::BIG);
                //address.sin6_flowinfo = ;
                address.sin6_addr = in6addr_any; // Allow connections to any local address
                //address.sin6_scope_id = ;

                if(bind(mSocketID, (struct sockaddr *)&address, sizeof(address)) < 0)
                {
                    Log::addFormatted(Log::ERROR, NETWORK_LOG_NAME,
                      "Listener binding failed : %s", std::strerror(errno));
                    ::close(mSocketID);
                    mSocketID = -1;
                    return;
                }
            }
            else if(pType == AF_INET)
            {
                struct sockaddr_in address;
                std::memset(&address, 0, sizeof(address));
                address.sin_family = AF_INET;
                address.sin_port = Endian::convert(mPort, Endian::BIG);
                address.sin_addr.s_addr = INADDR_ANY; // Allow connections to any local address

                if(bind(mSocketID, (struct sockaddr *)&address, sizeof(address)) < 0)
                {
                    Log::addFormatted(Log::ERROR, NETWORK_LOG_NAME, "Listener binding failed : %s",
                      std::strerror(errno));
                    ::close(mSocketID);
                    mSocketID = -1;
                    return;
                }
            }

            if(::listen(mSocketID, pListenBackLog) < 0)
            {
                Log::addFormatted(Log::ERROR, NETWORK_LOG_NAME, "Listener failed to listen : %s",
                  std::strerror(errno));
                ::close(mSocketID);
                mSocketID = -1;
                return;
            }
        }

        // Check for new connections and add them to the pending connections list
        bool Listener::processConnections()
        {
            // Setup the socket set to only contain the "listener" socket
            fd_set set;
            FD_ZERO(&set);
            FD_SET(mSocketID, &set);

            // Set timeout
            struct timeval timeout;
            timeout.tv_sec = mTimeoutSeconds;
            timeout.tv_usec = 0;

            int result = ::select(mSocketID + 1, &set, NULL, NULL, &timeout);
            if(result > 0)
            {
                // "result" is number of sockets connected
                // I think the set now contains only the new connection's socket IDs.
                // It doesn't contain the listener socket id anymore
                // "accept" below gives the new socket IDs one at a time

                Connection *newConnection;
                struct sockaddr_in6 peerAddress;
                socklen_t  peerAddressSize = sizeof(peerAddress);
                int newSocketID;
                uint8_t ip[INET6_ADDRLEN];
                char ipText[INET6_ADDRSTRLEN];
                uint16_t port;
                bool success = true;

                for(int i = 0; i < result; ++i)
                {
                    // Get new connection
                    newSocketID = ::accept(mSocketID, (struct sockaddr *)&peerAddress,
                      &peerAddressSize);
                    if(newSocketID < 0)
                    {
                        Log::addFormatted(Log::VERBOSE, NETWORK_LOG_NAME,
                          "Listener accept failed : %s", std::strerror(errno));
                        success = false;
                    }
                    else
                    {
                        // Create new connection and add it to the pending list
                        if(peerAddress.sin6_family == AF_INET6)
                        {
                            port = Endian::convert(((struct sockaddr_in6*)&peerAddress)->sin6_port,
                              Endian::BIG);
                            std::memcpy(ip,
                              (((struct sockaddr_in6*)&peerAddress)->sin6_addr.s6_addr),
                              INET6_ADDRLEN);
                            inet_ntop(peerAddress.sin6_family, ip, ipText, INET6_ADDRSTRLEN);
                            Log::addFormatted(Log::DEBUG, NETWORK_LOG_NAME,
                              "New IPv6 connection %s : %d (socket %d)", ipText, port,
                              newSocketID);
                        }
                        else if(peerAddress.sin6_family == AF_INET)
                        {
                            port =
                              Endian::convert(((struct sockaddr_in*)&peerAddress)->sin_port,
                              Endian::BIG);
                            std::memcpy(ip,
                              &(((struct sockaddr_in*)&peerAddress)->sin_addr.s_addr),
                              INET_ADDRLEN);
                            inet_ntop(peerAddress.sin6_family, ip, ipText, INET_ADDRSTRLEN);
                            Log::addFormatted(Log::DEBUG, NETWORK_LOG_NAME,
                              "New IPv4 connection %s : %d (socket %d)", ipText, port,
                              newSocketID);
                        }
                        else
                        {
                            Log::addFormatted(Log::VERBOSE, NETWORK_LOG_NAME,
                              "Unknown type for new connection");
                            success = false;
                            ::close(newSocketID);
                            continue;
                        }

                        try
                        {
                            newConnection = new Connection(newSocketID,
                              (struct sockaddr *)&peerAddress);
                        }
                        catch(std::bad_alloc &pBadAlloc)
                        {
                            NextCash::Log::addFormatted(NextCash::Log::ERROR, NETWORK_LOG_NAME,
                              "Bad allocation while allocating new connection : %s",
                              pBadAlloc.what());
                            ::close(newSocketID);
                            continue;
                        }
                        catch(...)
                        {
                            NextCash::Log::add(NextCash::Log::ERROR, NETWORK_LOG_NAME,
                              "Bad allocation while allocating new connection : unknown");
                            ::close(newSocketID);
                            continue;
                        }

                        mPendingConnections.push_back(newConnection);
                    }
                }

                return success;
            }
            else if(result < 0)
            {
                Log::addFormatted(Log::ERROR, NETWORK_LOG_NAME, "Listener select failed : %s",
                  std::strerror(errno));
                return false;
            }

            // No new connections
            return true;
        }

        // Check for a connection
        Connection *Listener::accept()
        {
            if(mPendingConnections.size() == 0)
                processConnections();

            Connection *result = NULL;
            if(mPendingConnections.size() > 0)
            {
                // Retrieve and remove the first connection
                result = mPendingConnections.front();
                mPendingConnections.erase(mPendingConnections.begin());
            }

            return result;
        }

        void Listener::close()
        {
            // Close pending sockets
            for(std::vector<Connection *>::iterator connection = mPendingConnections.begin();
              connection != mPendingConnections.end(); ++connection)
                delete *connection;

            mPendingConnections.clear();

            // Close "listener" socket
            if(mSocketID != -1)
                ::close(mSocketID);

            mSocketID = -1;
        }

        void Listener::setTimeout(unsigned int pSeconds)
        {
            // Set send/receive timeout
            struct timeval timeout;
            timeout.tv_sec = pSeconds;
            timeout.tv_usec = 0;

            if(setsockopt(mSocketID, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout,
              sizeof(timeout)) == -1)
            {
                Log::addFormatted(Log::ERROR, NETWORK_LOG_NAME,
                  "Failed to set receive timeout on listener %s : port %d", mPort,
                  std::strerror(errno));
            }

            if(setsockopt(mSocketID, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout,
              sizeof(timeout)) == -1)
            {
                Log::addFormatted(Log::ERROR, NETWORK_LOG_NAME,
                  "Failed to set send timeout on listener %s : port %d", mPort,
                  std::strerror(errno));
            }
        }
    }
}
