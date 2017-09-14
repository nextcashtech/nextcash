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

        Connection::Connection(const char *pIPAddress, const char *pPort, unsigned int pTimeout)
        {
            mSocketID = -1;
            mBytesReceived = 0;
            mBytesSent = 0;
            open(pIPAddress, pPort, pTimeout);
        }

        Connection::Connection(unsigned int pFamily, const uint8_t *pIP, uint16_t pPort, unsigned int pTimeout)
        {
            mSocketID = -1;
            mBytesReceived = 0;
            mBytesSent = 0;
            open(pFamily, pIP, pPort, pTimeout);
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

            std::memset(mIPv4Address, 0, INET_ADDRSTRLEN);
            std::memset(mIPv6Address, 0, INET6_ADDRSTRLEN);

            mSocketID = pSocketID;
            mBytesReceived = 0;
            mBytesSent = 0;

            struct sockaddr_in *address = (struct sockaddr_in *)pAddress;

            mType = address->sin_family;
            if(address->sin_family == AF_INET6)
            {
                mPort = Endian::convert(((struct sockaddr_in6*)address)->sin6_port, Endian::BIG);
                std::memcpy(mIPv6, (((struct sockaddr_in6*)address)->sin6_addr.s6_addr), INET6_ADDRLEN);
                inet_ntop(address->sin_family, mIPv6, mIPv6Address, INET6_ADDRSTRLEN);
            }
            else if(address->sin_family == AF_INET)
            {
                mPort = Endian::convert(((struct sockaddr_in*)address)->sin_port, Endian::BIG);
                std::memcpy(mIPv4, &(((struct sockaddr_in*)address)->sin_addr.s_addr), INET_ADDRLEN);
                inet_ntop(address->sin_family, mIPv4, mIPv4Address, INET_ADDRSTRLEN);

                std::memset(mIPv6 + INET6_ADDRLEN - INET_ADDRLEN - 2, 0xff, 2);
                std::memcpy(mIPv6 + INET6_ADDRLEN - INET_ADDRLEN, mIPv4, INET_ADDRLEN);
            }

            return false;
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

        bool Connection::open(unsigned int pFamily, const uint8_t *pIP, uint16_t pPort, unsigned int pTimeout)
        {
            close(); // Previous connection
            mBytesReceived = 0;
            mBytesSent = 0;

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
                address.sin_port = Endian::convert(mPort, Endian::BIG);
                std::memcpy(&address.sin_addr.s_addr, mIPv4, INET_ADDRLEN);

                if((mSocketID = socket(AF_INET, SOCK_STREAM, 6)) == -1)
                {
                    Log::addFormatted(Log::ERROR, NETWORK_LOG_NAME, "Socket failed : %s", std::strerror(errno));
                    return false;
                }

                setTimeout(pTimeout);

                if(connect(mSocketID, (const sockaddr *)&address, sizeof(address)) == -1)
                {
                    Log::addFormatted(Log::ERROR, NETWORK_LOG_NAME, "Connect failed : %s", std::strerror(errno));
                    ::close(mSocketID);
                    mSocketID = -1;
                    return false;
                }

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
                address.sin6_port = Endian::convert(mPort, Endian::BIG);
                address.sin6_flowinfo = 0;
                std::memcpy(address.sin6_addr.s6_addr, mIPv6, INET6_ADDRLEN);
                address.sin6_scope_id = 0;

                if((mSocketID = socket(AF_INET6, SOCK_STREAM, 6)) == -1)
                {
                    Log::addFormatted(Log::ERROR, NETWORK_LOG_NAME, "Socket failed : %s", std::strerror(errno));
                    return false;
                }

                setTimeout(pTimeout);

                if(connect(mSocketID, (const sockaddr *)&address, sizeof(address)) == -1)
                {
                    Log::addFormatted(Log::ERROR, NETWORK_LOG_NAME, "Connect failed : %s", std::strerror(errno));
                    ::close(mSocketID);
                    mSocketID = -1;
                    return false;
                }

                Log::addFormatted(Log::INFO, NETWORK_LOG_NAME, "Connected to IPv6 %s : %d", mIPv6Address, mPort);
                return true;
            }
            else
            {
                Log::add(Log::ERROR, NETWORK_LOG_NAME, "Unknown network family");
                return false;
            }
        }

        bool Connection::open(const char *pIPAddress, const char *pPort, unsigned int pTimeout)
        {
            close(); // Previous connection
            mBytesReceived = 0;
            mBytesSent = 0;

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
                    mPort = Endian::convert(((struct sockaddr_in6*)testAddress->ai_addr)->sin6_port, Endian::BIG);
                    std::memcpy(mIPv6, (((struct sockaddr_in6*)testAddress->ai_addr)->sin6_addr.s6_addr), INET6_ADDRLEN);
                    inet_ntop(testAddress->ai_family, mIPv6, mIPv6Address, INET6_ADDRSTRLEN);
                    Log::addFormatted(Log::VERBOSE, NETWORK_LOG_NAME, "Address found IPv6 %s : %d", mIPv6Address, mPort);
                }
                else if(testAddress->ai_addr->sa_family == AF_INET)
                {
                    mPort = Endian::convert(((struct sockaddr_in*)testAddress->ai_addr)->sin_port, Endian::BIG);
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

                setTimeout(pTimeout);

                if(connect(mSocketID, testAddress->ai_addr, testAddress->ai_addrlen) == -1)
                {
                    ::close(mSocketID);
                    continue;
                }

                if(testAddress->ai_addr->sa_family == AF_INET6)
                {
                    mPort = Endian::convert(((struct sockaddr_in6*)testAddress->ai_addr)->sin6_port, Endian::BIG);
                    std::memcpy(mIPv6, (((struct sockaddr_in6*)testAddress->ai_addr)->sin6_addr.s6_addr), INET6_ADDRLEN);
                    inet_ntop(testAddress->ai_family, mIPv6, mIPv6Address, INET6_ADDRSTRLEN);
                }
                else if(testAddress->ai_addr->sa_family == AF_INET)
                {
                    mPort = Endian::convert(((struct sockaddr_in*)testAddress->ai_addr)->sin_port, Endian::BIG);
                    std::memcpy(mIPv4, &(((struct sockaddr_in*)testAddress->ai_addr)->sin_addr.s_addr), INET_ADDRLEN);
                    inet_ntop(testAddress->ai_family, mIPv4, mIPv4Address, INET_ADDRSTRLEN);
                }

                mType = testAddress->ai_addr->sa_family;
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
                if(mType == AF_INET)
                    Log::addFormatted(Log::ERROR, NETWORK_LOG_NAME, "Failed to connect to IPv4 %s : %d - %s", mIPv4Address,
                      mPort, std::strerror(errno));
                else
                    Log::addFormatted(Log::ERROR, NETWORK_LOG_NAME, "Failed to connect to IPv6 %s : %d - %s", mIPv6Address,
                      mPort, std::strerror(errno));
                return false;
            }

            if(mType == AF_INET)
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
                if(mType == AF_INET)
                    Log::addFormatted(Log::ERROR, NETWORK_LOG_NAME, "Failed to set receive timeout on IPv4 %s : %d - %s", mIPv4Address,
                      mPort, std::strerror(errno));
                else
                    Log::addFormatted(Log::ERROR, NETWORK_LOG_NAME, "Failed to set receive timeout on IPv6 %s : %d - %s", mIPv6Address,
                      mPort, std::strerror(errno));
            }

            if(setsockopt(mSocketID, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) == -1)
            {
                if(mType == AF_INET)
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
                Log::add(Log::ERROR, NETWORK_LOG_NAME, "Receive failed : socket closed");
                return false;
            }

            int flags = 0;
            if(!pWait)
                flags |= MSG_DONTWAIT;

            int bytesCount;
            //Log::add(Log::VERBOSE, NETWORK_LOG_NAME, "Starting Receive");
            if((bytesCount = recv(mSocketID, mBuffer, NETWORK_BUFFER_SIZE-1, flags)) == -1)
            {
                //Log::add(Log::VERBOSE, NETWORK_LOG_NAME, "Failed Receive");
                if(errno != 11) // Resource temporarily unavailable because of MSG_DONTWAIT
                {
                    Log::addFormatted(Log::ERROR, NETWORK_LOG_NAME, "[%d] Receive failed : %s", mSocketID, std::strerror(errno));
                    close();
                }
                return 0;
            }
            //Log::add(Log::VERBOSE, NETWORK_LOG_NAME, "Finished Receive");

            mBytesReceived += bytesCount;
            pStream->write(mBuffer, bytesCount);
            //if(bytesCount > 0)
            //    Log::addFormatted(Log::DEBUG, NETWORK_LOG_NAME, "Received %d bytes", bytesCount);
            return bytesCount;
        }

        bool Connection::send(InputStream *pStream)
        {
            if(mSocketID == -1)
            {
                Log::add(Log::ERROR, NETWORK_LOG_NAME, "Send failed : socket closed");
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
                    Log::addFormatted(Log::ERROR, NETWORK_LOG_NAME, "[%d] Send of %d bytes failed : %s",
                      mSocketID, length, std::strerror(errno));
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

        Listener::Listener(sa_family_t pType, uint16_t pPort, unsigned int pListenBackLog, unsigned int pTimeoutSeconds)
        {
            mTimeoutSeconds = pTimeoutSeconds;
            mSocketID = ::socket(pType, SOCK_STREAM, 6);

            if(mSocketID == -1)
            {
                Log::addFormatted(Log::ERROR, NETWORK_LOG_NAME, "Listener socket create failed : %s", std::strerror(errno));
                return;
            }

            setTimeout(mTimeoutSeconds);

            //  Set listening socket to allow multiple connections
            int optionValue = -1;
            if(::setsockopt(mSocketID, SOL_SOCKET, SO_REUSEADDR, (char *)&optionValue, sizeof(optionValue)) < 0)
            {
                Log::addFormatted(Log::ERROR, NETWORK_LOG_NAME, "Listener socket failed to set options : %s", std::strerror(errno));
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
                    Log::addFormatted(Log::ERROR, NETWORK_LOG_NAME, "Listener binding failed : %s", std::strerror(errno));
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
                    Log::addFormatted(Log::ERROR, NETWORK_LOG_NAME, "Listener binding failed : %s", std::strerror(errno));
                    ::close(mSocketID);
                    mSocketID = -1;
                    return;
                }
            }

            if(::listen(mSocketID, pListenBackLog) < 0)
            {
                Log::addFormatted(Log::ERROR, NETWORK_LOG_NAME, "Listener failed to listen : %s", std::strerror(errno));
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

                for(int i=0;i<result;++i)
                {
                    // Get new connection
                    newSocketID = ::accept(mSocketID, (struct sockaddr *)&peerAddress, &peerAddressSize);
                    if(newSocketID < 0)
                    {
                        Log::addFormatted(Log::ERROR, NETWORK_LOG_NAME, "Listener accept failed : %s", std::strerror(errno));
                        success = false;
                    }
                    else
                    {
                        // Create new connection and add it to the pending list
                        if(peerAddress.sin6_family == AF_INET6)
                        {
                            port = Endian::convert(((struct sockaddr_in6*)&peerAddress)->sin6_port, Endian::BIG);
                            std::memcpy(ip, (((struct sockaddr_in6*)&peerAddress)->sin6_addr.s6_addr), INET6_ADDRLEN);
                            inet_ntop(peerAddress.sin6_family, ip, ipText, INET6_ADDRSTRLEN);
                            Log::addFormatted(Log::VERBOSE, NETWORK_LOG_NAME, "New IPv6 connection %s : %d", ipText, port);
                        }
                        else if(peerAddress.sin6_family == AF_INET)
                        {
                            port = Endian::convert(((struct sockaddr_in*)&peerAddress)->sin_port, Endian::BIG);
                            std::memcpy(ip, &(((struct sockaddr_in*)&peerAddress)->sin_addr.s_addr), INET_ADDRLEN);
                            inet_ntop(peerAddress.sin6_family, ip, ipText, INET_ADDRSTRLEN);
                            Log::addFormatted(Log::VERBOSE, NETWORK_LOG_NAME, "New IPv4 connection %s : %d", ipText, port);
                        }
                        else
                        {
                            Log::addFormatted(Log::ERROR, NETWORK_LOG_NAME, "Unknown type for new connection");
                            success = false;
                            ::close(newSocketID);
                            continue;
                        }

                        try
                        {
                            newConnection = new Connection(newSocketID, (struct sockaddr *)&peerAddress);
                        }
                        catch(std::bad_alloc &pBadAlloc)
                        {
                            ArcMist::Log::addFormatted(ArcMist::Log::ERROR, NETWORK_LOG_NAME,
                              "Bad allocation while allocating new connection : %s", pBadAlloc.what());
                            ::close(newSocketID);
                            continue;
                        }
                        catch(...)
                        {
                            ArcMist::Log::add(ArcMist::Log::ERROR, NETWORK_LOG_NAME,
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
                Log::addFormatted(Log::ERROR, NETWORK_LOG_NAME, "Listener select failed : %s", std::strerror(errno));
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
            for(std::vector<Connection *>::iterator connection=mPendingConnections.begin();connection!=mPendingConnections.end();++connection)
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

            if(setsockopt(mSocketID, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) == -1)
            {
                Log::addFormatted(Log::ERROR, NETWORK_LOG_NAME, "Failed to set receive timeout on listener %s : port %d",
                  mPort, std::strerror(errno));
            }

            if(setsockopt(mSocketID, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) == -1)
            {
                Log::addFormatted(Log::ERROR, NETWORK_LOG_NAME, "Failed to set send timeout on listener %s : port %d",
                  mPort, std::strerror(errno));
            }
        }
    }
}
