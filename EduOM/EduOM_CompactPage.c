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
 * Module : EduOM_CompactPage.c
 * 
 * Description : 
 *  EduOM_CompactPage() reorganizes the page to make sure the unused bytes
 *  in the page are located contiguously "in the middle", between the tuples
 *  and the slot array. 
 *
 * Exports:
 *  Four EduOM_CompactPage(SlottedPage*, Two)
 */


#include <string.h>
#include "EduOM_common.h"
#include "LOT.h"
#include "EduOM_Internal.h"



/*@================================
 * EduOM_CompactPage()
 *================================*/
/*
 * Function: Four EduOM_CompactPage(SlottedPage*, Two)
 * 
 * Description : 
 * (Following description is for original ODYSSEUS/COSMOS OM.
 *  For ODYSSEUS/EduCOSMOS EduOM, refer to the EduOM project manual.)
 *
 *  (1) What to do?
 *  EduOM_CompactPage() reorganizes the page to make sure the unused bytes
 *  in the page are located contiguously "in the middle", between the tuples
 *  and the slot array. To compress out holes, objects must be moved toward
 *  the beginning of the page.
 *
 *  (2) How to do?
 *  a. Save the given page into the temporary page
 *  b. FOR each nonempty slot DO
 *	Fill the original page by copying the object from the saved page
 *          to the data area of original page pointed by 'apageDataOffset'
 *	Update the slot offset
 *	Get 'apageDataOffet' to point the next moved position
 *     ENDFOR
 *   c. Update the 'freeStart' and 'unused' field of the page
 *   d. Return
 *	
 * Returns:
 *  error code
 *    eNOERROR
 *
 * Side Effects :
 *  The slotted page is reorganized to comact the space.
 */
Four EduOM_CompactPage(
    SlottedPage	*apage,		/* IN slotted page to compact */
    Two         slotNo)		/* IN slotNo to go to the end */
{
    SlottedPage	tpage;		/* temporay page used to save the given page */
    Object *obj;		/* pointer to the object in the data area */
    Two    apageDataOffset;	/* where the next object is to be moved */
    Four   len;			/* length of object + length of ObjectHdr */
    Four   alignedLen=0;			/* ailgned length of ObjectHdr */
    Two    lastSlot;		/* last non empty slot */
    Two    i;			/* index variable */

    // Page의 데이터 영역의 모든 자유공간이 연속된 하나의 contiguous free area를 형성하도록 object들의 offset를 조정함
    // save given page to temporary page
    tpage = *apage;
    
    // • 파라미터로 주어진 slotNo가 NIL (-1) 이 아닌 경우,
    //      – slotNo에 대응하는 object를 제외한 page의 모든 object들을 데이터 영역의 가장 앞부분부터 연속되게 저장함
    //         » Object 저장 순서: 대응하는 slot 번호 순
    //      – slotNo에 대응하는 object를 데이터 영역 상에서의 마지막 object로 저장함
    // • 파라미터로 주어진 slotNo가 NIL (-1) 인 경우,
    //      – Page의 모든 object들을 데이터 영역의 가장 앞부분부터 연속되게 저장함
    //         » Object 저장 순서: 대응하는 slot 번호 순
    apageDataOffset=0;
    for (i = 0; i < tpage.header.nSlots; i++) {
        if (tpage.slot[-i].offset == EMPTYSLOT || (slotNo!=NIL && slotNo == i)) continue;
        obj = &(tpage.data[tpage.slot[-i].offset]);
        alignedLen= ALIGNED_LENGTH(obj->header.length);
        len = alignedLen + sizeof(ObjectHdr);
        memcpy((apage->data)+apageDataOffset, obj, len);
        apage->slot[-i].offset = apageDataOffset;
        apageDataOffset += len;
    }

    if (slotNo != NIL) {
        obj = &(tpage.data[tpage.slot[-slotNo].offset]);
        alignedLen= ALIGNED_LENGTH(obj->header.length);
        len = alignedLen + sizeof(ObjectHdr);
        memcpy((apage->data)+apageDataOffset, obj, len);
        apage->slot[-slotNo].offset = apageDataOffset;
        apageDataOffset += len;
    }
    
    // • Page header를 갱신함
    apage->header.free = apageDataOffset;
    apage->header.unused = 0;

    return(eNOERROR);
    
} /* EduOM_CompactPage */
