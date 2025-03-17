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
 * Module: edubfm_AllocTrain.c
 *
 * Description : 
 *  Allocate a new buffer from the buffer pool.
 *
 * Exports:
 *  Four edubfm_AllocTrain(Four)
 */


#include <errno.h>
#include "EduBfM_common.h"
#include "EduBfM_Internal.h"


extern CfgParams_T sm_cfgParams;

/*@================================
 * edubfm_AllocTrain()
 *================================*/
/*
 * Function: Four edubfm_AllocTrain(Four)
 *
 * Description : 
 * (Following description is for original ODYSSEUS/COSMOS BfM.
 *  For ODYSSEUS/EduCOSMOS EduBfM, refer to the EduBfM project manual.)
 *
 *  Allocate a new buffer from the buffer pool.
 *  The used buffer pool is specified by the parameter 'type'.
 *  This routine uses the second chance buffer replacement algorithm
 *  to select a victim.  That is, if the reference bit of current checking
 *  entry (indicated by BI_NEXTVICTIM(type), macro for
 *  bufInfo[type].nextVictim) is set, then simply clear
 *  the bit for the second chance and proceed to the next entry, otherwise
 *  the current buffer indicated by BI_NEXTVICTIM(type) is selected to be
 *  returned.
 *  Before return the buffer, if the dirty bit of the victim is set, it 
 *  must be force out to the disk.
 *
 * Returns;
 *  1) An index of a new buffer from the buffer pool
 *  2) Error codes: Negative value means error code.
 *     eNOUNFIXEDBUF_BFM - There is no unfixed buffer.
 *     some errors caused by fuction calls
 */
Four edubfm_AllocTrain(
    Four 	type)			/* IN type of buffer (PAGE or TRAIN) */
{
    Four 	e;			/* for error */
    Four 	victim;			/* return value */
    Four 	i;
    Two     nBuf;
    

	/* Error check whether using not supported functionality by EduBfM */
	if(sm_cfgParams.useBulkFlush) ERR(eNOTSUPPORTED_EDUBFM);

    // Second chance buffer replacement algorithm을 사용
    nBuf = BI_NBUFS(type);
    for (i = BI_NEXTVICTIM(type); i < nBuf * 2; i=(i + 1) % BI_NBUFS(type)) {
        // 대응하는 fixed 변수값이 0인 buffer element들을 순차적으로 방문함
        if (BI_FIXED(type, i) == 0){
            // 각buffer element 방문시 REFER bit를 검사하여 
            // 동일한 buffer element를 2회째 방문한경우(REFER bit == 0), 
            // 해당 buffer element를 할당 대상으로 선정하고, 
            // 아닌경우(REFER bit == 1), REFER bit를 0으로 설정함
            if (BI_BITS(type, i) & REFER) {
                BI_BITS(type, i) &= ~REFER;
            }
            else {
                victim = i;
                BI_NEXTVICTIM(type) = (victim + 1) % BI_NBUFS(type);
                // 선정된 buffer element에 저장되어 있던 page/train이 수정된 경우, 기존 buffer element의 내용을 disk로 flush함
                if (BI_BITS(type, victim) & DIRTY) {
                    edubfm_FlushTrain(&BI_KEY(type, victim), type);
                }
                // 선정된 buffer element와 관련된 데이터 구조를 초기화함
                BI_BITS(type, victim) = ALL_0;

                // 선정된 buffer element의 array index (hashTable entry) 를 hashTable에서 삭제함
                edubfm_Delete(&BI_KEY(type, victim), type);
                break;
            }
        }
    }

    return victim;
}  /* edubfm_AllocTrain */
