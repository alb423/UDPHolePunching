#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <netdb.h>
#include <arpa/inet.h>



#ifdef __APPLE__
    #include <sys/socket.h>
    #include <sys/sysctl.h>
    #include <net/if.h>
    #include <net/if_dl.h>
#else
    #include <ifaddrs.h>
#endif


#include "twp2p_err.h"
#include "util.h"

int  gLocalInterfaceCount=0;

tNICInfo gxNICInfo[NET_MAX_INTERFACE];

void * MyMalloc(int vSize)
{
    char *pSrc = NULL;

    if(vSize<=0) return NULL;

    pSrc = malloc(vSize);
    memset(pSrc, 0, vSize);

    return pSrc;
}

void MyFree(void *ptr)
{
    free(ptr);
}

char * CopyString(char *pSrc)
{
    int vLen = 0;
    char *pDst = NULL;

    if(!pSrc) return NULL;

    vLen = (int)strlen(pSrc);
    pDst = MyMalloc(vLen+1);
    memcpy(pDst, pSrc, vLen);

    return pDst;
}


char * getMyMacAddress(void)
{
    return CopyString(gxNICInfo[0].pMacAddr);
}

// Utilities of Network
char * getMyIpString(char *pIfName)
{
    int i = 0;
    if(pIfName==NULL)
        return NULL;

    for(i=0; i<NET_MAX_INTERFACE; i++) {
        if(strncmp(gxNICInfo[i].pIfName, pIfName, strlen(pIfName))==0) {
            return CopyString(gxNICInfo[i].pLocalAddr);
        }
    }
    return NULL;
}


char * initMyIpString(void)
{
    int vInterfaceCount=0;
    struct ifaddrs * ifAddrStruct=NULL;
    struct ifaddrs * ifa=NULL;
    void * tmpAddrPtr=NULL;

    memset(gxNICInfo, 0, sizeof(gxNICInfo));
    getifaddrs(&ifAddrStruct);
    gLocalInterfaceCount = 0;
    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa ->ifa_addr->sa_family==AF_INET) {
            // check it is IP4
            // is a valid IP4 Address
            char addressBuffer[INET_ADDRSTRLEN];
            tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;

            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            DBG("%s IP Address %s ", ifa->ifa_name, addressBuffer);
            unsigned int vIP = ntohl(((struct in_addr *)tmpAddrPtr)->s_addr);
            if(isPrivateV4(vIP)==1) {
                DBG("(Private %u) \n",vIP);
            } else {
                DBG("(Public %u) \n", vIP);
            }
            // Note: you may set local address for different interface. For example:eth0, eth1
            memcpy(gxNICInfo[vInterfaceCount].pLocalAddr, addressBuffer, strlen(addressBuffer));
            memcpy(gxNICInfo[vInterfaceCount].pIfName, ifa->ifa_name, strlen(ifa->ifa_name));

            // TODO: We cannot not get MAC Address of all network interface
            // Only MAC Address of the interface binded to default routing can be obtained.
#ifdef __APPLE__
            // Reference https://gist.github.com/Coeur/1409855
            {
                int                 mgmtInfoBase[6];
                char                *msgBuffer = NULL;
                char                *errorFlag = NULL;
                size_t              length;

                // Setup the management Information Base (mib)
                mgmtInfoBase[0] = CTL_NET;        // Request network subsystem
                mgmtInfoBase[1] = AF_ROUTE;       // Routing table info
                mgmtInfoBase[2] = 0;
                mgmtInfoBase[3] = AF_LINK;        // Request link layer information
                mgmtInfoBase[4] = NET_RT_IFLIST;  // Request all configured interfaces


                // The interface name should be decided runtime
                int sock;
                struct ifreq ifr;

                sock = socket(AF_INET, SOCK_DGRAM, 0);
                ifr.ifr_addr.sa_family = AF_INET;

                strncpy(ifr.ifr_name, ifa->ifa_name, strlen(ifa->ifa_name));
                close(sock);
                
                // With all configured interfaces requested, get handle index
                //if ((mgmtInfoBase[5] = if_nametoindex("en0")) == 0)
                if ((mgmtInfoBase[5] = if_nametoindex(ifr.ifr_name)) == 0)
                    errorFlag = "if_nametoindex failure";
                // Get the size of the data available (store in len)
                else if (sysctl(mgmtInfoBase, 6, NULL, &length, NULL, 0) < 0)
                    errorFlag = "sysctl mgmtInfoBase failure";
                // Alloc memory based on above call
                else if ((msgBuffer = malloc(length)) == NULL)
                    errorFlag = "buffer allocation failure";
                // Get system information, store in buffer
                else if (sysctl(mgmtInfoBase, 6, msgBuffer, &length, NULL, 0) < 0) {
                    free(msgBuffer);
                    errorFlag = "sysctl msgBuffer failure";
                } else {
                    // Map msgbuffer to interface message structure
                    struct if_msghdr *interfaceMsgStruct = (struct if_msghdr *) msgBuffer;

                    // Map to link-level socket structure
                    struct sockaddr_dl *socketStruct = (struct sockaddr_dl *) (interfaceMsgStruct + 1);

                    // Copy link layer address data in socket structure to an array
                    unsigned char macAddress[6]= {0};
                    memcpy(&macAddress, socketStruct->sdl_data + socketStruct->sdl_nlen, 6);

                    // Read from char array into a string object, into traditional Mac address format
                    sprintf(gxNICInfo[vInterfaceCount].pMacAddr, "%.2x%.2x%.2x%.2x%.2x%.2x",
                            (unsigned char)macAddress[0],
                            (unsigned char)macAddress[1],
                            (unsigned char)macAddress[2],
                            (unsigned char)macAddress[3],
                            (unsigned char)macAddress[4],
                            (unsigned char)macAddress[5]);
                    DBG("MAC %s\n", gxNICInfo[vInterfaceCount].pMacAddr);

                    // Release the buffer memory
                    free(msgBuffer);
                }

                if(errorFlag) printf("err = %s\n", errorFlag);
            }
#else
            {

                // For linux system
                int sock;
                struct ifreq ifr;

                sock = socket(AF_INET, SOCK_DGRAM, 0);
                ifr.ifr_addr.sa_family = AF_INET;

                strncpy(ifr.ifr_name, ifa->ifa_name, strlen(ifa->ifa_name));

                ioctl(sock, SIOCGIFHWADDR, &ifr);

                close(sock);

                sprintf(gxNICInfo[vInterfaceCount].pMacAddr, "%.2x%.2x%.2x%.2x%.2x%.2x",
                        (unsigned char)ifr.ifr_hwaddr.sa_data[0],
                        (unsigned char)ifr.ifr_hwaddr.sa_data[1],
                        (unsigned char)ifr.ifr_hwaddr.sa_data[2],
                        (unsigned char)ifr.ifr_hwaddr.sa_data[3],
                        (unsigned char)ifr.ifr_hwaddr.sa_data[4],
                        (unsigned char)ifr.ifr_hwaddr.sa_data[5]);
                DBG("MAC %s\n", gxNICInfo[vInterfaceCount].pMacAddr);
            }
#endif

            vInterfaceCount++;
            gLocalInterfaceCount++;
        } else if (ifa->ifa_addr->sa_family==AF_INET6) {
            // check it is IP6
            // is a valid IP6 Address
            tmpAddrPtr=&((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
            char addressBuffer[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
            //DBG("%s IP Address %s\n", ifa->ifa_name, addressBuffer);
        }
    }

    if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);
    return gxNICInfo[0].pLocalAddr;
}

int CreateUnicastClient(struct sockaddr_in *pSockAddr)
{
    // http://www.tenouk.com/Module41c.html
    int sd=-1;

    struct timeval timeout;
    timeout.tv_sec  = 10;
    timeout.tv_usec = 0;

    if(pSockAddr==NULL) {
        return ERR_ARGUMENT_FAIL;
    }

    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sd < 0) {
        perror("Opening datagram socket error");
        return ERR_SOCKET_FAIL;
    }
    //else
    //   DBG("Opening the datagram socket %d...OK.\n", sd);

    if(setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
        DBG("setsockopt...error.\n");
        return ERR_SOCKET_FAIL;
    }

    if(bind(sd, (struct sockaddr*)pSockAddr, sizeof(*pSockAddr))) {
        perror("Binding datagram socket error");
        close(sd);
        return ERR_BIND_FAIL;
    }
    DBG("Create unicast client socket %d at port %d...OK.\n",sd, ntohs(pSockAddr->sin_port));
    return sd;
}

// if *pPort <=0, let system select port for us
int CreateUnicastServer(char *pAddress, int *pPort)
{
    int sd;
    int vPort;
    struct sockaddr_in localSock;
    socklen_t vSockLen;

    if(!pAddress)
        return ERR_ARGUMENT_FAIL;
    if(strlen(pAddress)==0)
        return ERR_ARGUMENT_FAIL;
    if(!pPort)
        return ERR_ARGUMENT_FAIL;

    vPort = *pPort;
    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sd < 0) {
        perror("Opening datagram socket error");
        return ERR_SOCKET_FAIL;
    }
    //else
    //   DBG("Opening unicast server socket %d for ip %s...OK.\n",sd, pAddress);

    int opt = 1;
    if(setsockopt(sd, IPPROTO_IP, IP_PKTINFO, &opt, sizeof(opt)) < 0) {
        perror("Setting IP_PKTINFO error");
        return ERR_SOCKET_FAIL;
    }

    if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) {
        perror("Setting SO_REUSEADDR error");
        close(sd);
        return ERR_SOCKET_FAIL;
    }
    /*
       // For Mac only
       if(setsockopt(sd, SOL_SOCKET, SO_REUSEPORT, (char *)&opt, sizeof(opt)) < 0)
       {
          perror("Setting SO_REUSEADDR error");
          close(sd);
          exit(1);
       }
    */
    if(vPort>0) {
        memset((char *) &localSock, 0, sizeof(localSock));
        localSock.sin_family = AF_INET;
        localSock.sin_port = htons(vPort);
        localSock.sin_addr.s_addr = inet_addr(pAddress);

        if(bind(sd, (struct sockaddr*)&localSock, sizeof(localSock))) {
            perror("Binding datagram socket error");
            close(sd);
            return ERR_BIND_FAIL;
        }
        //else
        //   DBG("Binding datagram socket...OK.\n");
    } else {
        memset((char *) &localSock, 0, sizeof(localSock));
        localSock.sin_family = AF_INET;
        localSock.sin_port = 0;  // let system select port
        localSock.sin_addr.s_addr = inet_addr(pAddress);

        if(bind(sd, (struct sockaddr*)&localSock, sizeof(localSock))) {
            perror("Binding datagram socket error");
            close(sd);
            return ERR_BIND_FAIL;
        }


        vSockLen = sizeof(localSock);
        if(getsockname(sd, (struct sockaddr*)&localSock, &vSockLen)) {
            perror("getsockname error");
            return ERR_SOCKET_FAIL;
        }
        vPort = ntohs(localSock.sin_port);
        *pPort = vPort;

    }

    DBG("Create unicast server socket %d at %s:%d...OK.\n",sd, pAddress,vPort);
    return sd;
}

// Random function
void InitMyRandom(char *myipaddr)
{
    unsigned int ourAddress;
    struct timeval timeNow;

    ourAddress = ntohl(inet_addr(myipaddr));
    gettimeofday(&timeNow, NULL);

    unsigned int seed = (unsigned int)(ourAddress^timeNow.tv_sec^timeNow.tv_usec);

    srandom(seed);
}

long our_random()
{
    return random();
}

unsigned int our_random16()
{
    long random1 = our_random();
    return (unsigned int)(random1&0xffff);
}


unsigned int our_random32()
{
    long random1 = our_random();
    long random2 = our_random();

    return (unsigned int)((random2<<31) | random1);
}

void UuidGen(char *uuidbuf)
{
    sprintf(uuidbuf, "%08x-%04x-%04x-%04x-%08x%04x",our_random32(), our_random16(),our_random16(),our_random16(),our_random32(), our_random16());
}


void noprintf(char *format, ...)
{
    ;
}


char* getNextLine(char* inputLine)
{
    char *nextLine = NULL;
    // Begin by finding the start of the next line (if any):
    char *ptr=NULL;
    for (ptr = inputLine; *ptr != '\0'; ++ptr) {
        if (*ptr == '\r' || *ptr == '\n') {
            // We found the end of the line
            ++ptr;
            while (*ptr == '\r' || *ptr == '\n') ++ptr;
            nextLine = ptr;
            if (nextLine[0] == '\0') nextLine = NULL; // special case for end
            break;
        }
    }

    return nextLine;
}

// vSendSrcAdd, "192.168.", "8"
int checkIPInTheNetwork( char *pTarIP, char *pNetIP, char *pNetMask)
{
    int vRet = 1;

    unsigned int vNetIP = *((unsigned int *) pNetIP);
    unsigned int vNetMask = *((unsigned int *) pNetMask);

    unsigned int vTarIP = inet_addr(pTarIP);

    if((vNetIP & vNetMask) == (vTarIP & vNetMask)) {
        vRet = 0;
    }
    return vRet ;
}

int isPrivateV4(unsigned int ip_in_host_order)
{
    return ((ip_in_host_order >> 24) == 127) ||
           ((ip_in_host_order >> 24) == 10) ||
           ((ip_in_host_order >> 20) == ((172<<4) | 1)) ||
           ((ip_in_host_order >> 16) == ((192<<8) | 168)) ||
           ((ip_in_host_order >> 16) == ((168<<8) | 254));
}

// Parse communication info between punching and coordinator
/*
 Define the structure in gpSendBuffer
 For request
 i=information
 c=connection description
 u=user identifier
 p=peer identifier user want to connect, maybe null
 
 For response
 s=session identifier
 c=connection description
 */

int parseUserData(char *pIn, tSessionInfo *pSessionInfo)
{
    char *c = pIn;
    if(!pIn)
        return -1;
    
    memset(pSessionInfo, 0, sizeof(tSessionInfo));
    
    while(c) {
        if( !strncmp(c,"i=",2) ) {
            sscanf(c,"i=%s", pSessionInfo->pInfo);
        }
        
        if( !strncmp(c,"c=",2) ) {
            sscanf(c,"c=%s %d", pSessionInfo->pConnAddr[pSessionInfo->ConnCount], &pSessionInfo->ConnPort[pSessionInfo->ConnCount]);
            pSessionInfo->ConnCount++;
        }
        
        if( !strncmp(c,"u=",2) ) {
            sscanf(c,"u=%s", pSessionInfo->pUserId);
        }
        
        if( !strncmp(c,"p=",2) ) {
            sscanf(c,"p=%s", pSessionInfo->pPeerId);
        }
        
        if( !strncmp(c,"s=",2) ) {
            sscanf(c,"s=%s", pSessionInfo->pSessionId);
        }
        c = getNextLine(c);
    }
    return 1;
}