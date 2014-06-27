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

//#define UNICAST_ADDR "192.168.1.24"//"239.255.255.250"
#define UNICAST_PORT 3702
#define NET_MAX_INTERFACE 4

#ifdef __APPLE__
#define INTERFACE_NAME_1 "en0"
#define INTERFACE_NAME_2 "en1"
#else
#define INTERFACE_NAME_1 "eth0"
#define INTERFACE_NAME_2 "eth1"
#endif
// Network

extern char gpLocalAddr[NET_MAX_INTERFACE][32];

extern int CreateUnicastClient(struct sockaddr_in *pSockAddr);
extern int CreateUnicastServer(char *pAddress, int port);

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

#endif

