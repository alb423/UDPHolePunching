#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>


#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <unistd.h>

#include "util.h"
#include "sqlite3_util.h"

typedef struct tMapping {
    int  SrcPort;
    char pSrcAddr[32];
    int  PrivateSrcPort;
    char pPrivateSrcAddr[32];
} tMapping;


typedef struct tClient {
    int   MappingCount;
    tMapping xMapping[NET_MAX_INTERFACE];
} tClient;

int coordinator(char *pIfName)
{
    int vServerSocket = 0, i=0, j=0, vLen=0;
    int vReciveLen=0, vResult=0, vPrivatePort=0;
    char pPrivateAddress[32]= {0};
    char pReceiveData[1024]= {0};
    char pSendBuffer[SEND_BUF_LEN]= {0};

    char *pMyAddress=NULL;

    int vListenPort = 0, vClientCnt=0, vSessionId=0;

    int result=0;
    char *pDBPath=DBPATH;

    tClient  vxClient;
    tClient  vxPeerClient;
    memset(&vxClient, 0, sizeof(tClient));
    memset(&vxPeerClient, 0, sizeof(tClient));

    initMyIpString();

    // TODO: The database may not need to be removed
    remove(pDBPath);

    result = DB_Init(pDBPath);
    if(result<0) printf("DB_Init error!!\n");


    vListenPort = COORDINATE_PORT;
    pMyAddress = getMyIpString(pIfName); // should be eth1 or eth0
    memset(pSendBuffer, 0, SEND_BUF_LEN);
    sprintf(pSendBuffer, "%s %d", pMyAddress, vListenPort);
    InitMyRandom(pMyAddress);

    vServerSocket = CreateUnicastServer(pMyAddress, &vListenPort);
    if(vServerSocket<0) {
        printf("%s:%d, vServerSocket=%d\n", pMyAddress, vListenPort, vServerSocket);
        return -1;
    }

    while(1) {
        struct sockaddr_in vxSockAddrFrom;
        memset((void*)&vxSockAddrFrom, 0, sizeof(vxSockAddrFrom));

        // Reference www.cnblogs.com/kissazi2/p/3158603.html
        char cmbuf[1024], iovbuf[1024*8];
        struct sockaddr_in localaddr;
        struct iovec iov[1];

        localaddr.sin_family = AF_INET;
        localaddr.sin_addr.s_addr = inet_addr(pMyAddress);
        iov[0].iov_base = iovbuf;
        iov[0].iov_len = sizeof(iovbuf);

        struct msghdr mh = {
            .msg_name = &localaddr,
            .msg_namelen = sizeof(localaddr),
            .msg_control = cmbuf,
            .msg_controllen = sizeof(cmbuf),
            .msg_iov = iov,
            .msg_iovlen = 1,
        };

        struct timeval tv;
        fd_set readset;
        tv.tv_sec = 300;
        tv.tv_usec = 0;
        //do {
        FD_ZERO(&readset);
        FD_SET(vServerSocket, &readset);
        vResult = select(vServerSocket + 1, &readset, NULL, NULL, &tv);
        //} while (vResult == -1);// && errno == EINTR);

        if(vResult < 0) {
            printf("unknow error\n");
            break;
        } else if(vResult == 0) {
            printf("timeout\n");
            break;
        }

        if (!FD_ISSET(vServerSocket, &readset)) {
            printf("receive unexpected data\n");
            break;
        }

        vReciveLen = recvmsg(vServerSocket, &mh, 0);
        //DBG("set msg_namelen=%d msg_iovlen=%d msg_controllen=%d, vReciveLen=%d\n", mh.msg_namelen, mh.msg_iovlen, mh.msg_controllen,vReciveLen);

        struct cmsghdr *cmsg = NULL;
        struct in_pktinfo *pi = NULL;
        for(cmsg = CMSG_FIRSTHDR(&mh) ;
                cmsg != NULL;
                cmsg = CMSG_NXTHDR(&mh, cmsg)) {
            if(cmsg->cmsg_level != IPPROTO_IP || cmsg->cmsg_type != IP_PKTINFO) {
                printf("cmsg->cmsg_level=%d\n", cmsg->cmsg_level);
                continue;
            } else if(cmsg->cmsg_type == IP_PKTINFO) {
                pi = (struct in_pktinfo *)CMSG_DATA(cmsg);
                if(pi) {
                    char *pTmpAddr, pSrc[32]= {0}, pDst[32]= {0}, pActor[32]= {0}, pUserId[32]= {0}, pPeerId[32]= {0};
                    char pSessionId[32]= {0}, pSrcPort[32]= {0}, pPrivateSrcPort[32]= {0};

                    // inet_ntoa() use a global buffer to store the string,
                    // so we need to copy the value before we invoke inet_ntoa() next time
                    pTmpAddr = inet_ntoa(pi->ipi_addr);
                    if(pTmpAddr)
                        memcpy(pDst, pTmpAddr, strlen(pTmpAddr));

                    pTmpAddr = inet_ntoa(localaddr.sin_addr);
                    if(pTmpAddr)
                        memcpy(pSrc, pTmpAddr, strlen(pTmpAddr));

                    memset(&vxClient, 0, sizeof(tClient));

                    DBG("\n<====\nreceive socket ipi_ifindex=%d packet from %s to %s:%d\n", pi->ipi_ifindex, pDst, pSrc, ntohs(localaddr.sin_port));
                    if(vReciveLen<1024) {
                        
                        tSessionInfo vxSessionInfo;
                        memset(&vxSessionInfo, 0, sizeof(tSessionInfo));
                        memset(pReceiveData, 0, 1024);
                        memcpy(pReceiveData, iovbuf, vReciveLen);
                        DBG("receive data :\n%s\n", pReceiveData);
                        
                        parseUserData(pReceiveData, &vxSessionInfo);
                        /*
                        printf("vxSessionInfo.pInfo=%s\n", vxSessionInfo.pInfo);
                        printf("vxSessionInfo.pUserId=%s\n", vxSessionInfo.pUserId);
                        printf("vxSessionInfo.pPeerId=%s\n", vxSessionInfo.pPeerId);
                        printf("vxSessionInfo.pSessionId=%s\n", vxSessionInfo.pSessionId);
                        for(j = 0; j < vxSessionInfo.ConnCount; j++ ) {
                            printf("vxSessionInfo.pConn=%s %d\n", vxSessionInfo.pConnAddr[j], vxSessionInfo.ConnPort[j]);
                        }
                        */
                        
                        strcpy(pActor, vxSessionInfo.pInfo);
                        strcpy(pUserId, vxSessionInfo.pUserId);
                        strcpy(pPeerId, vxSessionInfo.pPeerId);
                        
                        vSessionId = DB_SESSION_InsertNew(pDBPath, pUserId, pPeerId);
                        
                        // colect port mapping relation
                        for(i=0; i<vxSessionInfo.ConnCount; i++) {

                            vPrivatePort=0;
                            memset(pPrivateAddress, 0, 32);
                            strcpy(pPrivateAddress, vxSessionInfo.pConnAddr[i]);
                            vPrivatePort = vxSessionInfo.ConnPort[i];

                            vxClient.xMapping[i].SrcPort = ntohs(localaddr.sin_port);
                            memcpy(vxClient.xMapping[i].pSrcAddr, pSrc, 32);

                            vxClient.xMapping[i].PrivateSrcPort = vPrivatePort;
                            memcpy(vxClient.xMapping[i].pPrivateSrcAddr, pPrivateAddress, 32);

                            vxClient.MappingCount ++;
                        }

                        fprintf(stderr,"Generate mapping relation for client_%d is\n", vClientCnt);
                        for(i=0; i<vxClient.MappingCount; i++) {
                            char pEPort[8]= {0}, pIPort[8]= {0};
                            sprintf(pEPort, "%d", vxClient.xMapping[i].SrcPort);
                            sprintf(pIPort, "%d", vxClient.xMapping[i].PrivateSrcPort);

                            // store relation into a small database, for example: sqllite
                            DB_MAPPING_InsertNew(pDBPath, pUserId, \
                                                 vxClient.xMapping[i].pSrcAddr, pEPort, \
                                                 vxClient.xMapping[i].pPrivateSrcAddr,pIPort);

                            fprintf(stderr,"\"%s %d\" to \"%s %d\"\n",\
                                    vxClient.xMapping[i].pSrcAddr, vxClient.xMapping[i].SrcPort, \
                                    vxClient.xMapping[i].pPrivateSrcAddr, vxClient.xMapping[i].PrivateSrcPort);

                        }
                        fprintf(stderr,"\n");
                    }
                    else {
                        fprintf(stderr,"The length of receive data is too long\n");
                    }


                    
                    // Query connection info from database to find if peerId is already connected
                    vResult = DB_MAPPING_GetDataByUserId((char *)pDBPath, (char *)pPeerId, (char *)vxPeerClient.xMapping[0].pSrcAddr, (char *)pSrcPort, (char *)vxPeerClient.xMapping[0].pPrivateSrcAddr, (char *)pPrivateSrcPort);
                    printf("query pPeerId %s, result:%d\n", pPeerId, vResult);

                    // TODO: If we get more than one Mapping, what should we do?
                    // Send back only when both side is connected (the first connected client maybe offline)
                    if(vResult>0) {
                        vxPeerClient.MappingCount = 1;
                        vxPeerClient.xMapping[0].SrcPort = atoi(pSrcPort);
                        vxPeerClient.xMapping[0].PrivateSrcPort = atoi(pPrivateSrcPort);

                        // send port mapping relation back
                        struct sockaddr_in vSockAddr;
                        memset((char *) &vSockAddr, 0, sizeof(vSockAddr));
                        vSockAddr.sin_family = AF_INET;
                        vSockAddr.sin_addr.s_addr = inet_addr(vxClient.xMapping[0].pSrcAddr);
                        vSockAddr.sin_port = htons(vxClient.xMapping[0].SrcPort);

                        DB_SESSION_GetDataByUserId(pDBPath, pSessionId, pUserId, pPeerId);
                        memset(pSendBuffer, 0, SEND_BUF_LEN);
                        vLen = 0;
                        sprintf(pSendBuffer, "s=%d\n", atoi(pSessionId));
                        vLen = strlen(pSendBuffer);
                        sprintf(pSendBuffer+vLen, "c=%s %d\n", vxClient.xMapping[0].pSrcAddr, vxClient.xMapping[0].SrcPort);
                        vLen = strlen(pSendBuffer);
                        for(j=0; j<vxClient.MappingCount; j++) {
                            sprintf(pSendBuffer+vLen, "c=%s %d\n", vxClient.xMapping[j].pPrivateSrcAddr, vxClient.xMapping[j].PrivateSrcPort);
                            vLen = strlen(pSendBuffer);
                        }

                        DBG("send below data to %s:%d\n%s\n",vxClient.xMapping[0].pSrcAddr, vxClient.xMapping[0].SrcPort, pSendBuffer);
                        if(sendto(vServerSocket, pSendBuffer, strlen(pSendBuffer), 0, (struct sockaddr*)&vSockAddr, sizeof(vSockAddr)) < 0) {
                            fprintf (stderr, "Cannot send data to client %s:%d !!\n", vxClient.xMapping[0].pSrcAddr, vxClient.xMapping[0].SrcPort);
                        }

                        DB_SESSION_GetDataByUserId(pDBPath, pSessionId, pPeerId, pUserId);
                        memset((char *) &vSockAddr, 0, sizeof(vSockAddr));
                        vSockAddr.sin_family = AF_INET;
                        vSockAddr.sin_addr.s_addr = inet_addr(vxPeerClient.xMapping[0].pSrcAddr);
                        vSockAddr.sin_port = htons(vxPeerClient.xMapping[0].SrcPort);

                        memset(pSendBuffer, 0, SEND_BUF_LEN);
                        vLen = 0;
                        sprintf(pSendBuffer, "s=%d\n", atoi(pSessionId));
                        vLen = strlen(pSendBuffer);
                        sprintf(pSendBuffer+vLen, "c=%s %d\n", vxPeerClient.xMapping[0].pSrcAddr, vxPeerClient.xMapping[0].SrcPort);
                        vLen = strlen(pSendBuffer);
                        for(j=0; j<vxPeerClient.MappingCount; j++) {
                            sprintf(pSendBuffer+vLen, "c=%s %d\n", vxPeerClient.xMapping[j].pPrivateSrcAddr, vxPeerClient.xMapping[j].PrivateSrcPort);
                            vLen = strlen(pSendBuffer);
                        }

                        DBG("send below data to %s:%d\n%s\n",vxPeerClient.xMapping[0].pSrcAddr, vxPeerClient.xMapping[0].SrcPort,pSendBuffer);
                        if(sendto(vServerSocket, pSendBuffer, strlen(pSendBuffer), 0, (struct sockaddr*)&vSockAddr, sizeof(vSockAddr)) < 0) {
                            fprintf (stderr, "Cannot send data to client %s:%d !!\n", vxPeerClient.xMapping[0].pSrcAddr, vxPeerClient.xMapping[0].SrcPort);
                        }

                        // TODO: the time to delete mapping relation may need change
                        DB_MAPPING_DeleteByUserId(pDBPath, pUserId);
                        DB_MAPPING_DeleteByUserId(pDBPath, pPeerId);
                    }
                }
                break;
            } else if(cmsg->cmsg_type == IPPROTO_IP) {
                DBG("%s %s :%d\n",__FILE__,__func__, __LINE__);
            }
        }
        //DBG("%s %s :%d\n",__FILE__,__func__, __LINE__);
    }

    DBG("END TEST\n");


    close(vServerSocket);
    return 1;
}
