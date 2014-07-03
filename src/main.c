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
#include "libtwp2p.h" 

int main(int argc, char **argv)
{
   int vLen =0, vExecutableLen=0; 
   int i=0, vCount=0;
   
   
   // busybox
   vLen = strlen("coordinator");
   vExecutableLen = strlen(argv[0]);
   if(vLen <= vExecutableLen )
   {
      // If the executable name is coordinator
      if(strcmp(&argv[0][vExecutableLen-vLen],"coordinator")==0)
      {
         return coordinator(argc, argv);
      }
   }
   else
   {
       if(argc==4)
       {
          tPeerData gxPeerData;
          memset(&gxPeerData, 0, sizeof(gxPeerData));      
          
          vCount = punching(argv[1], atoi(argv[2]), argv[3], &gxPeerData);
          for(i=0;i<vCount;i++)
          {
              printf("Connection to %s:%d is esablised\n",gxPeerData.pPeerAddress[i], gxPeerData.PeerPort[i]);
              close(gxPeerData.Socket[i]);
          }
       }      
       else
       {
           printf("Usage: punching ifname localPort coordinatorAddr\n");
       }         
   }
   return 0;      
}
