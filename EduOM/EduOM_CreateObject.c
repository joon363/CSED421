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
 * Module : EduOM_CreateObject.c
 * 
 * Description :
 *  EduOM_CreateObject() creates a new object near the specified object.
 *
 * Exports:
 *  Four EduOM_CreateObject(ObjectID*, ObjectID*, ObjectHdr*, Four, char*, ObjectID*)
 */

#include <string.h>
#include "EduOM_common.h"
#include "RDsM.h"		/* for the raw disk manager call */
#include "BfM.h"		/* for the buffer manager call */
#include "EduOM_Internal.h"

/*@================================
 * EduOM_CreateObject()
 *================================*/
/*
 * Function: Four EduOM_CreateObject(ObjectID*, ObjectID*, ObjectHdr*, Four, char*, ObjectID*)
 * 
 * Description :
 * (Following description is for original ODYSSEUS/COSMOS OM.
 *  For ODYSSEUS/EduCOSMOS EduOM, refer to the EduOM project manual.)
 *
 * (1) What to do?
 * EduOM_CreateObject() creates a new object near the specified object.
 * If there is no room in the page holding the specified object,
 * it trys to insert into the page in the available space list. If fail, then
 * the new object will be put into the newly allocated page.
 *
 * (2) How to do?
 *	a. Read in the near slotted page
 *	b. See the object header
 *	c. IF large object THEN
 *	       call the large object manager's lom_ReadObject()
 *	   ELSE 
 *		   IF moved object THEN 
 *				call this function recursively
 *		   ELSE 
 *				copy the data into the buffer
 *		   ENDIF
 *	   ENDIF
 *	d. Free the buffer page
 *	e. Return
 *
 * Returns:
 *  error code
 *    eBADCATALOGOBJECT_OM
 *    eBADLENGTH_OM
 *    eBADUSERBUF_OM
 *    some error codes from the lower level
 *
 * Side Effects :
 *  0) A new object is created.
 *  1) parameter oid
 *     'oid' is set to the ObjectID of the newly created object.
 */
Four EduOM_CreateObject(
    ObjectID  *catObjForFile,	/* IN file in which object is to be placed */
    ObjectID  *nearObj,		/* IN create the new object near this object */
    ObjectHdr *objHdr,		/* IN from which tag is to be set */
    Four      length,		/* IN amount of data */
    char      *data,		/* IN the initial data for the object */
    ObjectID  *oid)		/* OUT the object's ObjectID */
{
    Four        e;		/* error number */
    ObjectHdr   objectHdr;	/* ObjectHdr with tag set from parameter */


    /*@ parameter checking */
    
    if (catObjForFile == NULL) ERR(eBADCATALOGOBJECT_OM);
    
    if (length < 0) ERR(eBADLENGTH_OM);

    if (length > 0 && data == NULL) return(eBADUSERBUF_OM);

	/* Error check whether using not supported functionality by EduOM */
	if(ALIGNED_LENGTH(length) > LRGOBJ_THRESHOLD) ERR(eNOTSUPPORTED_EDUOM);
    
    e = eNOERROR;
    
    // 삽입할object의 header를 초기화함
    // – properties := 0x0
    // – length := 0
    // – 파라미터로주어진objHdr가NULL이아닌경우, tag := objHdr에 저장된 tag 값
    // – 파라미터로주어진objHdr가NULL인경우, » tag := 0
    objectHdr.properties=0x0;
    objectHdr.length=0;
    objectHdr.tag=objHdr==NULL?0:objHdr->tag;

    // eduom_CreateObject()를 호출하여 page에 object를 삽입하고, 
    // 삽입된 object의 ID를 반환함
    e = eduom_CreateObject(
        catObjForFile,	/* IN file in which object is to be placed */
        nearObj,	    /* IN create the new object near this object */
        &objectHdr,	    /* IN from which tag & properties are set */
        length,		    /* IN amount of data */
        data,		    /* IN the initial data for the object */
        oid             /* OUT the object's ObjectID */
    );
    return(e);
}

/*@================================
 * eduom_CreateObject()
 *================================*/
/*
 * Function: Four eduom_CreateObject(ObjectID*, ObjectID*, ObjectHdr*, Four, char*, ObjectID*)
 *
 * Description :
 * (Following description is for original ODYSSEUS/COSMOS OM.
 *  For ODYSSEUS/EduCOSMOS EduOM, refer to the EduOM project manual.)
 *
 *  eduom_CreateObject() creates a new object near the specified object; the near
 *  page is the page holding the near object.
 *  If there is no room in the near page and the near object 'nearObj' is not
 *  NULL, a new page is allocated for object creation (In this case, the newly
 *  allocated page is inserted after the near page in the list of pages
 *  consiting in the file).
 *  If there is no room in the near page and the near object 'nearObj' is NULL,
 *  it trys to create a new object in the page in the available space list. If
 *  fail, then the new object will be put into the newly allocated page(In this
 *  case, the newly allocated page is appended at the tail of the list of pages
 *  cosisting in the file).
 *
 * Returns:
 *  error Code
 *    eBADCATALOGOBJECT_OM
 *    eBADOBJECTID_OM
 *    some errors caused by fuction calls
 */
Four eduom_CreateObject(
                        ObjectID	*catObjForFile,	/* IN file in which object is to be placed */
                        ObjectID 	*nearObj,	/* IN create the new object near this object */
                        ObjectHdr	*objHdr,	/* IN from which tag & properties are set */
                        Four	length,		/* IN amount of data */
                        char	*data,		/* IN the initial data for the object */
                        ObjectID	*oid)		/* OUT the object's ObjectID */
{
    Four        e;		/* error number */
    Four	neededSpace;	/* space needed to put new object [+ header] */
    SlottedPage *apage;		/* pointer to the slotted page buffer */
    Four        alignedLen;	/* aligned length of initial data */
    Boolean     needToAllocPage;/* Is there a need to alloc a new page? */
    PageID      pid;            /* PageID in which new object to be inserted */
    PageID      nearPid;
    Four        firstExt;	/* first Extent No of the file */
    Object      *obj;		/* point to the newly created object */
    Two         i;		/* index variable */
    sm_CatOverlayForData *catEntry; /* pointer to data file catalog information */
    SlottedPage *catPage;	/* pointer to buffer containing the catalog */
    FileID      fid;		/* ID of file where the new object is placed */
    Two         eff;		/* extent fill factor of file */
    Boolean     isTmp;
    PhysicalFileID pFid;
    VolNo volNo;			/* a temporary var for volNo */
    PageNo pageNo;		/* a temporary var for next page's PageNo */
    PageNo *availListPageNo = -1;
    
    
    /*@ parameter checking */
    
    if (catObjForFile == NULL) ERR(eBADCATALOGOBJECT_OM);
    
    if (objHdr == NULL) ERR(eBADOBJECTID_OM);
    
    /* Error check whether using not supported functionality by EduOM */
    if(ALIGNED_LENGTH(length) > LRGOBJ_THRESHOLD) ERR(eNOTSUPPORTED_EDUOM);
    
    // File을 구성하는 page들 중 파라미터로 지정한 object와 같은 (또는 인접한) page에 새로운 object를 삽입하고, 삽입된 object의 ID를 반환함

    // Get sm_CatOverlayForData from File
    volNo = catObjForFile->volNo;
    pageNo = catObjForFile->pageNo;
    MAKE_PHYSICALFILEID(pFid, volNo, pageNo);
    e = BfM_GetTrain(&pFid, &catPage, PAGE_BUF);
    if(e < eNOERROR) ERR(e);
    GET_PTR_TO_CATENTRY_FOR_DATA(catObjForFile, catPage, catEntry);
    MAKE_PHYSICALFILEID(pFid, catEntry->fid.volNo, catEntry->firstPage);
    e = RDsM_PageIdToExtNo(&pFid, &firstExt);
    if (e < eNOERROR) ERR(e);

    // • Object 삽입을 위해 필요한 자유 공간의 크기를 계산함
    //     – sizeof(ObjectHdr) + align 된 object 데이터 영역의 크기 + sizeof(SlottedPageSlot)
    
    alignedLen = ALIGNED_LENGTH(length);
    neededSpace = sizeof(ObjectHdr) + alignedLen + sizeof(SlottedPageSlot);

    // – 파라미터로 주어진 nearObj가 NULL이 아닌 경우,
    if(nearObj!=NULL){
        // nearObj 페이지 획득
        volNo = nearObj->volNo;
        pageNo = nearObj->pageNo;
        MAKE_PAGEID(nearPid, volNo,pageNo);
        e = BfM_GetTrain(&nearPid, &apage, PAGE_BUF);
        if (e < eNOERROR) ERR(e);

        // » nearObj가 저장된 page에 여유 공간이 있는 경우,
        if (SP_FREE(apage) >= neededSpace){
            // 해당 page를 object를 삽입할 page로 선정함
            MAKE_PAGEID(pid, nearPid.volNo, nearPid.pageNo);
            // 선정된 page를 현재 available space list에서 삭제함
            e = om_RemoveFromAvailSpaceList(catObjForFile, &pid, apage);
            if (e < eNOERROR) ERR(e);
            
            // 필요시 선정된 page를 compact 함
            // CFREE: contiguous free area -> compact의 기준으로 사용
            if (SP_CFREE(apage) < neededSpace) {
                e = EduOM_CompactPage(apage, nearObj->slotNo);
                if(e < eNOERROR) ERR(e);
            }
        }

        // » nearObj가 저장된 page에 여유 공간이 없는 경우,
        else {
            // nearObj 페이지 해제
            e = BfM_FreeTrain(&nearPid, PAGE_BUF);
            if (e < eNOERROR) ERR(e);
            
            //  • 새로운 page를 할당받아 object를 삽입할 page로 선정함
            e = RDsM_AllocTrains(catEntry->fid.volNo, firstExt, &nearPid, catEntry->eff, 1, PAGESIZE2, &pid);
            if (e < eNOERROR) ERR(e);
            
            // 선정된 page의 header를 초기화함
            e = BfM_GetNewTrain(&pid, &apage, PAGE_BUF);
            if (e < eNOERROR) ERR(e);
            apage->header.flags = 0x2;
            apage->header.free = 0;
            apage->header.unused = 0;
            apage->header.fid = catEntry->fid;
            
            // file 구성 page들로 이루어진 list에서 nearObj가 저장된 page의 다음 page로 삽입
            e = om_FileMapAddPage(catObjForFile, &nearPid, &pid);
            if (e < eNOERROR) ERR(e);
        }
    }
    //– 파라미터로 주어진 nearObj가 NULL인 경우,
    else{
        // list 확인
        if (neededSpace <= SP_10SIZE){
            availListPageNo = catEntry->availSpaceList10;
            MAKE_PAGEID(pid, pFid.volNo, catEntry->availSpaceList10);
        }  
        else if (neededSpace <= SP_20SIZE){
            availListPageNo = catEntry->availSpaceList10;
            MAKE_PAGEID(pid, pFid.volNo, catEntry->availSpaceList10);
        }       
        else if (neededSpace <= SP_30SIZE){
            availListPageNo = catEntry->availSpaceList20;
            MAKE_PAGEID(pid, pFid.volNo, catEntry->availSpaceList20);
        }    
        else if (neededSpace <= SP_40SIZE){
            availListPageNo = catEntry->availSpaceList30;
            MAKE_PAGEID(pid, pFid.volNo, catEntry->availSpaceList30);
        }  
        else if (neededSpace <= SP_50SIZE){
            availListPageNo = catEntry->availSpaceList40;
            MAKE_PAGEID(pid, pFid.volNo, catEntry->availSpaceList40);
        }
        else {
            availListPageNo = catEntry->availSpaceList50;
            MAKE_PAGEID(pid, pFid.volNo, catEntry->availSpaceList50);
        }

        // » Object 삽입을 위해 필요한 자유 공간의 크기에 알맞은 available space list가 존재하는 경우,
        if (availListPageNo != -1){
            // 해당 available space list의 첫 번째 page를 object를 삽입할 page로 선정함
            MAKE_PAGEID(pid, pFid.volNo, availListPageNo);
            e = BfM_GetTrain(&pid, &apage, PAGE_BUF);
            if (e < eNOERROR) ERR(e);

            // 선정된 page를 현재 available space list에서 삭제함
            e = om_RemoveFromAvailSpaceList(catObjForFile, &pid, apage);
            if (e < eNOERROR) ERR(e);

            // 필요시 선정된 page를 compact 함
            // Cfree: contiguous free area -> compact 기준
            if (SP_CFREE(apage) < neededSpace) {
                e = EduOM_CompactPage(apage, nearObj->slotNo);
                if(e < eNOERROR) ERR(e);
            }
        }       

       //Object 삽입을 위해 필요한 자유 공간의 크기에 알맞은 available space list가 존재하지 않고, 
       else {
            //file의 마지막 page에 여유 공간이 있는 경우,
            MAKE_PAGEID(pid, pFid.volNo, catEntry->lastPage);
            e = BfM_GetTrain(&pid, &apage, PAGE_BUF);
            if (e < eNOERROR) ERR(e);
            if (SP_FREE(apage) >= neededSpace){     
                // File의 마지막 page를 object를 삽입할 page로 선정함 
                // 선정된 page를 현재 available space list에서 삭제함
                e = om_RemoveFromAvailSpaceList(catObjForFile, &pid, apage);
                if(e < 0) ERR(e);        

                // • 필요시 선정된 page를 compact 함  
                if (SP_CFREE(apage) < neededSpace) {
                    e = EduOM_CompactPage(apage, nearObj->slotNo);
                    if(e < eNOERROR) ERR(e);
                }   
            }

            //file의 마지막 page에 여유 공간이 없는 경우,
            else {
                // 마지막 페이지 해제
                e = BfM_FreeTrain(&pid, PAGE_BUF);
                if (e < eNOERROR) ERR(e);
                
                // • 새로운 page를 할당받아 object를 삽입할 page로 선정함
                MAKE_PAGEID(nearPid, pFid.volNo, catEntry->lastPage);
                e = RDsM_AllocTrains(catEntry->fid.volNo, firstExt, &nearPid, catEntry->eff, 1, PAGESIZE2, &pid);
                if (e < eNOERROR) ERR(e);

                // • 선정된 page의 header를 초기화함
                e = BfM_GetNewTrain(&pid, &apage, PAGE_BUF);
                if (e < eNOERROR) ERR(e);
                apage->header.flags = 0x2;
                apage->header.free = 0;
                apage->header.unused = 0;
                apage->header.fid = catEntry->fid; 
                
                // • 선정된 page를 file의 구성 page들로 이루어진 list에서 마지막 page로 삽입함
                e = om_FileMapAddPage(catObjForFile, &(catEntry->lastPage), &pid);
                if (e < eNOERROR) ERR(e);
            }
        }
    }

    // Slot array의 빈 slot 또는 새로운 slot 한 개를 할당 받아 복사한 object의 식별을 위한 정보를 저장함
    i = 0;
    while (apage->slot[-i].offset != EMPTYSLOT && i < apage->header.nSlots){
        i++;
    }
    if(i == apage->header.nSlots) {
        apage->header.nSlots++;
    }
    SlottedPageSlot* newSlot = &(apage->slot[-i]);
    newSlot->offset = apage->header.free;

    // – Object의 header를 갱신함
    //    » length := 데이터의 길이
    obj = &(apage->data[newSlot->offset]);
    obj->header.properties = objHdr->properties;
    obj->header.tag = objHdr->tag;
    obj->header.length = length;
    e = om_GetUnique(&pid, &(newSlot->unique));
    if (e < eNOERROR) ERR(e);
    MAKE_OBJECTID(*oid, pid.volNo, pid.pageNo, i, newSlot->unique);

    // 선정한 page의 contiguous free area에 object를 복사함
    memcpy(obj->data, data, length);

    // – Page의 header를 갱신함
    // » Object 삽입으로 인해 사용되는 자유 공간의 크기
    //   = sizeof(ObjectHdr) + align 된 object 데이터 영역의 크기 + 새로운 slot을 할당 받은 경우의 sizeof(SlottedPageSlot)
    apage->header.free += sizeof(ObjectHdr) + alignedLen;
    
    
    // – Page를 알맞은 available space list에 삽입함
    e = om_PutInAvailSpaceList(catObjForFile, &pid, apage);
    if (e < eNOERROR) ERR(e);

    e = BfM_SetDirty(&pid, PAGE_BUF);
    if (e < eNOERROR) ERR(e);
    e = BfM_SetDirty(&pFid, PAGE_BUF);
    if (e < eNOERROR) ERR(e);
    
    e = BfM_FreeTrain(catObjForFile, PAGE_BUF);
    if (e < eNOERROR) ERR(e);
    e = BfM_FreeTrain(&pid, PAGE_BUF);
    if (e < eNOERROR) ERR(e);
    

    return(eNOERROR);
    
} /* eduom_CreateObject() */
