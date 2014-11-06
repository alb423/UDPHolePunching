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
#include "sqlite3_util.h"

void dbtest(void);
int main(int argc, char **argv)
{
    int vLen =0, vExecutableLen=0;
    int i=0, vCount=0;

    // busybox
    printf("argv[0]=%s\n",argv[0]);
    vLen = strlen("coordinator");
    vExecutableLen = strlen(argv[0]);
    if(vLen <= vExecutableLen ) {
        // If the executable name is coordinator
        if(strcmp(&argv[0][vExecutableLen-vLen],"coordinator")==0) {
            if(argc==2) {
                return coordinator(argv[1]);
            } else {
                printf("Usage: coordinator ifname\n");
            }
        }
    } else if(strstr(argv[0],"dbtest")!=NULL) {
        dbtest();
    } else {

        if(argc==5) {
            tPeerData gxPeerData;
            memset(&gxPeerData, 0, sizeof(gxPeerData));

            vCount = punching(atoi(argv[1]), argv[2], atoi(argv[3]), argv[4], &gxPeerData);
            for(i=0; i<vCount; i++) {
                printf("Connection to %s:%d is esablised\n",gxPeerData.pPeerAddress[i], gxPeerData.PeerPort[i]);
                close(gxPeerData.Socket[i]);
            }
        } else {
            printf("Usage: punching actor ifname localPort coordinatorAddr\n");
        }
    }
    return 0;
}


void dbtest(void)
{
    int i, result;

    char pDBPath[]="p2p.sqlite";
    char pUserId[10]= {0}, pPeerId[10]= {0};
    char pEAddr[32]= {0}, pIAddr[32]= {0};
    char pEPort[10]= {0}, pIPort[10]= {0};

    remove(pDBPath);

    result = DB_Exist(pDBPath);
    if(result<0) printf("DB doesn't exist!!\n");

    result = DB_Init(pDBPath);
    if(result<0) printf("DB_Init error!!\n");

    for(i=0; i<3; i++) {
        sprintf(pUserId, "%d", i+1);
        sprintf(pPeerId, "%d", i+11);
        DB_SESSION_InsertNew(pDBPath, pUserId, pPeerId);
    }
    printf("After insert 3 session\n");
    DB_DumpData(pDBPath);

    sprintf(pEPort, "7777");
    sprintf(pIPort, "7778");
    for(i=0; i<3; i++) {
        sprintf(pUserId, "%d", i+1);
        sprintf(pPeerId, "%d", i+11);
        sprintf(pEAddr, "168.95.0.%d", i+11);
        sprintf(pIAddr, "192.168.0.%d", i+1);

        DB_MAPPING_InsertNew(pDBPath, pUserId, pEAddr, pEPort, pIAddr, pIPort);
    }
    printf("After insert 3 mapping\n");
    DB_DumpData(pDBPath);


    printf("After delete and update\n");
    DB_SESSION_DeleteBySessionId(pDBPath, "1");
    DB_MAPPING_DeleteByUserId(pDBPath, "1");
    DB_MAPPING_SetUsed(pDBPath, "2", "168.95.0.12", "7777");
    DB_DumpData(pDBPath);
}