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
#include <errno.h>

#include "util.h"
#include "twp2p_err.h"
#include "libtwp2p.h"

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


// below triples may have more than one
// return socket, pPeerAddress, &vPeerPort
int punching(eP2PActor vActor, char *pIfName, int vPort, char *pRendezvousServerAddress, tPeerData *pPeerData)
{
   int i=0, j=0;
   int vLen=0, vReciveLen=0, vResult=0, vPeerPort=0;
   int vServerSocket=0, vPrivatePort=0;
   int bPunchingSuccess=0, vPunchingSuccessCount=0;
   char pReceiveData[1024]={0};
   char pAddress[32]={0};
   char *pMyAddress=NULL;

   if(!pPeerData)
       return ERR_ARGUMENT_FAIL;
    
   memset((char *) &gxPeerSockAddr, 0, sizeof(struct sockaddr_in)*4);
   initMyIpString();
   
   // TODO: the interface name can be assigned by user
   // or we do the same punching on all interface
   
   pMyAddress = getMyIpString(pIfName);
   if(pMyAddress==NULL)
   {
      fprintf(stderr, "Address for Interface(%s) can not obtained\n",pIfName);
      return ERR_ARGUMENT_FAIL;
   }
   memset(gpSendBuffer, 0, SEND_BUF_LEN);
   
   if(vActor==eActor_IPCAM)
   {
      sprintf(gpSendBuffer, "i=IAMIPCAM\n");
   }
   else
   {
      sprintf(gpSendBuffer, "i=IAMAPP\n");
   }
    
    for(i=0;i<gLocalInterfaceCount;i++)
    {
        if(strncmp(gxNICInfo[i].pIfName,"lo",2)!=0)
        {
            vLen = strlen(gpSendBuffer);
            if(gxNICInfo[i].pMacAddr[0]!=0x00)
            {
               sprintf(gpSendBuffer+vLen, "u=%s\n", gxNICInfo[i].pMacAddr);
            }
            else
            {
                sprintf(gpSendBuffer+vLen, "u=4c8d79eaee74\n");
            }
        }
    }

    if(vActor==eActor_IPCAM)
    {
        vLen = strlen(gpSendBuffer);
        sprintf(gpSendBuffer+vLen, "p=4c8d79eaee74\n");
    }
    else
    {
        vLen = strlen(gpSendBuffer);
        sprintf(gpSendBuffer+vLen, "p=080027fdf0db\n");
    }
    
   InitMyRandom(pMyAddress);

   // TODO: create a server for a NIC
   vResult = CreateUnicastServer(pMyAddress, &vPort);
   if(vResult<0)
   {
      return vResult;
   }
   else
   {
      vServerSocket = vResult;
      gClientSocket = vServerSocket;
   }

   //sprintf(gpSendBuffer, "aid=APPDeviceToken\n");
   //vLen = strlen(gpSendBuffer);
   //sprintf(gpSendBuffer+vLen, "cid=CamearID\n"); 
   for(i=0;i<gLocalInterfaceCount;i++)
   {
      // omit 127.0.0.1 here
      vLen = strlen(gpSendBuffer);
      if(strncmp(gxNICInfo[i].pLocalAddr,"127",3)==0)
      {
         //printf("omit 127.0.0.1\n");
      }
      else
      {
         sprintf(gpSendBuffer+vLen, "c=%s %d\n", gxNICInfo[i].pLocalAddr, vPort);
      }
   }
   fprintf(stderr, "\ngpSendBuffer=\n%s\n",gpSendBuffer);
   
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
      else
      {
         fprintf (stderr, "sendto %s success\n", pRendezvousServerAddress);;
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
         return ERR_NETWORK_IS_UNREACHABLE;
      }
      else if(vResult == 0)
      {
         printf("timeout\n");
         return ERR_NETWORK_IS_UNREACHABLE;
      }
             
      if (!FD_ISSET(vServerSocket, &readset)) 
      {
         printf("receive unexpected data\n");
         return ERR_NETWORK_IS_UNREACHABLE;
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
               char *pTmp=NULL, pSrc[32]={0}, pDst[32]={0};
                  
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
                  int   vPunchingId=0;
                  char *pTmp2;
                  memset(pReceiveData, 0, 1024);
                  memcpy(pReceiveData, iovbuf, vReciveLen);
                  DBG("receive data :\n%s\n", pReceiveData);
                  
                  pTmp2 = pReceiveData;
                  
                  // colect punching id assigned by coordinator
                  sscanf(pTmp2,"s=%d", &vPunchingId);
                  pPeerData->punchingID = vPunchingId;
                   
                  // colect port mapping relation     
                  for(i=0;i<NET_MAX_INTERFACE;i++)
                  {                                          
                     if(pTmp2!=NULL)
                     {
                        //fprintf(stderr,"pTmp=\n%s\n", pTmp);
                        
                        vPrivatePort=0;
                        memset(pAddress, 0, 32);
                        sscanf(pTmp2,"c=%s %d", pAddress, &vPrivatePort);
                        
                        memset((char *) &gxPeerSockAddr[i], 0, sizeof(struct sockaddr_in));
                        gxPeerSockAddr[i].sin_family = AF_INET;
                        gxPeerSockAddr[i].sin_addr.s_addr = inet_addr(pAddress);
                        gxPeerSockAddr[i].sin_port = htons(vPrivatePort);
                        
                        pTmp2 = strstr(pTmp2, "\n");
                        if(pTmp2!=NULL) 
                        {
                           // omit "\n"
                           pTmp2 = pTmp2 + strlen("\n");
                           if(strlen(pTmp2)==0)
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
      return ERR_THREAD_CREATE_FAIL;
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
                     if(gxPeerSockAddr[i].sin_port!=localaddr.sin_port)
                     {
                        DBG("Change port from %d to %d for %s\n", vPeerPort, ntohs(localaddr.sin_port), pSrc);
                        vPeerPort = ntohs(localaddr.sin_port);
                        gxPeerSockAddr[i].sin_port = htons(vPeerPort);
                     }
                     else
                     {                         
                         // inet_ntoa() use a global buffer to store the string,
                         // so we need to copy the value before we invoke inet_ntoa() next time
                         int bExist = 0;
                         for(j=0;j<vPunchingSuccessCount;j++)
                         {
                             //DBG("%d to %d \n", pPeerData->s_addr[j], localaddr.sin_addr.s_addr);
                             if(pPeerData->s_addr[j]==localaddr.sin_addr.s_addr)
                             {
                                bExist = 1;
                                break;
                             }
                         }
                         
                         if(bExist==0)
                         {
                            char *pTmp2;
                            pTmp2 = inet_ntoa(localaddr.sin_addr);
                            if(pTmp2)
                                memcpy(pPeerData->pPeerAddress[vPunchingSuccessCount], pTmp2, strlen(pTmp2));
                            
                            pPeerData->PeerPort[vPunchingSuccessCount] = ntohs(localaddr.sin_port);
                            pPeerData->Socket[vPunchingSuccessCount] = vServerSocket;
                            pPeerData->s_addr[vPunchingSuccessCount] = localaddr.sin_addr.s_addr;
                            vPunchingSuccessCount++;
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
       DBG("P2P HolePunching Success, session id = %d\n", pPeerData->punchingID);
       return vPunchingSuccessCount;
   }
   else
   {
       DBG("P2P HolePunching Fail\n");
       close(vServerSocket);
       return ERR_PUNCHING_FAIL;
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
   pthread_exit ((void *)"thread all done");
   close(gClientSocket);
}
