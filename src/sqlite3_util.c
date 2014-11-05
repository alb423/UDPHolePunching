#include <stdio.h>
#include <stdlib.h>
#include "sqlite3.h"
#include "twp2p_err.h"
/*
 The database schema
 SESSION: sessionId, userId, peerId
 MAPPING: userId, eAddr, ePort, iAddr, iPort, used
 */

int DB_Exist(char *pDBPath)
{
    FILE *fd = NULL;
    fd = fopen(pDBPath, "r");
    if(fd > 0)
    {
        fclose(fd);
        return 1;
    }
    else
    {
        return ERR_ARGUMENT_FAIL;
    }
}

void DB_DumpData(char *pDBPath)
{
    sqlite3 *pSqlHandle;
    
    // get absoulte path of DBNAME
    if (sqlite3_open(pDBPath, &pSqlHandle) == SQLITE_OK)
    {
        char pQuerySql[1024]={0};
        sprintf(pQuerySql, "select * from SESSION") ;
        
        sqlite3_stmt *statement = NULL;
        
        fprintf(stderr, "=== TABLE SESSION ===\n");
        if (sqlite3_prepare_v2(pSqlHandle, pQuerySql, -1, &statement, NULL) == SQLITE_OK)
        {
            while (sqlite3_step(statement) == SQLITE_ROW)
            {
                const unsigned char *pItem1, *pItem2, *pItem3;
                pItem1 = sqlite3_column_text(statement, 0);
                pItem2 = sqlite3_column_text(statement, 1);
                pItem3 = sqlite3_column_text(statement, 2);
                fprintf(stderr, "sessionId=%s, userId=%s, peerId=%s\n", pItem1, pItem2, pItem3);
            }
            sqlite3_finalize(statement);
        }
        
        sprintf(pQuerySql, "select * from MAPPING") ;
        fprintf(stderr, "=== TATABLE MAPPING ===\n");
        if (sqlite3_prepare_v2(pSqlHandle, pQuerySql, -1, &statement, NULL) == SQLITE_OK)
        {
            while (sqlite3_step(statement) == SQLITE_ROW)
            {
                const unsigned char *pItem1, *pItem2, *pItem3, *pItem4, *pItem5, *pItem6;
                pItem1 = sqlite3_column_text(statement, 0);
                pItem2 = sqlite3_column_text(statement, 1);
                pItem3 = sqlite3_column_text(statement, 2);
                pItem4 = sqlite3_column_text(statement, 3);
                pItem5 = sqlite3_column_text(statement, 4);
                pItem6 = sqlite3_column_text(statement, 5);
                
                fprintf(stderr, "(%s, %s, %s, %s, %s, %s)\n", pItem1, pItem2, pItem3, pItem4, pItem5, pItem6);
            }
            sqlite3_finalize(statement);
        }
        sqlite3_close(pSqlHandle);
        fprintf(stderr, "======\n\n");
    }
}

int DB_Init(char *pDBPath)
{
    sqlite3 *pSqlHandle;
    
    if (sqlite3_open(pDBPath, &pSqlHandle) == SQLITE_OK)
    {
        //fprintf(stderr,"DB OK\n");
        char *errorMsg;
        const char *createSql_1 = "create table if not exists SESSION (sessionId INTEGER PRIMARY KEY, userId text, peerId text)";
        if (sqlite3_exec(pSqlHandle, createSql_1, NULL, NULL, &errorMsg)==SQLITE_OK)
        {
            //fprintf(stderr,"Create SESSION table Ok\n");
        }
        else
        {
            fprintf(stderr,"Create SESSION Err: %s\n", errorMsg);
            sqlite3_free(errorMsg);
        }
        
        const char *createSql_2 = "create table if not exists MAPPING (userId text, eAddr text, ePort text, iAddr text, iPort text, used text)";
        if (sqlite3_exec(pSqlHandle, createSql_2, NULL, NULL, &errorMsg)==SQLITE_OK)
        {
            //fprintf(stderr,"Create MAPPING table Ok\n");
        }
        else
        {
            fprintf(stderr,"Create MAPPING Err: %s\n", errorMsg);
            sqlite3_free(errorMsg);
        }
        sqlite3_close(pSqlHandle);
    }
    else
    {
        fprintf(stderr, "Failed to open database at %s with error %s\n", pDBPath , sqlite3_errmsg(pSqlHandle));
        sqlite3_close (pSqlHandle);
        return ERR_ARGUMENT_FAIL;
    }
    
    return 1;
}


int DB_SESSION_InsertNew(char *pDBPath, char *pUserId, char *pPeerId)
{
    sqlite3 *pSqlHandle;
    
    if (sqlite3_open(pDBPath, &pSqlHandle) == SQLITE_OK)
    {
        char *errorMsg = NULL;
        char *insertErrorMsg;
        char pInsertSql[1024]={0};
        
        sprintf(pInsertSql,
                "insert into SESSION values (NULL, '%s','%s')", /* User Added */
                pUserId,
                pPeerId
                );
        
        if (sqlite3_exec(pSqlHandle, pInsertSql, NULL, NULL, &insertErrorMsg)==SQLITE_OK)
        {
            char pQuerySql[1024]={0};
            sprintf(pQuerySql, "select * from SESSION where userId = '%s' peerId = '%s'", pUserId, pPeerId) ;
            
            sqlite3_stmt *statement = NULL;
            
            const unsigned char *pItem1=NULL, *pItem2=NULL, *pItem3=NULL;
            if (sqlite3_prepare_v2(pSqlHandle, pQuerySql, -1, &statement, NULL) == SQLITE_OK)
            {
                if (sqlite3_step(statement) == SQLITE_ROW)
                {
                    pItem1 = sqlite3_column_text(statement, 0);
                    pItem2 = sqlite3_column_text(statement, 1);
                    pItem3 = sqlite3_column_text(statement, 2);
                    fprintf(stderr, "sessionId=%s, userId=%s, peerId=%s\n", pItem1, pItem2, pItem3);
                }
                sqlite3_finalize(statement);
            }
            
            sqlite3_close(pSqlHandle);
            if(pItem1 != NULL)
            {
               return atoi((const char*)pItem1);
            }
            else
            {
               return ERR_ARGUMENT_FAIL;
            }
        }
        else
        {
            fprintf(stderr,"error: %s\n",errorMsg);
            sqlite3_free(errorMsg);
        }
    }
    sqlite3_close(pSqlHandle);
    return ERR_ARGUMENT_FAIL;
}


int  DB_SESSION_GetDataByUserId(char *pDBPath, char *pSessionId, char *pUserId, char *pPeerId)
{
    int bFind=0;
    sqlite3 *pSqlHandle;
    
    if(!pDBPath || !pSessionId || !pUserId || !pPeerId)
        return ERR_ARGUMENT_FAIL;
        
    if (sqlite3_open(pDBPath, &pSqlHandle) == SQLITE_OK)
    {
        char pQuerySql[1024]={0};
        sprintf(pQuerySql, "select * from MAPPING where userId = '%s'", pUserId) ;
        
        sqlite3_stmt *statement = NULL;
        
        const unsigned char *pItem1=NULL, *pItem2=NULL, *pItem3=NULL;
        if (sqlite3_prepare_v2(pSqlHandle, pQuerySql, -1, &statement, NULL) == SQLITE_OK)
        {
            if (sqlite3_step(statement) == SQLITE_ROW)
            {
                pItem1 = sqlite3_column_text(statement, 0);
                pItem2 = sqlite3_column_text(statement, 1);
                pItem3 = sqlite3_column_text(statement, 2);
                
                strcpy(pSessionId, (const char *)pItem1);
                strcpy(pUserId, (const char *)pItem2);
                strcpy(pPeerId, (const char *)pItem3);
                bFind = 1;
            }
            sqlite3_finalize(statement);
        }
        
        sqlite3_close(pSqlHandle);
        if(bFind==1)
           return 1;
    }
    return 0;
}

void DB_SESSION_DeleteByUserId(char *pDBPath, char *pUserId)
{
    sqlite3 *pSqlHandle;
    char *deleteErrorMsg;
    char pDeleteSql[1024]={0};
    
    if (sqlite3_open(pDBPath, &pSqlHandle) == SQLITE_OK) {
        sprintf(pDeleteSql,
                "delete from SESSION where userId ='%s'",
                pUserId
                );
        
        if (sqlite3_exec(pSqlHandle, pDeleteSql, NULL, NULL, &deleteErrorMsg)==SQLITE_OK)
        {
            //fprintf(stderr,"DELETE OK\n");
        }
        else
        {
            fprintf(stderr,"DELETE Err: %s\n",deleteErrorMsg);
            sqlite3_free(deleteErrorMsg);
        }
        
        sqlite3_close(pSqlHandle);
    }
}

void DB_SESSION_DeleteBySessionId(char *pDBPath, char *pSessionId)
{
    sqlite3 *pSqlHandle;
    char *deleteErrorMsg;
    char pDeleteSql[1024]={0};
    
    if (sqlite3_open(pDBPath, &pSqlHandle) == SQLITE_OK)
    {
        sprintf(pDeleteSql,
                "delete from SESSION where sessionId ='%s'",
                pSessionId
                );
        
        if (sqlite3_exec(pSqlHandle, pDeleteSql, NULL, NULL, &deleteErrorMsg)==SQLITE_OK)
        {
            //fprintf(stderr,"DELETE OK\n");
        }
        else
        {
            fprintf(stderr,"DELETE Err: %s\n",deleteErrorMsg);
            sqlite3_free(deleteErrorMsg);
        }
        
        sqlite3_close(pSqlHandle);
    }
}


int DB_MAPPING_InsertNew(char *pDBPath, char *pUserId, char *pEAddr, char *pEPort, char *pIAddr, char *pIPort)
{
    sqlite3 *pSqlHandle;
    
    if (sqlite3_open(pDBPath, &pSqlHandle) == SQLITE_OK)
    {
        
        char *errorMsg = NULL;
        char *insertErrorMsg;
        char pInsertSql[1024]={0};
        sprintf(pInsertSql,
                "insert into MAPPING values ('%s','%s','%s','%s','%s','NO')", /* User Added */
                pUserId,
                pEAddr,
                pEPort,
                pIAddr,
                pIPort
                );
        
        if (sqlite3_exec(pSqlHandle, pInsertSql, NULL, NULL, &insertErrorMsg)==SQLITE_OK)
        {
            //fprintf(stderr,"INSERT OK\n");
            return 1;
        }
        else
        {
            fprintf(stderr,"error: %s\n",errorMsg);
            sqlite3_free(errorMsg);
        }
    }
    return ERR_ARGUMENT_FAIL;
}


int DB_MAPPING_GetDataByUserId(char *pDBPath, char *pUserId, char *pEAddr, char *pEPort, char *pIAddr, char *pIPort)
{
    int bFind = 0;
    sqlite3 *pSqlHandle;

    if(!pDBPath || !pUserId || !pEAddr || !pEPort || !pIAddr || !pIPort)
        return ERR_ARGUMENT_FAIL;
    
    if (sqlite3_open(pDBPath, &pSqlHandle) == SQLITE_OK)
    {
        char pQuerySql[1024]={0};
        sprintf(pQuerySql, "select * from MAPPING where userId = '%s'", pUserId) ;
        
        sqlite3_stmt *statement = NULL;
        
        const unsigned char *pItem1=NULL, *pItem2=NULL, *pItem3=NULL, *pItem4=NULL, *pItem5=NULL, *pItem6=NULL;
        if (sqlite3_prepare_v2(pSqlHandle, pQuerySql, -1, &statement, NULL) == SQLITE_OK)
        {
            if (sqlite3_step(statement) == SQLITE_ROW)
            {
                pItem1 = sqlite3_column_text(statement, 0);
                pItem2 = sqlite3_column_text(statement, 1);
                pItem3 = sqlite3_column_text(statement, 2);
                pItem4 = sqlite3_column_text(statement, 3);
                pItem5 = sqlite3_column_text(statement, 4);
                pItem6 = sqlite3_column_text(statement, 5);
                
                strcpy(pUserId, (const char *)pItem1);
                strcpy(pEAddr, (const char *)pItem2);
                strcpy(pEPort, (const char *)pItem3);
                strcpy(pIAddr, (const char *)pItem4);
                strcpy(pIPort, (const char *)pItem5);
                bFind = 1;
            }
            sqlite3_finalize(statement);
        }
        
        sqlite3_close(pSqlHandle);
        if(bFind==1)
           return 1;
    }
    return 0;
}


void DB_MAPPING_SetUsed(char *pDBPath, char *pUserId, char *pEAddr, char *pEPort)
{
    sqlite3 *pSqlHandle;
    char *updateErrorMsg;
    char pInsertSql[1024]={0};
    
    if (sqlite3_open(pDBPath, &pSqlHandle) == SQLITE_OK)
    {
        sprintf(pInsertSql,
                "update MAPPING set used='YES' where userId = '%s' and eAddr = '%s' and ePort = '%s'",
                pUserId,
                pEAddr,
                pEPort
                );
        
        if (sqlite3_exec(pSqlHandle, pInsertSql, NULL, NULL, &updateErrorMsg)==SQLITE_OK)
        {
            //fprintf(stderr,"UPDATE OK\n");
        }
        else
        {
            fprintf(stderr,"UPDATE Err: %s\n",updateErrorMsg);
            sqlite3_free(updateErrorMsg);
        }
        
        sqlite3_close(pSqlHandle);
    }
}


void DB_MAPPING_DeleteByUserId(char *pDBPath, char *pUserId)
{
    sqlite3 *pSqlHandle;
    char *deleteErrorMsg;
    char pDeleteSql[1024]={0};
    
    if (sqlite3_open(pDBPath, &pSqlHandle) == SQLITE_OK)
    {
        sprintf(pDeleteSql,
                "delete from MAPPING where userId ='%s'",
                pUserId
                );
        
        if (sqlite3_exec(pSqlHandle, pDeleteSql, NULL, NULL, &deleteErrorMsg)==SQLITE_OK) 
        {
            //fprintf(stderr,"DELETE OK\n");
        } 
        else 
        {
            fprintf(stderr,"DELETE Err: %s\n",deleteErrorMsg);
            sqlite3_free(deleteErrorMsg);
        }
        
        sqlite3_close(pSqlHandle);
    }
}
