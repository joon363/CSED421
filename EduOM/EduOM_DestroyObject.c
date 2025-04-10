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
 * Module : EduOM_DestroyObject.c
 * 
 * Description : 
 *  EduOM_DestroyObject() destroys the specified object.
 *
 * Exports:
 *  Four EduOM_DestroyObject(ObjectID*, ObjectID*, Pool*, DeallocListElem*)
 */

#include "EduOM_common.h"
#include "Util.h"		/* to get Pool */
#include "RDsM.h"
#include "BfM.h"		/* for the buffer manager call */
#include "LOT.h"		/* for the large object manager call */
#include "EduOM_Internal.h"

/*@================================
 * EduOM_DestroyObject()
 *================================*/
/*
 * Function: Four EduOM_DestroyObject(ObjectID*, ObjectID*, Pool*, DeallocListElem*)
 * 
 * Description : 
 * (Following description is for original ODYSSEUS/COSMOS OM.
 *  For ODYSSEUS/EduCOSMOS EduOM, refer to the EduOM project manual.)
 *
 *  (1) What to do?
 *  EduOM_DestroyObject() destroys the specified object. The specified object
 *  will be removed from the slotted page. The freed space is not merged
 *  to make the contiguous space; it is done when it is needed.
 *  The page's membership to 'availSpaceList' may be changed.
 *  If the destroyed object is the only object in the page, then deallocate
 *  the page.
 *
 *  (2) How to do?
 *  a. Read in the slotted page
 *  b. Remove this page from the 'availSpaceList'
 *  c. Delete the object from the page
 *  d. Update the control information: 'unused', 'freeStart', 'slot offset'
 *  e. IF no more object in this page THEN
 *	   Remove this page from the filemap List
 *	   Dealloate this page
 *    ELSE
 *	   Put this page into the proper 'availSpaceList'
 *    ENDIF
 * f. Return
 *
 * Returns:
 *  error code
 *    eBADCATALOGOBJECT_OM
 *    eBADOBJECTID_OM
 *    eBADFILEID_OM
 *    some errors caused by function calls
 */
Four EduOM_DestroyObject(
    ObjectID *catObjForFile,	/* IN file containing the object */
    ObjectID *oid,		/* IN object to destroy */
    Pool     *dlPool,		/* INOUT pool of dealloc list elements */
    DeallocListElem *dlHead)	/* INOUT head of dealloc list */
{
    Four        e;		/* error number */
    Two         i;		/* temporary variable */
    FileID      fid;		/* ID of file where the object was placed */
    PageID	pid;		/* page on which the object resides */
    SlottedPage *apage;		/* pointer to the buffer holding the page */
    Four        offset;		/* start offset of object in data area */
    Object      *obj;		/* points to the object in data area */
    Four        alignedLen;	/* aligned length of object */
    Boolean     last;		/* indicates the object is the last one */
    SlottedPage *catPage;	/* buffer page containing the catalog object */
    sm_CatOverlayForData *catEntry; /* overlay structure for catalog object access */
    DeallocListElem *dlElem;	/* pointer to element of dealloc list */
    PhysicalFileID pFid;	/* physical ID of file */
    VolNo volNo;			/* a temporary var for volNo */
    PageNo pageNo;		/* a temporary var for next page's PageNo */
    
    

    /*@ Check parameters. */
    if (catObjForFile == NULL) ERR(eBADCATALOGOBJECT_OM);

    if (oid == NULL) ERR(eBADOBJECTID_OM);

    // File을 구성하는 page에서 object를 삭제함

    // Get sm_CatOverlayForData from File
    volNo = catObjForFile->volNo;
    pageNo = catObjForFile->pageNo;
    MAKE_PHYSICALFILEID(pFid, volNo, pageNo);
    e = BfM_GetTrain(&pFid, &catPage, PAGE_BUF);
    if(e < eNOERROR) ERR(e);
    GET_PTR_TO_CATENTRY_FOR_DATA(catObjForFile, catPage, catEntry);

    // Get object page
    MAKE_PAGEID(pid, oid->volNo, oid->pageNo);
    e = BfM_GetTrain(&pid, &apage, PAGE_BUF);
    if (e < eNOERROR) ERR(e);

    // • 삭제할 object가 저장된 page를 현재 available space list에서 삭제함
    e = om_RemoveFromAvailSpaceList(catObjForFile, &pid, apage);
    if (e < eNOERROR) ERR(e);

    // obj 삭제 전 정보 저장
    obj = &(apage->data[apage->slot[-(oid->slotNo)].offset]);
    alignedLen = ALIGNED_LENGTH(obj->header.length);

    // • 삭제할 object에 대응하는 slot을 사용하지 않는 빈 slot으로 설정함
    //      – Slot의 offset := EMPTYSLOT
    apage->slot[-(oid)->slotNo].offset = EMPTYSLOT;
    apage->slot[-(oid)->slotNo].unique = 0;

    // • Page header를 갱신함
    //  – 삭제할 object에 대응하는 slot이 slot array의 마지막 slot인 경우, slot array의 크기를 갱신함
    //     » nSlots := 사용 중인 slot들 중 마지막 slot의 번호 + 1
    //  – 삭제할 object의 데이터 영역상에서의 offset에 따라 free 또는 unused 변수를 갱신함
    //      » Object 삭제로 인해 확보되는 자유 공간의 크기
    //      = sizeof(ObjectHdr) + align 된 object 데이터 영역의 크기 + slot array 크기 갱신에 따라 확보되는 자유 공간의 크기
    if (oid->slotNo == apage->header.nSlots - 1) {
        apage->header.nSlots--;
        apage->header.free -= sizeof(ObjectHdr) + alignedLen;
    }
    else{
        apage->header.unused += sizeof(ObjectHdr) + alignedLen;
    }

    // • 삭제된 object가 page의 유일한 object이고, 해당 page가 file의 첫 번째 page가 아닌 경우,
    if (apage->header.nSlots == 0 && apage->header.pid.pageNo != catEntry->firstPage){
        // – Page를 file 구성 page들로 이루어진 list에서 삭제함
        e = om_FileMapDeletePage(catObjForFile, &pid);
        if (e < eNOERROR) ERR(e);

        //  – 해당 page를 deallocate 함
        //      » 파라미터로 주어진 dlPool에서 새로운 dealloc list element 한 개를 할당 받음
        //          • Dealloc list: deallocate 할 page들의 linked list
        e = Util_getElementFromPool(dlPool, &dlElem);
        if (e < eNOERROR) ERR(e);
        
        // » 할당받은 element에 deallocate 할 page 정보를 저장함
        dlElem->type = DL_PAGE;
        dlElem->elem.pid = pid;

        // » Deallocate 할 page 정보가 저장된 element를 dealloc list의 첫 번째 element로 삽입함
        // 링크드 리스트 기반 구현
        dlElem->next = dlHead->next;
        dlHead->next = dlElem;        
    }

    // • 삭제된 object가 page의 유일한 object가 아니거나, 해당 page가 file의 첫 번째 page인 경우,
    //      – Page를 알맞은 available space list에 삽입함
    else {
        om_PutInAvailSpaceList(catObjForFile, &pid, apage);
        if (e < eNOERROR) ERR(e);
    }
    
    e = BfM_SetDirty(&pid, PAGE_BUF);
    if (e < eNOERROR) ERR(e);
    e = BfM_FreeTrain(&pid, PAGE_BUF);
    if (e < eNOERROR) ERR(e);
    e = BfM_FreeTrain(catObjForFile, PAGE_BUF);
    if (e < eNOERROR) ERR(e);

    return(eNOERROR);
    
} /* EduOM_DestroyObject() */
