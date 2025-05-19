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
 * Module: edubtm_Compact.c
 * 
 * Description:
 *  Two functions edubtm_CompactInternalPage() and edubtm_CompactLeafPage() are
 *  used to compact the internal page and the leaf page, respectively.
 *
 * Exports:
 *  void edubtm_CompactInternalPage(BtreeInternal*, Two)
 *  void edubtm_CompactLeafPage(BtreeLeaf*, Two)
 */


#include <string.h>
#include "EduBtM_common.h"
#include "EduBtM_Internal.h"



/*@================================
 * edubtm_CompactInternalPage()
 *================================*/
/*
 * Function: edubtm_CompactInternalPage(BtreeInternal*, Two)
 *
 * Description:
 * (Following description is for original ODYSSEUS/COSMOS BtM.
 *  For ODYSSEUS/EduCOSMOS EduBtM, refer to the EduBtM project manual.)
 *
 *  Reorganize the internal page to make sure the unused bytes in the page
 *  are located contiguously "in the middle", between the entries and the
 *  slot array. To compress out holes, entries must be moved toward the
 *  beginning of the page.
 *
 * Returns:
 *  None
 *
 * Side effects:
 *  The leaf page is reorganized to compact the space.
 */
void edubtm_CompactInternalPage(
    BtreeInternal       *apage,                 /* INOUT internal page to compact */
    Two                 slotNo)                 /* IN slot to go to the boundary of free space */
{
    BtreeInternal       tpage;                  /* temporay page used to save the given page */
    Two                 apageDataOffset;        /* where the next object is to be moved */
    Two                 len;                    /* length of the leaf entry */
    Two 		        alignedKlen;		    /* aligned length of the key length */
    Two                 i;                      /* index variable */
    btm_InternalEntry   *entry;                 /* an entry in leaf page */
    Two 		lastSlot;		/* position of last slot */

    tpage = *apage;
    apageDataOffset = 0;

    /* slotNo에 대응하는 index entry를 제외한 page의 모든 index entry들을 
       데이터 영역의 가장 앞부분부터 연속되게 저장함
       » Index entry 저장 순서: 대응하는 slot 번호 순 */
    lastSlot = apage->hdr.nSlots - 1;
    for ( i = 0; i <= lastSlot ; i++ ){
        if (i==slotNo) continue;
        /*
        --------------------------------------
        |     spid    |   klen   |   kval[]  |
        --------------------------------------
        ShortPageID       Two      entry->klen
        */
        entry = (btm_InternalEntry*)(tpage.data[tpage.slot[-i]]);
        alignedKlen = ALIGNED_LENGTH(sizeof(Two)+entry->klen);
        len = sizeof(ShortPageID)+ alignedKlen;
        memcpy((apage->data)+apageDataOffset, entry, len);
        apage->slot[-i] = apageDataOffset;
        apageDataOffset += len;
    }

    // slotNo에 대응하는 index entry를 데이터 영역 상에서의 마지막 index entry로 저장함
    if (slotNo != NIL){
        entry = (btm_InternalEntry*)(tpage.data[tpage.slot[-slotNo]]);
        alignedKlen = ALIGNED_LENGTH(sizeof(Two)+entry->klen);
        len = sizeof(ShortPageID)+ alignedKlen;
        memcpy((apage->data)+apageDataOffset, entry, len);
        apage->slot[-slotNo] = apageDataOffset;
        apageDataOffset += len;
    }
    
    // Page Header를 갱신함
    apage->hdr.unused = 0;
    apage->hdr.free = apageDataOffset;

    return (eNOERROR);

} /* edubtm_CompactInternalPage() */



/*@================================
 * edubtm_CompactLeafPage()
 *================================*/
/*
 * Function: void edubtm_CompactLeafPage(BtreeLeaf*, Two)
 *
 * Description:
 * (Following description is for original ODYSSEUS/COSMOS BtM.
 *  For ODYSSEUS/EduCOSMOS EduBtM, refer to the EduBtM project manual.)
 *
 *  Reorganizes the leaf page to make sure the unused bytes in the page
 *  are located contiguously "in the middle", between the entries and the
 *  slot array. To compress out holes, entries must be moved toward the
 *  beginning of the page.
 *	
 * Return Values :
 *  None
 *
 * Side Effects :
 *  The leaf page is reorganized to comact the space.
 */
void edubtm_CompactLeafPage(
    BtreeLeaf 		*apage,			/* INOUT leaf page to compact */
    Two       		slotNo)			/* IN slot to go to the boundary of free space */
{	
    BtreeLeaf 		tpage;			/* temporay page used to save the given page */
    Two                 apageDataOffset;        /* where the next object is to be moved */
    Two                 len;                    /* length of the leaf entry */
    Two                 i;                      /* index variable */
    btm_LeafEntry 	*entry;			/* an entry in leaf page */
    Two 		alignedKlen;		/* aligned length of the key length */
    Two 		lastSlot;		/* position of last slot */

    tpage = *apage;
    apageDataOffset = 0;

    /* slotNo에 대응하는 index entry를 제외한 page의 모든 index entry들을 
       데이터 영역의 가장 앞부분부터 연속되게 저장함
       » Index entry 저장 순서: 대응하는 slot 번호 순 */
    lastSlot = apage->hdr.nSlots - 1;
    for ( i = 0; i <= lastSlot ; i++ ){
        if (i==slotNo) continue;
        /*
        -----------------------------------------------------
        |  nObjects |  klen  |   key   |  value(Object ID)  |
        -----------------------------------------------------
            Two        Two   (aligned)klen    ObjectID
        */
        entry = (btm_LeafEntry*)(tpage.data[tpage.slot[-i]]);
        alignedKlen = ALIGNED_LENGTH(entry->klen);
        len = sizeof(Two)+ sizeof(Two)+ alignedKlen+ sizeof(ObjectID);
        memcpy((apage->data)+apageDataOffset, entry, len);
        apage->slot[-i] = apageDataOffset;
        apageDataOffset += len;
    }

    // slotNo에 대응하는 index entry를 데이터 영역 상에서의 마지막 index entry로 저장함
    if (slotNo != NIL){
        entry = (btm_LeafEntry*)&tpage.data[tpage.slot[-slotNo]];
        alignedKlen = ALIGNED_LENGTH(entry->klen);
        len = sizeof(Two)+ sizeof(Two)+ alignedKlen+ sizeof(ObjectID);
        memcpy(&apage->data[apageDataOffset], entry, len);
        apage->slot[-slotNo] = apageDataOffset;
        apageDataOffset += len;
    }

    // Page Header를 갱신함
    apage->hdr.unused = 0;
    apage->hdr.free = apageDataOffset;

    return (eNOERROR);
} /* edubtm_CompactLeafPage() */
