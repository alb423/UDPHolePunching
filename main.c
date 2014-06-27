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
                       
int thread_ret=0, thread_no=0;
typedef struct {
   pthread_t thread_tid;
   long thread_count;
}tThread;
tThread tptr[2];

void PunchingThread(void* data);


int vClientSocket = 0;
int vServerSocket = 0;
struct sockaddr_in gPeerSockAddr;
char pSendBuffer[128]={0};

int main(int argc, char **argv)
{
   int vReciveLen=0, vResult=0;
   int vPort=8888, vPeerPort=9999; 
   char *pMyAddress=NULL;
            
   initMyIpString();
   
   pMyAddress = getMyIpString(INTERFACE_NAME_1);
   memcpy(pSendBuffer, pMyAddress, strlen(pMyAddress));
   InitMyRandom(pMyAddress);
   if(argc==4)
   {
      memset((char *) &gPeerSockAddr, 0, sizeof(gPeerSockAddr));
      vPort = atoi(argv[1]);      
      vPeerPort = atoi(argv[3]);
      gPeerSockAddr.sin_family = AF_INET;
      gPeerSockAddr.sin_addr.s_addr = inet_addr(argv[2]);
      gPeerSockAddr.sin_port = htons(vPeerPort);          
   }
   else
   {
      printf("Usage: punching localPort peerAddr peerPort");
   }
   
   #if 0
   if(pMyAddress)
   {
      struct sockaddr_in vSockAddr;
      memset((char *) &vSockAddr, 0, sizeof(vSockAddr));
      vSockAddr.sin_family = AF_INET;
      vSockAddr.sin_addr.s_addr = inet_addr(pMyAddress);
      vSockAddr.sin_port = htons((vPort));      
      vClientSocket = CreateUnicastClient(&vSockAddr);
   }
   #endif
   
   vServerSocket = CreateUnicastServer(pMyAddress, vPort);
   vClientSocket = vServerSocket;

   // do hole punching
   
   free(pMyAddress);

   thread_ret=pthread_create( &tptr[thread_no].thread_tid, NULL, (void *) PunchingThread, (void*)&thread_no );
   if(thread_ret!=0)
   {
      fprintf (stderr, "Create pthread error!\n");
      exit (1);
   }
   thread_no++;   
                  
   while(1)
   {     
      struct sockaddr_in vxSockAddrFrom;
      memset((void*)&vxSockAddrFrom, 0, sizeof(vxSockAddrFrom));      
      
      // Reference www.cnblogs.com/kissazi2/p/3158603.html
      char cmbuf[1024], iovbuf[1024];
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
      tv.tv_sec = 5;
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
      DBG("set msg_namelen=%d msg_iovlen=%d msg_controllen=%d, vReciveLen=%d\n", mh.msg_namelen, mh.msg_iovlen, mh.msg_controllen,vReciveLen);
                 
      struct cmsghdr *cmsg = NULL;      
      struct in_pktinfo *pi = NULL;           
      for(cmsg = CMSG_FIRSTHDR(&mh) ;
          cmsg != NULL;
          cmsg = CMSG_NXTHDR(&mh, cmsg))
      {
         DBG("%s %s :%d\n",__FILE__,__func__, __LINE__);
         if(cmsg->cmsg_level != IPPROTO_IP || cmsg->cmsg_type != IP_PKTINFO)
         {DBG("%s %s :%d\n",__FILE__,__func__, __LINE__);
            printf("cmsg->cmsg_level=%d\n", cmsg->cmsg_level);
            continue;
         }        
         else if(cmsg->cmsg_type == IP_PKTINFO)
         {DBG("%s %s :%d\n",__FILE__,__func__, __LINE__);
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
                 
               DBG("%s %s :%d receive socket nIndex=%d pSrc=%s:%d, pDst=%s\n",__FILE__,__func__, __LINE__, pi->ipi_ifindex, pSrc, ntohs(localaddr.sin_port), pDst);                  
               DBG("%s %s :%d localaddr=%s port=%d\n",__FILE__,__func__, __LINE__, inet_ntoa(localaddr.sin_addr), ntohs(localaddr.sin_port));   
 
            }    
            break;
         }
         else if(cmsg->cmsg_type == IPPROTO_IP)
         {DBG("%s %s :%d\n",__FILE__,__func__, __LINE__);         
            DBG("%s %s :%d Good Job\n",__FILE__,__func__, __LINE__);   
         }
      }

      DBG("%s %s :%d\n",__FILE__,__func__, __LINE__);   

   }

   DBG("%s %s :%d end test\n",__FILE__,__func__, __LINE__);
      

   close(vServerSocket);
   return 1;
}

void PunchingThread(void* data)
{  
   int i=0; 
   pthread_detach(pthread_self());
   for(i=0;i<3;i++)
   {
      if(sendto(vClientSocket, pSendBuffer, strlen(pSendBuffer), 0, (struct sockaddr*)&gPeerSockAddr, sizeof(gPeerSockAddr)) < 0)
      {
         perror("punching error");
      }
      else
      {
         DBG("punching\n");  
      } 
      sleep(1);          
   }
   pthread_exit ("thread all done");
   close(vClientSocket);
}
