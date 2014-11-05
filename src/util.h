#ifndef UTIL_H
#define UTIL_H

#include <netinet/in.h>
#include <ifaddrs.h>

#ifndef noprintf
extern void noprintf(char *format, ...);
#endif
   
#define _DEUBG_
#ifdef _DEUBG_
	#define DBG printf
#else
	#define DBG noprintf
#endif

#define SEND_BUF_LEN  1024
#define COORDINATE_PORT 10001
#define NET_MAX_INTERFACE 8

#ifdef __APPLE__
#define INTERFACE_NAME_1 "en0"
#define INTERFACE_NAME_2 "en1"
#else
#define INTERFACE_NAME_1 "eth0"
#define INTERFACE_NAME_2 "eth1"
#endif


// Network
typedef struct tNICInfo {
    char pIfName[32];
    char pLocalAddr[32];
    char pMacAddr[32];
} tNICInfo;

extern int  gLocalInterfaceCount;
extern tNICInfo gxNICInfo[NET_MAX_INTERFACE];
extern int CreateUnicastClient(struct sockaddr_in *pSockAddr);
extern int CreateUnicastServer(char *pAddress, int *pPort);

extern void * MyMalloc(int vSize);
extern void MyFree(void *ptr);

extern char * CopyString(char *pSrc);
extern char * getMyIpString(char *pInterfaceName);
extern char * initMyIpString(void);
extern char * getMyMacAddress(void);

extern void InitMyRandom(char *myipaddr);
extern long our_random() ;
extern unsigned int our_random16();
extern unsigned int our_random32();
extern void UuidGen(char *uuidbuf);
extern int checkIPInTheNetwork( char *pTarIP, char *pNetIP, char *pNetMask);
extern char* getNextLine(char * inputLine);

extern int  isPrivateV4(unsigned int ip_in_host_order);
#endif

