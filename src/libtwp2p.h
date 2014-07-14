#ifndef LIBTWP2P_H
#define LIBTWP2P_H

#define ACTOR_APP    2
#define ACTOR_IPCAM  1

#define NET_MAX_INTERFACE 8
typedef struct tPeerData {
    in_addr_t s_addr[NET_MAX_INTERFACE];
    int  Socket[NET_MAX_INTERFACE];
    int  PeerPort[NET_MAX_INTERFACE];
    char pPeerAddress[NET_MAX_INTERFACE][32];
} tPeerData;

extern int coordinator(char *pIfName);

/*
return 0,  means punching fail
return > 0, means punching success
*/
extern int punching(int vActor, char *pIfName, int vPort, char *pRendezvousServerAddress, tPeerData *pPeerData);


#endif

