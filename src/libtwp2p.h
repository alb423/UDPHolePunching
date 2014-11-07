#ifndef LIBTWP2P_H
#define LIBTWP2P_H

#include "twp2p_err.h"

typedef enum eP2PActor
{
   eActor_IPCAM = 0,
   eActor_APP   = 1,
   eActor_MAX
} eP2PActor;


#ifndef NET_MAX_INTERFACE
#define NET_MAX_INTERFACE 8
#endif

typedef struct tPeerData {
    int punchingID;
    in_addr_t s_addr[NET_MAX_INTERFACE];
    int  Socket[NET_MAX_INTERFACE];
    int  Port[NET_MAX_INTERFACE];
    char pAddress[NET_MAX_INTERFACE][32];
} tPeerData;

typedef struct tLocalData {
    int  Port[NET_MAX_INTERFACE];
    char pAddress[NET_MAX_INTERFACE][32];
} tLocalData;

extern int coordinator(char *pIfName);

/*
return <= 0,  means punching fail
return > 0, means punching success
*/
extern int punching(eP2PActor vActor, char *pIfName, int vPort, char *pRendezvousServerAddress, tLocalData *pLocalData, tPeerData *pPeerData);


#endif

