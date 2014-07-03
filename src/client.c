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

#include <pthread.h>
#include <sys/msg.h>

#include "util.h"
                       
typedef struct {
   pthread_t thread_tid;
   long thread_count;
}tThread;
tThread gpThreadPtr[2];

int   gThreadRet=0, gThreadNo=0;
int   gClientSocket = 0;
struct sockaddr_in gxPeerSockAddr[NET_MAX_INTERFACE];
char gpSendBuffer[SEND_BUF_LEN]={0};

void PunchingThread(void* data);

int punching(char *pIfName, int vPort, char *pRendezvousServerAddress, char * pPeerAddress, int *pPeerPort)
{
   int i=0, vLen=0, vReciveLen=0, vResult=0, vPeerPort=0;
   int vServerSocket=0, vPrivatePort=0;
   int bPunchingSuccess = 0;
   char pReceiveData[1024]={0};
   char pAddress[32]={0};
   char *pMyAddress=NULL;

   memset((char *) &gxPeerSockAddr, 0, sizeof(struct sockaddr_in)*4);
   initMyIpString();
   
   // TODO: the interface name should be assigned by user
   
   pMyAddress = getMyIpString(pIfName);
   memset(gpSendBuffer, 0, SEND_BUF_LEN);
   
   //sprintf(gpSendBuffer, "aid=APPDeviceToken\n");
   //vLen = strlen(gpSendBuffer);
   //sprintf(gpSendBuffer+vLen, "cid=CamearID\n");   
   for(i=0;i<gLocalInterfaceCount;i++)
   {
      vLen = strlen(gpSendBuffer);
      sprintf(gpSendBuffer+vLen, "%s %d\n", gxNICInfo[i].pLocalAddr, vPort);
   }
   fprintf(stderr, "gpSendBuffer=\n%s\n",gpSendBuffer);
   InitMyRandom(pMyAddress);

   
   vServerSocket = CreateUnicastServer(pMyAddress, vPort);
   gClientSocket = vServerSocket;

   
   // send message to coordinator
   {
      struct sockaddr_in vSockAddr;
      memset((char *) &vSockAddr, 0, sizeof(vSockAddr));
      vSockAddr.sin_family = AF_INET;
      vSockAddr.sin_addr.s_addr = inet_addr(pRendezvousServerAddress);
      vSockAddr.sin_port = htons((COORDINATE_PORT));      
      if(sendto(gClientSocket, gpSendBuffer, strlen(gpSendBuffer), 0, (struct sockaddr*)&vSockAddr, sizeof(vSockAddr)) < 0)
      {
         fprintf (stderr, "Cannot send data to coordinator %s!\n", gpSendBuffer);
      }
   }
      
   // wait for port mapping notify message from coordinator
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
      tv.tv_sec = 10;
      tv.tv_usec = 0;
      //do {
         FD_ZERO(&readset);
         FD_SET(vServerSocket, &readset);
         vResult = select(vServerSocket + 1, &readset, NULL, NULL, &tv);
      //} while (vResult == -1);// && errno == EINTR);

      if(vResult < 0)
      {
         printf("unknow error\n");
         exit(0);
      }
      else if(vResult == 0)
      {
         printf("timeout\n");
         exit(0);
      }
             
      if (!FD_ISSET(vServerSocket, &readset)) 
      {
         printf("receive unexpected data\n");
         exit(0);
      }
      
      vReciveLen = recvmsg(vServerSocket, &mh, 0);
      //DBG("set msg_namelen=%d msg_iovlen=%d msg_controllen=%d, vReciveLen=%d\n", mh.msg_namelen, mh.msg_iovlen, mh.msg_controllen, vReciveLen);
      
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
                
               DBG("receive packet from %s:%d to %s\n", pSrc, ntohs(localaddr.sin_port), pDst);
               
               if(vReciveLen<1024)
               {
                  char *pTmp;
                  memset(pReceiveData, 0, 1024);
                  memcpy(pReceiveData, iovbuf, vReciveLen);
                  DBG("receive data :\n%s\n", pReceiveData);
                  
                  pTmp = pReceiveData;
                  
                  // colect port mapping relation     
                  for(i=0;i<NET_MAX_INTERFACE;i++)
                  {                                          
                     if(pTmp!=NULL)
                     {
                        //fprintf(stderr,"pTmp=\n%s\n", pTmp);
                        
                        vPrivatePort=0;
                        memset(pAddress, 0, 32);
                        sscanf(pTmp,"%s %d", pAddress, &vPrivatePort);
                        
                        memset((char *) &gxPeerSockAddr[i], 0, sizeof(struct sockaddr_in));
                        gxPeerSockAddr[i].sin_family = AF_INET;
                        gxPeerSockAddr[i].sin_addr.s_addr = inet_addr(pAddress);
                        gxPeerSockAddr[i].sin_port = htons(vPrivatePort);
                        
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
                  
                  vPeerPort = vPrivatePort;
               }       
            }    
            break;
         }
         else if(cmsg->cmsg_type == IPPROTO_IP)
         {
            DBG("%s %s :%d\n",__FILE__,__func__, __LINE__);
         }
      }
   }
      

   // do hole punching
   gThreadRet=pthread_create( &gpThreadPtr[gThreadNo].thread_tid, NULL, (void *) PunchingThread, (void*)&gThreadNo );
   if(gThreadRet!=0)
   {
      fprintf (stderr, "Create pthread error!\n");
      exit (1);
   }
   gThreadNo++;   
                  
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
      tv.tv_sec = 10;
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
      //DBG("receive data from: %s\n", mh.msg_iov[0].iov_base);
             
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

               //pTmp = inet_ntoa(pi->ipi_spec_dst);
               pTmp = inet_ntoa(localaddr.sin_addr);
               if(pTmp)
                  memcpy(pSrc, pTmp, strlen(pTmp));
               
               
               // TODO: check here
               //DBG("%s %s :%d\n",__FILE__,__func__, __LINE__);
               //gxPeerSockAddr[i].sin_addr.s_addr
               DBG("-> receive packet from %s:%d, to %s\n", pSrc, ntohs(localaddr.sin_port), pDst);
               for(i=0;i<NET_MAX_INTERFACE;i++)
               {
                  if(gxPeerSockAddr[i].sin_addr.s_addr==localaddr.sin_addr.s_addr)
                  {
                     if(gxPeerSockAddr[0].sin_port!=localaddr.sin_port)
                     {
                        DBG("Change port from %d to %d for %s\n", vPeerPort, ntohs(localaddr.sin_port), pSrc);
                        vPeerPort = ntohs(localaddr.sin_port);
                        gxPeerSockAddr[0].sin_port = htons(vPeerPort);
                     }
                     else
                     {
                         char *pTmp;
                         
                         // inet_ntoa() use a global buffer to store the string,
                         // so we need to copy the value before we invoke inet_ntoa() next time
                         if(pPeerAddress)
                         {
                             pTmp = inet_ntoa(localaddr.sin_addr);
                             if(pTmp)
                                 memcpy(pPeerAddress, pTmp, strlen(pTmp));
                         }
                         
                         if(pPeerPort)
                         {
                             *pPeerPort = ntohs(localaddr.sin_port);
                         }
                         
                         bPunchingSuccess = 1;
                     }
                  }
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

   if(bPunchingSuccess==1)
   {
       DBG("P2P HolePunching Success\n");
       return vServerSocket;
   }
   else
   {
       DBG("P2P HolePunching Fail\n");
       close(vServerSocket);
       return 0;
   }
}

void PunchingThread(void* data)
{  
   int i=0,j=0; 
   pthread_detach(pthread_self());
   for(i=0;i<3;i++)
   {
      for(j=0;j<NET_MAX_INTERFACE;j++)
      {      
         if(gxPeerSockAddr[j].sin_port!=0)
         {
            if(sendto(gClientSocket, gpSendBuffer, strlen(gpSendBuffer), 0, (struct sockaddr*)&gxPeerSockAddr[j], sizeof(struct sockaddr)) < 0)
            {
               perror("punching error");
            }
            else
            {
               DBG("punching to %s:%d\n", inet_ntoa(gxPeerSockAddr[j].sin_addr), ntohs(gxPeerSockAddr[j].sin_port));
            } 
            sleep(1); 
         }
      }
   }
   pthread_exit ("thread all done");
   close(gClientSocket);
}
