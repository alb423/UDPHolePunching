#ifndef SQLITE3_UTIL_H
#define SQLITE3_UTIL_H

#define  DBPATH "p2p.sqlite"

int DB_Exist(char *pDBPath);
int DB_Init(char *pDBPath);
void DB_DumpData(char *pDBPath);

int DB_SESSION_InsertNew(char *pDBPath, char *pUserId, char *pPeerId);
int  DB_SESSION_GetDataByUserId(char *pDBPath, char *pSessionId, char *pUserId, char *pPeerId);
void DB_SESSION_DeleteByUserId(char *pDBPath, char *pUserId);
void DB_SESSION_DeleteBySessionId(char *pDBPath, char *pSessionId);

int DB_MAPPING_InsertNew(char *pDBPath, char *pUserId, char *pEAddr, char *pEPort, char *pIAddr, char *pIPort);
void DB_MAPPING_SetUsed(char *pDBPath, char *pUserId, char *pEAddr, char *pEPort);
void DB_MAPPING_DeleteByUserId(char *pDBPath, char *pUserId);
int DB_MAPPING_GetDataByUserId(char *pDBPath, char *pUserId, char *pEAddr, char *pEPort, char *pIAddr, char *pIPort);
#endif
