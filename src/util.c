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

#include <ifaddrs.h>

#include "util.h"

int  gLocalInterfaceCount=0;
char gpLocalAddr[NET_MAX_INTERFACE][32]={{0}};
char gpMacAddr[NET_MAX_INTERFACE][32]={{0}};
#define LOCAL_ADDR gpLocalAddr//"192.168.2.102"


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
   
   vLen = strlen(pSrc);
   pDst = MyMalloc(vLen+1); 
   memcpy(pDst, pSrc, vLen);
   
   return pDst;
}


char * getMyMacAddress(void)
{
   return CopyString(gpMacAddr[0]);
}

// Utilities of Network
char * getMyIpString(char *pIfName)
{
   int i = 0;
   if(pIfName==NULL)
   return NULL;
   
   for(i=0;i<NET_MAX_INTERFACE;i++)
   {
      //if(strncmp("en1", pIfName, 3)==0)
      if(strncmp(INTERFACE_NAME_2, pIfName, strlen(INTERFACE_NAME_2))==0)
      {
         return CopyString(gpLocalAddr[1]);
      } 
   }
   return CopyString(gpLocalAddr[0]);
}


char * initMyIpString(void)
{
   int vInterfaceCount=0;
   char pInterface[128]={0};
   struct ifaddrs * ifAddrStruct=NULL;
   struct ifaddrs * ifa=NULL;
   void * tmpAddrPtr=NULL;
   
   getifaddrs(&ifAddrStruct);
   gLocalInterfaceCount = 0;
   for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) 
   {
      if (ifa ->ifa_addr->sa_family==AF_INET) 
      {   
         // check it is IP4
         // is a valid IP4 Address
         char addressBuffer[INET_ADDRSTRLEN];
         tmpAddrPtr=&((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
         
         inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
         //DBG("%s IP Address %s\n", ifa->ifa_name, addressBuffer); 
         if(strncmp(ifa->ifa_name, INTERFACE_NAME_1, 2)==0)
         {
            // Note: you may set local address for different interface. For example:eth0, eth1
            memcpy(gpLocalAddr[vInterfaceCount], addressBuffer, strlen(addressBuffer));
            memset(pInterface, 0 ,128);
            memcpy(pInterface, ifa->ifa_name, strlen(ifa->ifa_name));
            
            #ifdef __APPLE__
               // I don't know how to get Mac Address in Mac OS
               sprintf(gpMacAddr[0], "10ddb1acc6ee");
               sprintf(gpMacAddr[1], "4c8d79eaee74");
            #else            
            {
               // For linux system
               int sock;
               struct ifreq ifr;
               
               sock = socket(AF_INET, SOCK_DGRAM, 0);
               ifr.ifr_addr.sa_family = AF_INET;
               
               strncpy(ifr.ifr_name, pInterface, IFNAMSIZ-1);
               
               ioctl(sock, SIOCGIFHWADDR, &ifr);
               
               close(sock);
               
               sprintf(gpMacAddr[vInterfaceCount], "%.2x%.2x%.2x%.2x%.2x%.2x", 
               (unsigned char)ifr.ifr_hwaddr.sa_data[0],
               (unsigned char)ifr.ifr_hwaddr.sa_data[1],
               (unsigned char)ifr.ifr_hwaddr.sa_data[2],
               (unsigned char)ifr.ifr_hwaddr.sa_data[3],
               (unsigned char)ifr.ifr_hwaddr.sa_data[4],
               (unsigned char)ifr.ifr_hwaddr.sa_data[5]);
               //DBG("MAC %s\n", gpMacAddr[vInterfaceCount]); 
            }
            #endif
            vInterfaceCount++;
            gLocalInterfaceCount++;
         } 
      } 
      else if (ifa->ifa_addr->sa_family==AF_INET6) 
      {   
         // check it is IP6
         // is a valid IP6 Address
         tmpAddrPtr=&((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
         char addressBuffer[INET6_ADDRSTRLEN];
         inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
         //DBG("%s IP Address %s\n", ifa->ifa_name, addressBuffer); 
      }         
   }
   
   
   DBG("gpLocalAddr is set to %s, MAC is %s\n", gpLocalAddr[0], gpMacAddr[0]);    
   if (ifAddrStruct!=NULL) freeifaddrs(ifAddrStruct);
   return gpLocalAddr[0];
}

int CreateUnicastClient(struct sockaddr_in *pSockAddr)
{
   // http://www.tenouk.com/Module41c.html
   int sd=-1;
   
   struct timeval timeout;
   timeout.tv_sec  = 10;
   timeout.tv_usec = 0;
   
   sd = socket(AF_INET, SOCK_DGRAM, 0);
   if(sd < 0)
   {
      perror("Opening datagram socket error");
      exit(1);
   }
   //else
   //   DBG("Opening the datagram socket %d...OK.\n", sd);
   
   if(setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0)
   {
      DBG("setsockopt...error.\n");
   }
   
   if(bind(sd, (struct sockaddr*)pSockAddr, sizeof(*pSockAddr)))
   {
      perror("Binding datagram socket error");
      close(sd);
      exit(1);
   }   
   DBG("Create unicast client socket %d at port %d...OK.\n",sd, ntohs(pSockAddr->sin_port));
   return sd;
}

int CreateUnicastServer(char *pAddress, int vPort)
{
   int sd;
   struct sockaddr_in localSock;   
   
   if(!pAddress)
      return -1;
   if(strlen(pAddress)==0)
      return -1;
         
   sd = socket(AF_INET, SOCK_DGRAM, 0);
   if(sd < 0)
   {
      perror("Opening datagram socket error");
      exit(1);
   }
   //else
   //   DBG("Opening unicast server socket %d for ip %s...OK.\n",sd, pAddress);
   
   int opt = 1;
   if(setsockopt(sd, IPPROTO_IP, IP_PKTINFO, &opt, sizeof(opt)) < 0)
   {
       perror("Setting IP_PKTINFO error");
       close(sd);
       exit(1);
   }
    
   if(setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0)
   {
      perror("Setting SO_REUSEADDR error");
      close(sd);
      exit(1);
   }
/*      
   if(setsockopt(sd, SOL_SOCKET, SO_REUSEPORT, (char *)&opt, sizeof(opt)) < 0)
   {
      perror("Setting SO_REUSEADDR error");
      close(sd);
      exit(1);
   }
*/    
   memset((char *) &localSock, 0, sizeof(localSock));
   localSock.sin_family = AF_INET;
   localSock.sin_port = htons(vPort);
   localSock.sin_addr.s_addr = inet_addr(pAddress);
   
   if(bind(sd, (struct sockaddr*)&localSock, sizeof(localSock)))
   {
      perror("Binding datagram socket error");
      close(sd);
      exit(1);
   }
   //else
   //   DBG("Binding datagram socket...OK.\n");
   
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
   
   unsigned int seed = ourAddress^timeNow.tv_sec^timeNow.tv_usec;
     
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
           