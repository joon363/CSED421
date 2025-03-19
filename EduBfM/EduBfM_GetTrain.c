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
 * Module: EduBfM_GetTrain.c
 *
 * Description : 
 *  Return a buffer which has the disk content indicated by `trainId'.
 *
 * Exports:
 *  Four EduBfM_GetTrain(TrainID *, char **, Four)
 */


#include "EduBfM_common.h"
#include "EduBfM_Internal.h"



/*@================================
 * EduBfM_GetTrain()
 *================================*/
/*
 * Function: EduBfM_GetTrain(TrainID*, char**, Four)
 *
 * Description : 
 * (Following description is for original ODYSSEUS/COSMOS BfM.
 *  For ODYSSEUS/EduCOSMOS EduBfM, refer to the EduBfM project manual.)
 *
 *  Return a buffer which has the disk content indicated by `trainId'.
 *  Before the allocation of a buffer, look up the train in the buffer
 *  pool using hashing mechanism.   If the train already  exist in the pool
 *  then simply return it and set the reference bit of the correponding
 *  buffer table entry.   Otherwise, i.e. the train does not exist in the
 *  pool, allocate a buffer (a buffer selected as victim may be forced out
 *  by the buffer replacement algorithm), read a disk train into the 
 *  selected buffer train, and return it.
 *
 * Returns:
 *  error code
 *    eBADBUFFER_BFM - Invalid Buffer
 *    eBADBUFFERTYPE_BFM - Invalid Buffer type
 *    some errors caused by function calls
 *
 * Side effects:
 *  1) parameter retBuf
 *     pointer to buffer holding the disk train indicated by `trainId'
 */
Four EduBfM_GetTrain(
    TrainID             *trainId,               /* IN train to be used */
    char                **retBuf,               /* OUT pointer to the returned buffer */
    Four                type )                  /* IN buffer type */
{
    Four                e;                      /* for error */
    Four                index;                  /* index of the buffer pool */


    /*@ Check the validity of given parameters */
    /* Some restrictions may be added         */
    if(retBuf == NULL) ERR(eBADBUFFER_BFM);

    /* Is the buffer type valid? */
    if(IS_BAD_BUFFERTYPE(type)) ERR(eBADBUFFERTYPE_BFM);	

    // Fix 할 page/train이 bufferPool에 존재하지 않는 경우,
    index = edubfm_LookUp(trainId,type);
    if(index==NIL){
        // bufferPool에서 page/train을 저장할 buffer element 한 개를 할당 받음
        index=edubfm_AllocTrain(type);

        //Page/train을 disk로부터 읽어와서 할당 받은 buffer element에 저장함
        e= edubfm_ReadTrain(trainId, BI_BUFFER(type, index),type);
        if (e != eNOERROR) return e;

        //할당받은buffer element에 대응하는 bufTable element를 갱신함
        BI_KEY(type,index).pageNo=trainId->pageNo;
        BI_KEY(type,index).volNo=trainId->volNo;
        BI_BITS(type, index) = BI_BITS(type, index) | REFER;
        BI_FIXED(type, index)=1; 

        //할당받은buffer element의 array index를 hashTable에 삽입함– 
        e = edubfm_Insert(&BI_KEY(type, index), index, type);    
        if (e != eNOERROR) return e;
    }
    else{
        //해당page/train이 저장된 buffer element에 대응하는 bufTable element를 갱신함
        BI_FIXED(type, index)=BI_FIXED(type, index)+1;
        BI_BITS(type, index) = BI_BITS(type, index) | REFER;
    }

    // buffer element에 대한 포인터를 반환함
    *retBuf = BI_BUFFER(type, index);

    return(eNOERROR);   /* No error */

}  /* EduBfM_GetTrain() */
