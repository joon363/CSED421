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
#ifndef _EDUBTM_TESTMODULE_H_
#define _EDUBTM_TESTMODULE_H_


#include "BfM.h"
#include "Util_pool.h"



/*
 * Definition for EduBtM Test Module
 */
#define MAX_DEVICES_IN_VOLUME 20
#define ARRAYINDEX 0
#define NUMOFINSERTEDOBJECT	200
#define NUMOFPLAYER 1000
#define MAXPLAYERNAME 60
#define MAXFILENAME 255
#define MAXKEY 60
#define MAXPERFTEST 30

#define f(x) #x

DeallocListElem dlHead;
extern Pool dlPool;
FILE *logFp;
FILE *resultFp;

/*
 * Type Definition about transaction
 */
typedef struct {        /* 8 byte unsigned integer */
    UFour high;
    UFour low;
} XactID;

typedef enum { X_BROWSE_BROWSE, X_CS_BROWSE, X_CS_CS, X_RR_BROWSE, X_RR_CS, X_RR_RR } ConcurrencyLevel; /* isolation degree */

/*
 *  Definition for Physical Index ID
 */
typedef PageID  PhysicalIndexID;  /* use the root Page's PageID as the physical IndexID */

/*
** Definition for Index ID
*/
typedef LogicalID IndexID;


/*
** Definition of Test options
*/
typedef enum {COVERAGE=0x1, PERFORMANCE=0x2} TestType;

typedef enum {RANDINT=0x1, MONOINT=0x2, EMAIL=0x3} KeyType;

typedef enum {LOAD=0x1, TXNS=0x2} WorkloadType;

typedef enum {A=0x1, B=0x2, C=0x3, D=0x4, E=0x5} SpecType;


/*
** Definition of Query Operator
*/
typedef enum {INSERT=0x1, DELETE=0x2, SCAN=0x3, NOP=0x4} Opcode;

/*@
 * Function Prototypes
 */
/* Interface Function Prototypes */
Four LRDS_Init(void);
Four LRDS_AllocHandle(Four*);
Four LRDS_FormatDataVolume(Four, char**, char*, Four, Two, Four*, Four);
Four LRDS_Mount(Four, char**, Four*);
Four LRDS_BeginTransaction(XactID*, ConcurrencyLevel); /* COOKIE09NOV1999 */
Four LRDS_CommitTransaction(XactID*);
Four LRDS_AbortTransaction(XactID*);
Four LRDS_Dismount(Four);
Four LRDS_FreeHandle(Four);
Four LRDS_Final(void);

Four EduBtM_Test(Four, Four);


#endif /* _EDUBTM_TESTMODULE_H_ */
