#ifndef LIBP2P_H
#define LIBP2P_H

extern int coordinator(int argc, char **argv);

/*
return 0,  means punching fail
return > 0, means punching success
*/
extern int punching(char *pIfName, int vPort, char *pRendezvousServerAddress, char * pPeerAddress, int *pPeerPort);


#endif

