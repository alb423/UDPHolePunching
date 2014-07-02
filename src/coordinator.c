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


typedef struct tMapping{
   int  SrcPort;
   char pSrcAddr[32];  
   int  PrivateSrcPort;
   char pPrivateSrcAddr[32];
}tMapping;


typedef struct tClient{
   int   MappingCount;
   tMapping xMapping[NET_MAX_INTERFACE];   
}tClient;

int coordinator(int argc, char **argv)
{
   int vServerSocket = 0, i=0, j=0, vLen=0;
   int vReciveLen=0, vResult=0, vPrivatePort=0;
   char pPrivateAddress[32]={0};
   char pReceiveData[1024]={0};
   char pSendBuffer[SEND_BUF_LEN]={0};
   
   char *pMyAddress=NULL;
               
   int vListenPort = 0, vClientCnt=0;
   
   tClient  vxClient[2];
   
   if(argc!=2)
   {
     printf("Usage: coordinator interface\n");
     return -1;
   }    
   
   initMyIpString();
             
   vListenPort = COORDINATE_PORT;             
   pMyAddress = getMyIpString(argv[1]); // should be eth1 or eth0
   memset(pSendBuffer, 0, SEND_BUF_LEN);
   sprintf(pSendBuffer, "%s %d", pMyAddress, vListenPort);
   InitMyRandom(pMyAddress);

   vServerSocket = CreateUnicastServer(pMyAddress, vListenPort);
   if(vServerSocket<0)
   {
      printf("%s:%d, vServerSocket=%d\n", pMyAddress, vListenPort, vServerSocket);
      return -1;
   }
   
   while(1)
   {     
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

      if(vResult < 0)
      {
         printf("unknow error\n");
         break;
      }
      else if(vResult == 0)
      {
         printf("timeout\n");
         break;
      }
             
      if (!FD_ISSET(vServerSocket, &readset)) 
      {
         printf("receive unexpected data\n");
         break;
      }
      
      vReciveLen = recvmsg(vServerSocket, &mh, 0);
      //DBG("set msg_namelen=%d msg_iovlen=%d msg_controllen=%d, vReciveLen=%d\n", mh.msg_namelen, mh.msg_iovlen, mh.msg_controllen,vReciveLen);
      
      struct cmsghdr *cmsg = NULL;
      struct in_pktinfo *pi = NULL;
      for(cmsg = CMSG_FIRSTHDR(&mh) ;
          cmsg != NULL;
          cmsg = CMSG_NXTHDR(&mh, cmsg))
      {
         if(cmsg->cmsg_level != IPPROTO_IP || cmsg->cmsg_type != IP_PKTINFO)
         {
            printf("cmsg->cmsg_level=%d\n", cmsg->cmsg_level);
            continue;
         }        
         else if(cmsg->cmsg_type == IP_PKTINFO)
         {
            pi = (struct in_pktinfo *)CMSG_DATA(cmsg); 
            if(pi)
            {
               char *pTmp, pSrc[32]={0}, pDst[32]={0};
                  
               // inet_ntoa() use a global buffer to store the string,
               // so we need to copy the value before we invoke inet_ntoa() next time        
               pTmp = inet_ntoa(pi->ipi_addr);
               if(pTmp)
                  memcpy(pDst, pTmp, strlen(pTmp));

               pTmp = inet_ntoa(localaddr.sin_addr);
               if(pTmp)
                  memcpy(pSrc, pTmp, strlen(pTmp));
                
               //DBG("%s %s :%d\n",__FILE__,__func__, __LINE__);
               DBG("receive socket nIndex=%d packet from %s:%d to %s\n", pi->ipi_ifindex, pSrc, ntohs(localaddr.sin_port), pDst);
               if(vReciveLen<1024)
               {
                  char *pTmp;
                  memset(pReceiveData, 0, 1024);
                  memcpy(pReceiveData, iovbuf, vReciveLen);
                  DBG("receive data :\n%s\n", pReceiveData);
                  
                  pTmp = pReceiveData;
                  //memset(&vxClient[vClientCnt], 0, sizeof(tClient));
                  
                  // colect port mapping relation     
                  for(i=0;i<NET_MAX_INTERFACE;i++)
                  {                                          
                     if(pTmp!=NULL)
                     {
                        //fprintf(stderr,"pTmp=\n%s\n", pTmp);
                        
                        vPrivatePort=0;
                        memset(pPrivateAddress, 0, 32);                        
                        sscanf(pTmp,"%s %d", pPrivateAddress, &vPrivatePort);
                        
                        vxClient[vClientCnt].xMapping[i].SrcPort = ntohs(localaddr.sin_port);                  
                        memcpy(vxClient[vClientCnt].xMapping[i].pSrcAddr, pSrc, 32);
                        
                        vxClient[vClientCnt].xMapping[i].PrivateSrcPort = vPrivatePort;                        
                        memcpy(vxClient[vClientCnt].xMapping[i].pPrivateSrcAddr, pPrivateAddress, 32);
                        
                        vxClient[vClientCnt].MappingCount ++; 
                        
                        pTmp = strstr(pTmp, "\n");
                        if(pTmp!=NULL) 
                        {
                           // omit "\n"
                           pTmp = pTmp + strlen("\n");
                           if(strlen(pTmp)==0)
                              break;
                        }
                     }
                  }
                 
                  
                  fprintf(stderr,"Generate mapping relation for client_%d is\n", vClientCnt);
                  for(i=0;i<vxClient[vClientCnt].MappingCount;i++)
                  {
                     fprintf(stderr,"\"%s %d\" to \"%s %d\"\n",\
                        vxClient[vClientCnt].xMapping[i].pSrcAddr, vxClient[vClientCnt].xMapping[i].SrcPort, \
                        vxClient[vClientCnt].xMapping[i].pPrivateSrcAddr, vxClient[vClientCnt].xMapping[i].PrivateSrcPort);                  
                  }
                  fprintf(stderr,"\n");
               }                     
               
               vClientCnt++;

                if(vClientCnt==2)
                {
                  // send port mapping relatiion back
                  for(i=0;i<2;i++)
                  {
                     struct sockaddr_in vSockAddr;
                     memset((char *) &vSockAddr, 0, sizeof(vSockAddr));
                     vSockAddr.sin_family = AF_INET;
                     vSockAddr.sin_addr.s_addr = inet_addr(vxClient[i].xMapping[0].pSrcAddr);
                     vSockAddr.sin_port = htons(vxClient[i].xMapping[0].SrcPort);  
    
                     memset(pSendBuffer, 0, SEND_BUF_LEN);
                     if(i==0)
                     {
                        vLen = 0;
                        sprintf(pSendBuffer, "%s %d\n", vxClient[1].xMapping[0].pSrcAddr, vxClient[1].xMapping[0].SrcPort);
                        vLen = strlen(pSendBuffer);   
                        // The 1st item is used to store public address and pot                     
                        for(j=0;j<vxClient[1].MappingCount;j++)
                        {                        
                           sprintf(pSendBuffer+vLen, "%s %d\n", vxClient[1].xMapping[j].pPrivateSrcAddr, vxClient[1].xMapping[j].PrivateSrcPort);
                           vLen = strlen(pSendBuffer);
                        }
                        
                        fprintf(stderr,"send below data to %s\n%s\n",vxClient[0].xMapping[0].pSrcAddr, pSendBuffer);
                     }
                     else if(i==1)
                     {
                        vLen = 0;
                        sprintf(pSendBuffer, "%s %d\n", vxClient[0].xMapping[0].pSrcAddr, vxClient[0].xMapping[0].SrcPort);
                        vLen = strlen(pSendBuffer);                        
                        for(j=0;j<vxClient[0].MappingCount;j++)
                        {                               
                           sprintf(pSendBuffer+vLen, "%s %d\n", vxClient[0].xMapping[j].pPrivateSrcAddr, vxClient[0].xMapping[j].PrivateSrcPort);
                           vLen = strlen(pSendBuffer);
                        }
                        fprintf(stderr,"send below data to %s\n%s\n",vxClient[1].xMapping[0].pSrcAddr, pSendBuffer);
                     }
                                             
                     if(sendto(vServerSocket, pSendBuffer, strlen(pSendBuffer), 0, (struct sockaddr*)&vSockAddr, sizeof(vSockAddr)) < 0)
                     {
                        fprintf (stderr, "Cannot send data to client %s:%d !!\n", vxClient[i].xMapping[0].pSrcAddr, vxClient[i].xMapping[0].SrcPort);
                     }
                  }   
                  vClientCnt = 0;
                  memset(&vxClient, 0, sizeof(tClient)*2);                                  
                }
            }    
            break;
         }
         else if(cmsg->cmsg_type == IPPROTO_IP)
         {
            DBG("%s %s :%d\n",__FILE__,__func__, __LINE__);
         }
      }
      //DBG("%s %s :%d\n",__FILE__,__func__, __LINE__);
   }

   DBG("END TEST\n");
      

   close(vServerSocket);   
   return 1;
}
