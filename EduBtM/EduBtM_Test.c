/******************************************************************************/
/*                                                                            */
/*    ODYSSEUS/EduCOSMOS Educational-Purpose Object Storage System            */
/*                                                                            */
/*    Developed by Professor Kyu-Young Whang et al.                           */
/*                                                                            */
/*    Database and Multimedia Laboratory                                      */
/*                                                                            */
/*    Computer Science Department and                                         */
/*    Advanced Information Technology Research Center (AITrc)                 */
/*    Korea Advanced Institute of Science and Technology (KAIST)              */
/*                                                                            */
/*    e-mail: kywhang@cs.kaist.ac.kr                                          */
/*    phone: +82-42-350-7722                                                  */
/*    fax: +82-42-350-8380                                                    */
/*                                                                            */
/*    Copyright (c) 1995-2013 by Kyu-Young Whang                              */
/*                                                                            */
/*    All rights reserved. No part of this software may be reproduced,        */
/*    stored in a retrieval system, or transmitted, in any form or by any     */
/*    means, electronic, mechanical, photocopying, recording, or otherwise,   */
/*    without prior written permission of the copyright owner.                */
/*                                                                            */
/******************************************************************************/
/*
 * Module: EduBtM_Test.c
 *
 * Description : 
 * Test the EduBtM and show the result of test.
 *
 * Exports:
 *  Four EduBtM_Test(Four, Four)
 */

#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <stdlib.h>
#include "EduBtM_common.h"
#include "EduBtM_basictypes.h"
#include "EduBtM.h"
#include "EduBtM_TestModule.h"
#include "Util_hash.h"

struct AnalyticsStruct {
	int 	numInsertDupButNoDup;
	int 	numInsertNoDupButDup;
	int 	numDeleteExistButNoExist;
	int		numDeleteNoExistButExist;
	int 	numScanNotFoundButFound;
	int 	numScanFoundButNotFound;
	int		numScanNotSameObject;
	int		numScanUndercount;
	int		numScanOvercount;
	int 	numNotImplemented;
	int 	numEtcError;
};

struct perfTestResultStruct {
	Four		keyType;
	Four		specType;
	uint64_t	spendTime;
};

static Boolean logFlag;
const struct objectMapStruct *objectMap = NULL;

Four dumpBtreePage(PageID*, KeyDesc);
void dumpInternal(BtreeInternal*, PageID*, Two);
void dumpLeaf(BtreeLeaf*, PageID*, Two);
void dumpOverflow(BtreeOverflow*, PageID*);
void generateWorkloadFileName(Four, Four, Four, Four, char*);
void generateTestPrintf(Four, Four, Four);
void parse(char*, Four, Four, Four*, Four*, Four*, char*, Four*, Four*, Four*, char*, Four*);
void execute(ObjectID*, PhysicalIndexID*, KeyDesc*, Four, Four, Four, Four*, Four*, Four*, Four*, char*, Four*, Four*, Four*, char*, Four*, struct AnalyticsStruct*);
void rawKey2Key(char*, Four, Four*, char*);
void stringToCompOp(char*, Four* );
void makeKeyValue(Four, Four* , char*, KeyValue*);
void fprintfWrapper(FILE*, char*, ...);
void printAnalytics(struct AnalyticsStruct* );
void mergeAnalytics(struct AnalyticsStruct*, struct AnalyticsStruct*);
void printPerformanceTest(struct perfTestResultStruct *, Four, Four*);
void printFinalScore(Four, Four, Four);
void fprintJSONResult(FILE*, Four, Four);
Four gradeWorkload(struct AnalyticsStruct *);
Four totalErrorCount(struct AnalyticsStruct *);

/*@================================
 * EduBtM_Test()
 *================================*/
/*
 * Function: EduBtM_Test(Four volId, Four handle)
 *
 * Description : 
 *  Show a result of a test of EduBtM.
 *  EduBtM is module which is implemented for operation 
 *  related to B+ tree structure. The B+ tree strucutre is 
 *  composed of root page, internal page, and leaf page.
 *  There are five operations in EduBtM.
 *  EduBtM_Test() test these below operations in EduBtM.
 *  EduBtM_CreateIndex(), EduBtM_InsertObject(), EduBtM_Fetch(),
 *  EduBtM_FetchNext(), EduBtM_DropIndex().
 *
 *
 * Returns:
 *  error code
 *    some errors caused by function calls
 */

Four EduBtM_Test(Four volId, Four handle){
	
	Four e;												/* for errors */
    FileID      fid;                                    /* file identifier */
	ObjectID    catalogEntry;                           /* catalog object */
	Four numOfIntegerObject = NUMOFINSERTEDOBJECT;		/* number of integer object */
	Four numOfVariableObject = 0;						/* number of variable object */
	Two operation;										/* operation number */
	Two	scanOperation;									/* operation number of scan function */
	IndexID iid;										/* index id */
	ObjectID oid;										/* object id */
	PageID 	dumpPage;									/* page identifier for dump */
	PhysicalFileID pFid;								/* physical file identifier for EduBtM_DropIndex() */ 
	PhysicalIndexID		rootPid;						/* root page identifier */
	KeyValue	kval;									/* value of key */
	KeyValue	startKval;								/* start value of key for EduBtM_FetchNext() */
	KeyValue	stopKval;								/* stop value of key for EduBtM_FetchNext() */
	KeyDesc		kdesc;									/* key descriptor */
	Four		keyValueNumber = 0;						/* value of integer key */	
    sm_CatOverlayForBtree catalogOverlay; 				/* Btree part of the catalog entry */
	BtreeCursor cursor;									/* cursor for EduBtM_FetchNext() */
	BtreeCursor next;									/* next object cursor from EduBtM_FetchNext() */
    Two 		lengthOfPlayerName;						/* length of variable key */
	char 		playerName[MAXPLAYERNAME];				/* value of  variable key */
	char		waste[MAXPLAYERNAME];					/* waste value */
	char		workloadFileName[MAXFILENAME] = "";		/* workload file name to use in test */
	char 		line[MAXFILENAME];						/* file read line */
	FILE		*fp;
	Four 		opcode;									/* query operation code */
	Four 		startCompOp; 							/* start comparion operation code */
	Four 		startIntKey; 							/* start integer key */
	char 		startStringKey[MAXKEY] = ""; 			/* start string key */
	Four 		startValue; 							/* start value (int) */
	Four 		endCompOp; 								/* end comparion operation code */
	Four 		endIntKey; 								/* end integer key */
	char 		endStringKey[MAXKEY] = ""; 				/* end string key */
	Four 		endValue;								/* end value (int) */
	Four 		testType; 								/* test type */
	Four 		keyType;								/* key type */
	Four 		workloadType; 							/* workload type */
	Four 		specType;								/* workload type */
	Four		numObjects;								/* number of inserted Objects */
	Four		numPerfTests = 0;						/* number of performance tests */
	uint64_t 	microSec;								/* time of performance test */
	char		testLogFileName[10] = "log.txt";		/* log file name */
	char		resultFileName[20] = "./test/result.json";		/* result file name */
	Four 		coverageScore = 0;						/* coverage score */
	Four 		totalScore	= 0;						/* total coverage score with base */
	Four 		totalTime	= 0;						/* total coverage score with base */
	struct timespec startTime, endTime;
	struct AnalyticsStruct curAnalytics = {0};
	struct perfTestResultStruct perfTestResults[MAXPERFTEST];
	
	printf("Loading EduBtM_Test() complete...\n");
	logFp = fopen(testLogFileName, "w");
	resultFp = fopen(resultFileName, "w");

	for(testType = COVERAGE; testType <= PERFORMANCE; testType++) {
		logFlag = testType == COVERAGE ? TRUE : FALSE;
		for (keyType = RANDINT; keyType <= EMAIL; keyType++) {
			/* Create File */
			e = SM_CreateFile(volId, &fid, FALSE, NULL);
			if (e < eNOERROR) ERR(e);
			/* Get catalog entry */
			e = sm_GetCatalogEntryFromDataFileId(ARRAYINDEX, &fid, &catalogEntry);
			if (e < eNOERROR) ERR(e);
			
			for (specType = A; specType <= E; specType++) {
				generateTestPrintf(testType, keyType, specType);
				struct AnalyticsStruct tmpAnalytics = {0};
				clock_gettime(CLOCK_MONOTONIC_RAW, &startTime);
		
				/* The successful default solution code is called if "Edu" is omitted from the function name in the following line */
				e = EduBtM_CreateIndex(&catalogEntry, &rootPid);
				if(e == eNOTSUPPORTED_EDUBTM) {
					tmpAnalytics.numNotImplemented++;
				}
				else if(e < eNOERROR) {
					tmpAnalytics.numEtcError++;
					ERR(e);
				}
				
				/* Construct Kval and Kdesc */
				kdesc.flag = KEYFLAG_UNIQUE;
				kdesc.nparts = 1;
				kdesc.kpart[0].type = keyType == EMAIL ? SM_VARSTRING : SM_INT;
				kdesc.kpart[0].offset = 0;
				kdesc.kpart[0].length = keyType == EMAIL ? MAXKEY : sizeof(Four);
				
				fprintfWrapper(logFp,"****************************** Inserting objects ******************************\n");
				workloadType = LOAD;
				generateWorkloadFileName(testType, keyType, workloadType, specType, workloadFileName);

				fp = fopen(workloadFileName, "r");
				if (fp == NULL) { printf("No workload file %s\n", workloadFileName); continue; }

				numObjects = 0;
				while (fgets(line, sizeof(line), fp) != NULL ) {
					parse(line, testType, keyType, &opcode, &startCompOp, &startIntKey, startStringKey, &startValue, &endCompOp, &endIntKey, endStringKey, &endValue);
					execute(&catalogEntry, &rootPid, &kdesc, volId, testType, keyType, &numObjects, &opcode, 
							&startCompOp, &startIntKey, startStringKey, &startValue, &endCompOp, &endIntKey, endStringKey, &endValue, &tmpAnalytics);
				}

				fclose(fp);
				
				fprintfWrapper(logFp, "****************************** Running workload ******************************\n");
				workloadType = TXNS;
				generateWorkloadFileName(testType, keyType, workloadType, specType, workloadFileName);

				fp = fopen(workloadFileName, "r");
				if (fp == NULL) { printf("No workload file %s\n", workloadFileName); continue; }

				while (fgets(line, sizeof(line), fp) != NULL ) {
					fprintfWrapper(logFp, "\nExecuting %s\n", line);
					parse(line, testType, keyType, &opcode, &startCompOp, &startIntKey, startStringKey, &startValue, &endCompOp, &endIntKey, endStringKey, &endValue);
					execute(&catalogEntry, &rootPid, &kdesc, volId, testType, keyType, &numObjects, &opcode, 
							&startCompOp, &startIntKey, startStringKey, &startValue, &endCompOp, &endIntKey, endStringKey, &endValue, &tmpAnalytics);
				}

				fclose(fp);

				MAKE_PHYSICALFILEID(pFid, catalogOverlay.fid.volNo, catalogOverlay.firstPage);

				/* The successful default solution code is called if "Edu" is omitted from the function name in the following line */
				e = EduBtM_DropIndex(&pFid, &rootPid, &dlPool, &dlHead);
				if (e < eNOERROR) ERR(e);	

				fprintfWrapper(logFp,"\n");
				
				if(testType == COVERAGE) {
					coverageScore += gradeWorkload(&tmpAnalytics);
					printAnalytics(&tmpAnalytics);
					mergeAnalytics(&tmpAnalytics, &curAnalytics);
					deleteAll();
				}
				else {
					clock_gettime(CLOCK_MONOTONIC_RAW, &endTime);
					microSec = (endTime.tv_sec - startTime.tv_sec) * 1000000 + (endTime.tv_nsec - startTime.tv_nsec) / 1000;
					perfTestResults[numPerfTests].keyType = keyType;
					perfTestResults[numPerfTests].specType = specType;
					perfTestResults[numPerfTests].spendTime = microSec;
					numPerfTests++;
				}
			}
		
			/* Destroy File */
			e = SM_DestroyFile(&fid, NULL);
			if (e < eNOERROR) ERR(e);
		}
	}
	
	printf("\n########################### TOTAL TEST RESULT ############################\n");
	printf("\n                               Coverage \n");
	printAnalytics(&curAnalytics);
	printf("                             Performance \n");
	printPerformanceTest(perfTestResults, numPerfTests, &totalTime);
	//printf("                             Final Score \n");
	//printFinalScore(coverageScore, totalScore = coverageScore + 10, totalTime);
	//fprintJSONResult(resultFp, totalScore, totalTime);
	fclose(logFp); 
	fclose(resultFp);
}
/* End test for variable key value. */


/*@================================
 * dumpBtreePage()
 *================================*/
/*
 * Function: Four  dumpBtreePage(PageID*)
 *
 * Description:
 *  dump routine.
 *
 * Returns:
 *  Error code
 *    some errors caused by function calls
 */
Four dumpBtreePage(
		PageID		*pid,
		KeyDesc		kdesc)
{
	Four e;         /* error number */
	BtreePage *apage;       /* page to dump */
	
	
	e = BfM_GetTrain(pid, (char **)&apage, PAGE_BUF);
	if (e < 0)  ERR(e);
	
	if (kdesc.kpart[0].type == SM_INT){
		if (apage->any.hdr.type & INTERNAL)
			dumpInternal(&(apage->bi), pid, kdesc.kpart[0].type);
		else if (apage->any.hdr.type & LEAF)
			dumpLeaf(&(apage->bl), pid, kdesc.kpart[0].type);
		else if (apage->bo.hdr.type & OVERFLOW)
			dumpOverflow(&(apage->bo), pid);
		else
			ERRB1(eBADBTREEPAGE_BTM, pid, PAGE_BUF);
	}else if (kdesc.kpart[0].type == SM_VARSTRING){
		if (apage->any.hdr.type & INTERNAL)     
			dumpInternal(&(apage->bi), pid, kdesc.kpart[0].type);  
		else if (apage->any.hdr.type & LEAF)       
			dumpLeaf(&(apage->bl), pid, kdesc.kpart[0].type);  
		
		else if (apage->bo.hdr.type & OVERFLOW)     
			dumpOverflow(&(apage->bo), pid);  
		else        
			ERRB1(eBADBTREEPAGE_BTM, pid, PAGE_BUF);
	}
	e = BfM_FreeTrain(pid, PAGE_BUF);
	if (e < 0) ERR(e);
	return(eNOERROR);
}

/*@================================
 * dumpInternal()
 *================================*/
/* Function: void dumpInternal(BtreeInternal*, PageID*)
 *
 * Description:
 *  Dump an internal page.
 * 
 * Returns:
 *  None
 */
void dumpInternal(
		BtreeInternal       *internal,      /* IN pointer to buffer of internal page */
		PageID              *pid,			/* IN page identifier */
		Two					type            /* IN type for key value */
		) 
{
	Two                 i;              /* index variable */
	Two                 entryOffset;    /* starting offset of an internal entry */
	btm_InternalEntry   *entry;         /* IN an internal entry */
	Two                 j;              /* index variable */
	Two                 len;            /* key value length */
	char        playerName[MAXPLAYERNAME];  /* data of the key value */
	int					tempKval;

	printf("\n\t|=========================================================|\n");
	printf("\t|    PageID = (%4d,%6d)     type = INTERNAL%s      |\n",
			pid->volNo, pid->pageNo, (internal->hdr.type & ROOT) ? "|ROOT":"     ");
	printf("\t|=========================================================|\n");
	printf("\t| free = %-5d, unused = %-5d", internal->hdr.free, internal->hdr.unused);
	printf("nSlots = %-5d, p0 = %-5d  |\n", internal->hdr.nSlots, internal->hdr.p0 );
	printf("\t|---------------------------------------------------------|\n");
	if (type == SM_INT)
		for (i = 0; i < internal->hdr.nSlots; i++) {
			entryOffset = internal->slot[-i];
			entry = (btm_InternalEntry*)&(internal->data[entryOffset]);
			printf("\t| ");
			memcpy((char*)&tempKval, (char*)entry->kval, sizeof(Four_Invariable)); /* YRK07JUL2003 */
			printf("        klen = %4d :  Key = %4d  : spid = %4d        |\n", entry->klen, tempKval, entry->spid);
		}
	else if (type == SM_VARSTRING)
		for (i = 0; i < internal->hdr.nSlots; i++) {
			entryOffset = internal->slot[-i];
			entry = (btm_InternalEntry*)&(internal->data[entryOffset]);
			printf("\t| ");
			memcpy((char*)&len, (char*)&(entry->kval[0]), sizeof(Two));
			memcpy(&playerName, &(entry->kval[sizeof(Two)]), MAXPLAYERNAME);
			printf("	klen = %4d : Key = %s", len, playerName);
			printf(" : spid = %5d |\n", entry->spid);
		}

	printf("\t|---------------------------------------------------------|\n");
	
}  /* dumpInternal */


/*@================================
 * dumpLeaf()
 *================================*/
/*
 * Function: void dumpLeaf(BtreeLeaf*, PageID*)
 * 
 * Description:
 *  Dump a leaf page.
 *  
 * Returns:
 *  None
 */
void dumpLeaf(
		BtreeLeaf           *leaf,          /* IN pointer to buffer of Leaf page */
		PageID              *pid,			/* IN pointer to leaf PageID */
		Two					type			/* IN type for key value */
		)        
{
	Two                 entryOffset;    /* starting offset of a leaf entry */
	btm_LeafEntry       *entry;         /* a leaf entry */
	ObjectID            *oid;           /* an object identifier */
	Two                 i;              /* index variable */
	Two                 j;              /* index variable */
	Two                 alignedKlen;    /* aligned length of the key length */
	Two                 len;            /* length of the key value */
	char        playerName[MAXPLAYERNAME];  /* data of the key value */
	Four				tempKval;


	printf("\n\t|===============================================================================|\n");
	printf("\t|               PageID = (%4d,%4d)            type = LEAF%s                |\n",
			pid->volNo, pid->pageNo, (leaf->hdr.type & ROOT) ? "|ROOT":"     ");
	printf("\t|===============================================================================|\n");
	printf("\t|             free = %-5d, unused = %-5d", leaf->hdr.free, leaf->hdr.unused);												   
	printf("  nSlots = %-5d                      |\n", leaf->hdr.nSlots);
	printf("\t|             nextPage = %-10d,  prevPage = %-10d                     |\n",
			leaf->hdr.nextPage, leaf->hdr.prevPage );
	printf("\t|-------------------------------------------------------------------------------|\n");

	if (type == SM_INT) 
		for (i = 0; i < leaf->hdr.nSlots; i++) {
			entryOffset = leaf->slot[-i];
			entry = (btm_LeafEntry*)&(leaf->data[entryOffset]);
			printf("\t| ");
			memcpy((char*)&tempKval, (char*)entry->kval, sizeof(Four_Invariable)); /* YRK07JUL2003 */
			printf("klen = %3d : Key = %-4d", entry->klen, tempKval);
			printf(" : nObjects = %d : ", entry->nObjects);
			
			alignedKlen = ALIGNED_LENGTH(entry->klen);
			if (entry->nObjects < 0)
				printf(" OverPID = %d ", *((ShortPageID*)&(entry->kval[alignedKlen])));
			else {
				oid = (ObjectID *)&(entry->kval[alignedKlen]);
				printf(" ObjectID = (%4d, %4d, %4d, %4d) |\n", oid->volNo, oid->pageNo, oid->slotNo, oid->unique);
			}
		}
	else if (type == SM_VARSTRING)
		for (i = 0; i < leaf->hdr.nSlots; i++) {
			entryOffset = leaf->slot[-i];
			entry = (btm_LeafEntry*)&(leaf->data[entryOffset]);
			
			printf("\t| ");
			memcpy((char*)&len, (char*)&(entry->kval[0]), sizeof(Two));
			memcpy(&playerName, &(entry->kval[sizeof(Two)]), MAXPLAYERNAME);
			printf("klen = %3d : Key = %s", len, playerName);
			printf(" : nObjects = %d : ", entry->nObjects);

			alignedKlen = ALIGNED_LENGTH(entry->klen);			            
			if (entry->nObjects < 0)
				printf(" OverPID = %d ", *((ShortPageID*)&(entry->kval[alignedKlen])));
			else {
				oid = (ObjectID *)&(entry->kval[alignedKlen]);
				printf(" ObjectID = (%4d, %4d, %4d, %4d) |\n", oid->volNo, oid->pageNo, oid->slotNo, oid->unique);
			}

		}
	printf("\t|-------------------------------------------------------------------------------|\n");
	
}  /* dumpLeaf*/




/*@================================
 * dumpOverflow()
 *================================*/
/*
 * Function: Four dumpOverflow(BtreeOverflow*, PageID*)
 *
 * Description:
 *  Dump the overflow page.
 *
 * Returns:
 *  None
 */
void dumpOverflow(
		BtreeOverflow *overflow,    /* IN pointer to bugffer of Overflow Page */
		PageID        *pid)     /* IN PageID of overflow page */
{
	Two           i;            /* index variable */
	ObjectID      oid;      /* an object identifier */
	
	
	printf("\n\t|===========================================================================|\n");
	printf("\t|          PageID = (%4d,%6d)            type = OVERFLOW            |\n",
			pid->volNo, pid->pageNo);
	printf("\n\t|===========================================================================|\n");
	printf("nextPage = %-5d, prevPage = %-5d       |\n",
			overflow->hdr.nextPage, overflow->hdr.prevPage);
	printf("\t|---------------------------------------------------------------------------|");
	
	for (i = 0; i < overflow->hdr.nObjects; i++) {
		if(i%3 == 0) printf("\n\t| ");
		
		oid = overflow->oid[i];
		printf("  (%-4d, %-4d, %-4d, %-4d) ",
				oid.volNo, oid.pageNo, oid.slotNo, oid.unique);
	}
	printf("\n\t|---------------------------------------------------------------------------|\n");
	
}  /* dumpOverflow() */

/*@================================
 * parse()
 *================================*/
/*
 * Function: void parse(char*, Four, Four, Four*, Four*, Four*, char*, Four*, Four*, Four*, char*, Four*)
 *
 * Description:
 *  Parse given query. Generate execute options.
 *
 * Returns:
 *  Parsed execute options
 */
void parse(
		char* query,			/* IN raw query */
		Four  testType,			/* IN test type */
		Four  keyType,			/* In key type */
		Four* opcode,			/* OUT operation code */
		Four* startCompOp, 		/* OUT start comparion operation code */
		Four* startIntKey, 		/* OUT start integer key */
		char* startStringKey, 	/* OUT start string key */
		Four* startValue, 		/* OUT start value (int) */
		Four* endCompOp, 		/* OUT end comparion operation code */
		Four* endIntKey, 		/* OUT end integer key */
		char* endStringKey, 	/* OUT end string key */
		Four* endValue			/* OUT end value (int) */
	)
{
	char* delimeter = " ";
	char* rawKeyString;
	char* rawEndKeyString;
	char* rawOpcodeString;
	char* rawCompOpString;
	
	*startCompOp = NIL;
	*startIntKey = NIL;
	*startValue = NIL;
	*endCompOp = NIL;
	*endIntKey = NIL;
	*endValue = NIL;
	
	rawOpcodeString = strtok(query, delimeter);
	
	if (testType == PERFORMANCE) {
		if (strcmp(rawOpcodeString, "INSERT") == 0) {
			*opcode = INSERT;
			
			rawKeyString = strtok(NULL, delimeter);
			rawKey2Key(rawKeyString, keyType, startIntKey, startStringKey);
		}
		else if (strcmp(rawOpcodeString, "DELETE") == 0) {
			*opcode = DELETE;
			
			rawKeyString = strtok(NULL, delimeter);
			rawKey2Key(rawKeyString, keyType, startIntKey, startStringKey);
		}
		else if (strcmp(rawOpcodeString, "SCAN") == 0) {
			*opcode = SCAN;
			
			strtok(NULL, delimeter); //Alwayes EQ
			*startCompOp = SM_EQ;
			
			rawKeyString = strtok(NULL, delimeter);
			rawKey2Key(rawKeyString, keyType, startIntKey, startStringKey);
			
			rawCompOpString = strtok(NULL, delimeter);
			rawKeyString = strtok(NULL, delimeter);
			if(strcmp(rawCompOpString, "EQ") == 0) {
				*endCompOp = SM_EQ;
				rawKey2Key(rawKeyString, keyType, endIntKey, endStringKey);
			}
			else if(strcmp(rawCompOpString, "EOF") == 0) {
				*endCompOp = SM_EOF;
				rawKey2Key(rawKeyString, RANDINT, endIntKey, endStringKey);
			}
			else {
				fprintfWrapper(logFp,"Unsupported comparison operator\n");
				return 1;
			}
		}
		else {
			*opcode = NOP;
		}
	}
	else {
		if (strcmp(rawOpcodeString, "INSERT") == 0) {
			*opcode = INSERT;
			
			rawKeyString = strtok(NULL, delimeter);
			rawKey2Key(rawKeyString, keyType, startIntKey, startStringKey);
		}
		else if (strcmp(rawOpcodeString, "DELETE") == 0) {
			*opcode = DELETE;
			
			rawKeyString = strtok(NULL, delimeter);
			rawKey2Key(rawKeyString, keyType, startIntKey, startStringKey);
		}
		else if (strcmp(rawOpcodeString, "SCAN") == 0) {
			*opcode = SCAN;
			
			rawCompOpString = strtok(NULL, delimeter);
			stringToCompOp(rawCompOpString, startCompOp);
			if(!(*startCompOp == SM_EOF || *startCompOp == SM_BOF)) {
				rawKeyString = strtok(NULL, delimeter);
				rawKey2Key(rawKeyString, keyType, startIntKey, startStringKey);
			}
			
			rawCompOpString = strtok(NULL, delimeter);
			stringToCompOp(rawCompOpString, endCompOp);
			if(!(*endCompOp == SM_EOF || *endCompOp == SM_BOF)) {
				rawKeyString = strtok(NULL, delimeter);
				rawKey2Key(rawKeyString, keyType, endIntKey, endStringKey);
			}
		}
		else {
			*opcode = NOP;
		}
	}
}

/*@================================
 * execute()
 *================================*/
/*
 * Function: void execute(Four*, Four*, Four*, char*, Four*, Four*, Four*, char*, Four*)
 *
 * Description:
 *  Execute query
 *
 * Returns:
 *  None
 */
void execute(
		ObjectID* catalogEntry, 		/* IN catalog object */
		PhysicalIndexID* rootPid,		/* IN root page identifier */
		KeyDesc* kdesc,					/* IN key descriptor */
		Four volId, 					/* IN volume ID */
		Four testType,				/* IN test type */
		Four keyType,					/* IN key type */
		Four* numObjects,				/* IN number of objects */
		Four* opcode,					/* IN operation code */
		Four* startCompOp, 				/* IN start comparion operation code */
		Four* startIntKey, 				/* IN start integer key */
		char* startStringKey, 			/* IN start string key */
		Four* startValue, 				/* IN start value (int) */
		Four* endCompOp, 				/* IN end comparion operation code */
		Four* endIntKey, 				/* IN end integer key */
		char* endStringKey, 			/* IN end string key */
		Four* endValue,					/* IN end value (int) */
		struct AnalyticsStruct* analytics		/* IN coverage analytics */
	)
{
	Four 		e;					/* for errors */
	ObjectID 	oid;				/* object id */
	KeyValue	kval;				/* value of key */
	KeyValue	startKval;			/* start value of key for EduBtM_FetchNext() */
	KeyValue	stopKval;			/* stop value of key for EduBtM_FetchNext() */
	char		stringKey[MAXKEY];	/* key storage use for scan */
	Four		intKey;				/* key storage use for scan */
	Boolean 	numberScanFlag = FALSE;	/* EOF numbers flag */
	Four		numberCount;
	struct objectMapStruct *hashResult = NULL;
	
	BtreeCursor cursor;				/* cursor for EduBtM_FetchNext() */
	BtreeCursor next;				/* next object cursor from EduBtM_FetchNext() */
	
	switch(*opcode) {
		case INSERT: 
		{
			oid.pageNo = 777;
			oid.volNo = volId;

			makeKeyValue(keyType, startIntKey, startStringKey, &kval);

			oid.slotNo = *numObjects;
			oid.unique = (*numObjects)++;
			/* The successful default solution code is called if "Edu" is omitted from the function name in the following line */
			e = EduBtM_InsertObject(catalogEntry, rootPid, kdesc, &kval, &oid, NULL, NULL);
			if (e == eDUPLICATEDKEY_BTM) {
				fprintfWrapper(logFp, "There is the same key in the B+ tree index.\nEduBtM allows only unique keys\n");
				if(testType == COVERAGE) {
					if (isExist(keyType, *startIntKey, startStringKey) == FALSE) {
						analytics->numInsertDupButNoDup++;
						fprintfWrapper(logFp, "Correctness failed. Actually no duplication exists\n");
					}
					else addObject(keyType, *startIntKey, startStringKey, oid);	
				}
			}
			else if(e == eNOTSUPPORTED_EDUBTM) {
				analytics->numNotImplemented++;
			}
			else if(e < eNOERROR) {
				analytics->numEtcError++;
				ERR(e);
			}
			else {
				if(keyType == EMAIL)
					fprintfWrapper(logFp, "The object (key: %s , OID: (%4d, %4d, %4d, %4d)) is inserted into the index.\n", startStringKey, oid.volNo, oid.pageNo, oid.slotNo, oid.unique);
				else
					fprintfWrapper(logFp, "The object (key: %d , OID: (%d, %d, %d, %d)) is inserted into the index.\n", *startIntKey, oid.volNo, oid.pageNo, oid.slotNo, oid.unique);
				
				if (testType == COVERAGE) {
					if (isExist(keyType, *startIntKey, startStringKey) == TRUE){
						analytics->numInsertNoDupButDup++;
						fprintfWrapper(logFp, "Correctness failed. Actually duplication exists\n");
					}
					else addObject(keyType, *startIntKey, startStringKey, oid);
				}
			}
			
			break;	
		}
		case DELETE: 
		{
			makeKeyValue(keyType, startIntKey, startStringKey, &kval);
			
			/* The successful default solution code is called if "Edu" is omitted from the function name in the following line */
			e = EduBtM_Fetch(rootPid, kdesc, &kval, SM_EQ, &kval, SM_EQ, &cursor);
			if(e == eNOTSUPPORTED_EDUBTM) {
				analytics->numNotImplemented++;
			}
			else if(e < eNOERROR) {
				analytics->numEtcError++;
				ERR(e);
			}
			else if (cursor.flag == CURSOR_EOS) {
				fprintfWrapper(logFp, "There is no object that satisfies the condition.\n");
				
				if(testType == COVERAGE){
					if (isExist(keyType, *startIntKey, startStringKey) == TRUE) {
						analytics->numDeleteNoExistButExist++;
						fprintfWrapper(logFp, "Correctness failed. Fetching returns not exist but exist\n");
						deleteObject(findObject(keyType, *startIntKey, startStringKey));
					}
				}
			}
			else
			{
				/* The successful default solution code is called if "Edu" is omitted from the function name in the following line */
				e = EduBtM_DeleteObject(catalogEntry, rootPid, kdesc, &kval, &(cursor.oid), &dlPool, &dlHead);
				if (e == eNOTFOUND_BTM) {
					fprintfWrapper(logFp,"There is no object that statisfies the condition.\n");
					
					if(testType == COVERAGE){
						if (isExist(keyType, *startIntKey, startStringKey) == TRUE) {
							analytics->numDeleteNoExistButExist++;
							fprintfWrapper(logFp, "Correctness failed. Delete returns not exist but exists\n");
							deleteObject(findObject(keyType, *startIntKey, startStringKey));
						}
					}
				}
				else if(e == eNOTSUPPORTED_EDUBTM) {
					analytics->numNotImplemented++;
				}
				else if(e < eNOERROR) {
					analytics->numEtcError++;
					ERR(e);
				}
				else {
					if(keyType == EMAIL)
						fprintfWrapper(logFp,"The object (key: %s, OID: ( %4d, %4d, %4d, %4d)) is deleted from the B+ tree index.\n",
							startStringKey, cursor.oid.volNo, cursor.oid.pageNo, cursor.oid.slotNo, cursor.oid.unique);
					else
						fprintfWrapper(logFp,"The object (key: %d, OID: ( %d, %d, %d, %d)) is deleted from the B+ tree index.\n",
							*startIntKey, cursor.oid.volNo, cursor.oid.pageNo, cursor.oid.slotNo, cursor.oid.unique);
					
					if(testType == COVERAGE){
						if (isExist(keyType, *startIntKey, startStringKey) == FALSE) {
							analytics->numDeleteExistButNoExist++;
							fprintfWrapper(logFp, "Correctness failed. Actually key not exists\n");
						}
						else deleteObject(findObject(keyType, *startIntKey, startStringKey));
					}
				}
			}
			break;	
		}
		case SCAN: 
		{
			makeKeyValue(keyType, startIntKey, startStringKey, &startKval);
			makeKeyValue(keyType, endIntKey, endStringKey, &stopKval);
			
			/* The successful default solution code is called if "Edu" is omitted from the function name in the following line */	
			e = EduBtM_Fetch(rootPid, kdesc, &startKval, *startCompOp, &stopKval, *endCompOp, &cursor);
			if (testType == COVERAGE) {
				sort(keyType);
				hashResult = fetch(keyType, *startCompOp, *endCompOp, *startIntKey, startStringKey, *endIntKey, endStringKey);
			}
			if(e == eNOTSUPPORTED_EDUBTM) {
				analytics->numNotImplemented++;
			}
			else if(e < eNOERROR) {
				analytics->numEtcError++;
				ERR(e);
			}
			else if (cursor.flag == CURSOR_EOS) {
				fprintfWrapper(logFp,"There is no object that satisfies the condition.\n");
				
				if(testType == COVERAGE && hashResult != NULL) {
					analytics->numScanNotFoundButFound++;
					fprintfWrapper(logFp, "Correctness failed. In first scan(btm_fetch), no found but actually exists\n");
				}
				break;
			}
			else {
				fprintfWrapper(logFp,"Cursor points to the %dth slot in the leaf page ( volNo = %d, pageNo = %d )\n",
						cursor.slotNo, cursor.leaf.volNo, cursor.leaf.pageNo);
				
				if(keyType == EMAIL) {
					memcpy(stringKey, &(cursor.key.val[sizeof(Two)]), MAXKEY);
					fprintfWrapper(logFp,"Key: %s, OID: (%4d, %4d, %4d, %4d)\n",
							stringKey, cursor.oid.volNo, cursor.oid.pageNo, cursor.oid.slotNo, cursor.oid.unique);
				}
				else {
					memcpy(&intKey, &(cursor.key.val[0]), sizeof(Four_Invariable));
					fprintfWrapper(logFp, "Key: %d, OID: (%d, %d, %d, %d)\n",
							intKey, cursor.oid.volNo, cursor.oid.pageNo, cursor.oid.slotNo, cursor.oid.unique);
				}
				
				if(testType == COVERAGE) {
					if(hashResult == NULL) {
						analytics->numScanFoundButNotFound++;
						fprintfWrapper(logFp, "Correctness failed. In first scan(btm_fetch), found but actually not exists\n");
						break;
					}
					if(sameObject(keyType, hashResult, intKey, stringKey, cursor.oid) == FALSE)  {
						analytics->numScanNotSameObject++;
						fprintfWrapper(logFp, "Correctness failed. In first scan(btm_fetch), found but not same.\n");
						if(keyType == EMAIL) {
							fprintfWrapper(logFp, "Key: %s, OID: (%d, %d, %d, %d)\n",
								hashResult->stringKey, hashResult->oid.volNo, hashResult->oid.pageNo, hashResult->oid.slotNo, hashResult->oid.unique);
						}
						else {
							fprintfWrapper(logFp, "Key: %d, OID: (%d, %d, %d, %d)\n",
								hashResult->intKey, hashResult->oid.volNo, hashResult->oid.pageNo, hashResult->oid.slotNo, hashResult->oid.unique);
						}
						break;
					}
				} 
			
				if(*endCompOp == SM_EOF && *endIntKey > 0) {
					numberScanFlag = TRUE;
					numberCount = 0;
				}
			}
			
			do{
				/* The successful default solution code is called if "Edu" is omitted from the function name in the following line */
				e = EduBtM_FetchNext(rootPid, kdesc, &stopKval, *endCompOp, &cursor, &next);
				if (testType == COVERAGE) hashResult = fetchNext(keyType, *endCompOp, hashResult, *endIntKey, endStringKey);

				if(e == eNOTSUPPORTED_EDUBTM) {
					analytics->numNotImplemented++;
				}
				else if(e < eNOERROR) {
					analytics->numEtcError++;
					ERR(e);
				}
				else if (next.flag == CURSOR_EOS) {
					fprintfWrapper(logFp, "There is no object that satisfies the condition.\n");
					
					if(testType == COVERAGE && hashResult != NULL) {
						analytics->numScanUndercount++;
						fprintfWrapper(logFp, "Correctness failed. In second scan(btm_fetchNext), no next but exists\n");
					}
					break;
				}   
				cursor = next;
				
				fprintfWrapper(logFp, "Cursor points to the %dth slot in the leaf page ( volNo = %d, pageNo = %d )\n",
						cursor.slotNo, cursor.leaf.volNo, cursor.leaf.pageNo);

				if(keyType == EMAIL) {
					memcpy(stringKey, &(cursor.key.val[sizeof(Two)]), MAXKEY);
					fprintfWrapper(logFp, "Key: %s, OID: (%4d, %4d, %4d, %4d)\n",
							stringKey, cursor.oid.volNo, cursor.oid.pageNo, cursor.oid.slotNo, cursor.oid.unique);
				}
				else {
					memcpy(&intKey, &(cursor.key.val[0]), sizeof(Four_Invariable));
					fprintfWrapper(logFp, "Key: %d, OID: (%d, %d, %d, %d)\n",
							intKey, cursor.oid.volNo, cursor.oid.pageNo, cursor.oid.slotNo, cursor.oid.unique);
				}
				
				if(testType == COVERAGE) {
					if(hashResult == NULL){
						analytics->numScanOvercount++;
						fprintfWrapper(logFp, "Correctness failed. In second scan(btm_fetchNext), next but not exists\n");
						break;
					}
					else if(sameObject(keyType, hashResult, intKey, stringKey, cursor.oid) == FALSE)  {
						analytics->numScanNotSameObject++;
						fprintfWrapper(logFp, "Correctness failed. In first scan(btm_fetch), found but not same.\n");
						if(keyType == EMAIL) {
							fprintfWrapper(logFp, "Key: %s, OID: (%d, %d, %d, %d)\n",
								hashResult->stringKey, hashResult->oid.volNo, hashResult->oid.pageNo, hashResult->oid.slotNo, hashResult->oid.unique);
						}
						else {
							fprintfWrapper(logFp, "Key: %d, OID: (%d, %d, %d, %d)\n",
								hashResult->intKey, hashResult->oid.volNo, hashResult->oid.pageNo, hashResult->oid.slotNo, hashResult->oid.unique);
						}
						break;
					}
				} 
				
				if(numberScanFlag == TRUE) {
					if(++numberCount >= *endIntKey) break;
				}
			} while(1);
			
			break;	
		}
	}
}

/*@================================
 * rawKey2Key()
 *================================*/
/*
 * Function: void rawKey2Key(char*, Four, Four*, char*)
 *
 * Description:
 *  convert raw key string to int or string key based on key type
 *
 * Returns:
 *  converted key
 */
void rawKey2Key(
		char* rawKeyString, 	/* IN raw string of key */
		Four keyType,			/* IN int or string. key type */
		Four* intKey, 			/* OUT int version key */
		char* stringKey			/* OUT string version key */
	) 
{
	if (keyType == EMAIL) {
		strcpy(stringKey, rawKeyString);
		stringKey[strcspn(stringKey, "\n")] = 0;
	}
	else {
		*intKey = atoi(rawKeyString);
	}
}

/*@================================
 * stringToCompOp()
 *================================*/
/*
 * Function: void stringToCompOp(char*, Four*)
 *
 * Description:
 *  dict from string to comparison operator
 *
 * Returns:
 *  converted comparison operator
 */
void stringToCompOp(
		char* rawString,		/* IN input raw string */
		Four* compOp 			/* OUT result */
	)
{
	rawString[strcspn(rawString, "\n")] = 0;
	if(strcmp(rawString, "EQ") == 0) 
		*compOp = SM_EQ;
	else if (strcmp(rawString, "LT") == 0) 
		*compOp = SM_LT;
	else if (strcmp(rawString, "LE") == 0) 
		*compOp = SM_LE;
	else if (strcmp(rawString, "GT") == 0) 
		*compOp = SM_GT;
	else if (strcmp(rawString, "GE") == 0) 
		*compOp = SM_GE;
	else if (strcmp(rawString, "NE") == 0) 
		*compOp = SM_NE;
	else if (strcmp(rawString, "EOF") == 0) 
		*compOp = SM_EOF;
	else if (strcmp(rawString, "BOF") == 0) 
		*compOp = SM_BOF;
	else {
		fprintfWrapper(logFp, "Unsupported comparison operator\n");
		exit(0);
	}
}

/*@================================
 * makeKeyValue()
 *================================*/
/*
 * Function: void makeKeyValue(Four, Four*, char*)
 *
 * Description:
 *  Generate keyValue object from key
 *
 * Returns:
 *  KeyValue
 */
void makeKeyValue(
		Four keyType,
		Four* intKey,
		char* stringKey,
		KeyValue* kval
	)
{
	Two length;
	if (keyType == EMAIL) {
		length = strlen(stringKey);
		kval->len = MAXKEY;
		memcpy(&(kval->val[0]), &length, sizeof(Two));
		memcpy(&(kval->val[sizeof(Two)]), stringKey, MAXKEY);
	}
	else {
		kval->len = sizeof(Four_Invariable);
		memcpy(&(kval->val[0]), intKey, sizeof(Four_Invariable));
	}
}

/*@================================
 * generateWorkloadFileName()
 *================================*/
/*
 * Function: void generateWorkloadFileName(Four, Four, Four, Four, char*)
 *
 * Description:
 *  Generate workload file name based on given parameters
 *
 * Returns:
 *  file name
 */
void generateWorkloadFileName(
		Four testType, 		/* IN test type */
		Four keyType, 		/* IN key type */
		Four workloadType, 	/* IN workload type */
		Four specType,		/* IN workload type */
		char* fileName		/* OUT generated file name */
	)
{
	char* dirName = "test/workloads/";
	char* testName = testType == COVERAGE ? "coverage" : "performance";
	char* keyName = keyType == RANDINT ? "rand_int" : keyType == MONOINT ? "mono_inc" : "email";
	char* workloadName = workloadType == LOAD ? "load" : "txns";
	char* specName = specType == A ? "a" : specType == B ? "b" : specType == C ? "c" : specType == D ? "d" : "e" ;
	char* middleName = "_zipf_int_100M_";
	char* extensionName = ".dat";
	
	fileName[0] = '\0';
	strcat(fileName, dirName);
	strcat(fileName, testName);
	strcat(fileName, "_");
	strcat(fileName, keyName);
	strcat(fileName, "_");
	strcat(fileName, workloadName);
	strcat(fileName, middleName);
	strcat(fileName, specName);
	strcat(fileName, extensionName);
}

/*@================================
 * generateTestPrintf()
 *================================*/
/*
 * Function: void generateTestPrintf(Four, Four, Four, Four)
 *
 * Description:
 *  Generate test setting printf
 *
 * Returns:
 *  None
 */
void generateTestPrintf(
		Four testType, 		/* IN test type */
		Four keyType, 		/* IN key type */
		Four specType		/* IN workload type */
	)
{
	char* testName = testType == COVERAGE ? "Coverage" : "Performance";
	char* keyName = keyType == RANDINT ? "Random Integer" : keyType == MONOINT ? "Monotonically Increasing Integer" : "Email";
	char* specName = specType == A ? "A" : specType == B ? "B" : specType == C ? "C" : specType == D ? "D" : "E" ;
	
	fprintfWrapper(logFp, "############################## Test Setting ##############################\n");
	fprintfWrapper(logFp, "Test purpose : %s\n", testName);
	fprintfWrapper(logFp, "Key type : %s\n", keyName);
	fprintfWrapper(logFp, "Workload spec type : %s\n", specName);
	fprintfWrapper(logFp, "##########################################################################\n");
	
	printf("%s %s workload%s is now running...\n", testName, keyName, specName);
}

/*@================================
 * fprintfWrapper()
 *================================*/
/*
 * Function: void fprintfWrapper(Boolean, FILE*, char*, ...)
 *
 * Description:
 *  Wrapping fprintf. Print on only debug mode
 *
 * Returns:
 *  None
 */
void fprintfWrapper(FILE* stream, char* fmt, ...)
{
	va_list arg_ptr;
	
	if(logFlag == TRUE) {
		va_start(arg_ptr, fmt);
		vfprintf(stream, fmt, arg_ptr);
		va_end(arg_ptr);		
	}
}

/*=================================
 * Analytics functions
 *================================*/

Four totalErrorCount(struct AnalyticsStruct* as) {
	return 
		as->numInsertDupButNoDup +
		as->numInsertNoDupButDup +
		as->numDeleteExistButNoExist +
		as->numDeleteNoExistButExist +
		as->numScanNotFoundButFound +
		as->numScanFoundButNotFound +
		as->numScanNotSameObject +
		as->numScanOvercount +
		as->numScanUndercount +
		as->numNotImplemented +
		as->numEtcError;
}

void printAnalytics(struct AnalyticsStruct* as){
	int sum = totalErrorCount(as);
	printf("\n############################# Test Result ################################\n");
	printf("INSERT | ANS = Duplicate, ACTUAL = No duplicate : %d\n", as->numInsertDupButNoDup);
	printf("INSERT | ANS = No duplicate, ACTUAL = Duplicate : %d\n", as->numInsertNoDupButDup);
	printf("DELETE | ANS = Exist , ACTUAL = Not exist 	: %d\n", as->numDeleteExistButNoExist);
	printf("DELETE | ANS = Not exist , ACTUAL = Exist 	: %d\n", as->numDeleteNoExistButExist);
	printf("SCAN   | ANS = Not found , ACTUAL = Found 	: %d\n", as->numScanNotFoundButFound);
	printf("SCAN   | ANS = Found , ACTUAL = Not found 	: %d\n", as->numScanFoundButNotFound);
	printf("SCAN   | ANS = Same , ACTUAL = Not same 	: %d\n", as->numScanNotSameObject);
	printf("SCAN   | ANS = No next , ACTUAL = Next 		: %d\n", as->numScanUndercount);
	printf("SCAN   | ANS = Next , ACTUAL = No next 		: %d\n", as->numScanOvercount);
	printf("ETC    | NOT IMPLEMENTED		        : %d\n", as->numNotImplemented);
	printf("ETC    | UNKNOWN ERROR		                : %d\n", as->numEtcError);
	printf("TOTAL  |                              		: %d FAILURES\n", sum);
	printf("##########################################################################\n\n");
}

void mergeAnalytics(struct AnalyticsStruct* prev, struct AnalyticsStruct* cur){
	cur->numInsertDupButNoDup += prev->numInsertDupButNoDup;
	cur->numInsertNoDupButDup += prev->numInsertNoDupButDup;
	cur->numDeleteExistButNoExist += prev->numDeleteExistButNoExist;
	cur->numDeleteNoExistButExist += prev->numDeleteNoExistButExist;
	cur->numScanNotFoundButFound += prev->numScanNotFoundButFound;
	cur->numScanFoundButNotFound += prev->numScanFoundButNotFound;
	cur->numScanNotSameObject += prev->numScanNotSameObject;
	cur->numScanOvercount += prev->numScanOvercount;
	cur->numScanUndercount += prev->numScanUndercount;
	cur->numNotImplemented += prev->numNotImplemented;
	cur->numEtcError += prev->numEtcError;
}

void printPerformanceTest(struct perfTestResultStruct *ps, Four numTests, Four* totalTime) {
	int sum = 0;
	printf("\n############################# Test Result ################################\n");
	for(Four i = 0; i < numTests; i++) {
		char* keyName = ps[i].keyType == RANDINT ? "Random Integer" : ps[i].keyType == MONOINT ? "Monotonically Increasing Integer" : "Email";
		char* specName = ps[i].specType == A ? "A" : ps[i].specType == B ? "B" : ps[i].specType == C ? "C" : ps[i].specType == D ? "D" : "E" ;
		printf("%-35.40s | %s | 	%d μs\n", keyName, specName, ps[i].spendTime);
		sum += ps[i].spendTime;
	}
	*totalTime = sum;
	printf("%-37.40s | 	%d μs\n", "TOTAL", sum);
	printf("##########################################################################\n\n");
}

void printFinalScore(Four coverageScore, Four totalScore, Four totalTime) {
	printf("\n################################ Score ###################################\n");

	printf("%-37.40s | 	%d\n", "BASE", 10);

	printf("%-37.40s | 	%d\n", "COVERAGE", coverageScore);

	printf("%-37.40s | 	%d\n", "TOTAL", totalScore);

	printf("%-37.40s | 	%d\n", "Performance", totalTime);

	printf("##########################################################################\n\n");
}

Four gradeWorkload(struct AnalyticsStruct* as) {
	if(totalErrorCount(as) > 0) return 0;
	else return 6;
}

void fprintJSONResult(FILE* resultFp, Four totalScore, Four totalTime) {
	fprintf(resultFp, "{ \"Test\" : %d, \"Perf\" : %d }", totalScore, totalTime);
}
