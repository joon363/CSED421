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
 * Module: EduOM_PrevObject.c
 *
 * Description: 
 *  Return the previous object of the given current object.
 *
 * Exports:
 *  Four EduOM_PrevObject(ObjectID*, ObjectID*, ObjectID*, ObjectHdr*)
 */


#include "EduOM_common.h"
#include "BfM.h"
#include "EduOM_Internal.h"

/*@================================
 * EduOM_PrevObject()
 *================================*/
/*
 * Function: Four EduOM_PrevObject(ObjectID*, ObjectID*, ObjectID*, ObjectHdr*)
 *
 * Description: 
 * (Following description is for original ODYSSEUS/COSMOS OM.
 *  For ODYSSEUS/EduCOSMOS EduOM, refer to the EduOM project manual.)
 *
 *  Return the previous object of the given current object. Find the object in
 *  the same page which has the current object and  if there  is no previous
 *  object in the same page, find it from the previous page.
 *  If the current object is NULL, return the last object of the file.
 *
 * Returns:
 *  error code
 *    eBADCATALOGOBJECT_OM
 *    eBADOBJECTID_OM
 *    some errors caused by function calls
 *
 * Side effect:
 *  1) parameter prevOID
 *     prevOID is filled with the previous object's identifier
 *  2) parameter objHdr
 *     objHdr is filled with the previous object's header
 */
Four EduOM_PrevObject(
    ObjectID *catObjForFile,	/* IN informations about a data file */
    ObjectID *curOID,		/* IN a ObjectID of the current object */
    ObjectID *prevOID,		/* OUT the previous object of a current object */
    ObjectHdr*objHdr)		/* OUT the object header of previous object */
{
    Four e;			/* error */
    Two  i;			/* index */
    Two  firstObjIdx;			/* index of first object */
    Four offset;		/* starting offset of object within a page */
    PageID pid;			/* a page identifier */
    VolNo volNo;			/* a temporary var for volNo */
    PageNo pageNo;		/* a temporary var for previous page's PageNo */
    SlottedPage *apage;		/* a pointer to the data page */
    Object *obj;		/* a pointer to the Object */
    PhysicalFileID pFid;	/* file in which the objects are located */
    SlottedPage *catPage;	/* buffer page containing the catalog object */
    sm_CatOverlayForData *catEntry; /* overlay structure for catalog object access */



    /*@ parameter checking */
    if (catObjForFile == NULL) ERR(eBADCATALOGOBJECT_OM);
    
    if (prevOID == NULL) ERR(eBADOBJECTID_OM);

    /* 현재object의 이전 object의 ID를 반환함
     * 파라미터로주어진curOID가 NULL 인 경우,
     * – File의 마지막 page의 slot array 상에서의 마지막 object의 ID를 반환함
     * 파라미터로주어진curOID가 NULL 이 아닌경우,
     * – curOID에 대응하는 object를 탐색함
     * – Slot array 상에서, 탐색한 object의 이전 object의 ID를 반환함
     *   » 탐색한object가 page의 첫 번째 object인 경우,
     *    • 이전page의마지막object의 ID를 반환함
     *   » 탐색한object가 file의 첫 번째 page의 첫 번째 object인 경우,
     *   • EOS (End Of Scan) 를 반환함*/

    // Get sm_CatOverlayForData from File
    volNo = catObjForFile->volNo;
    pageNo = catObjForFile->pageNo;
    MAKE_PHYSICALFILEID(pFid, volNo, pageNo);
    e = BfM_GetTrain(&pFid, &catPage, PAGE_BUF);
    if(e < eNOERROR) ERR(e);
    GET_PTR_TO_CATENTRY_FOR_DATA(catObjForFile, catPage, catEntry);

    // 파라미터로 주어진 curOID가 NULL인 경우
    if (curOID == NULL) {
        // File의 마지막 page의 slot array 상에서의 마지막 object의 ID를 반환함
        // 마지막 page
        volNo = catObjForFile->volNo;
        pageNo = catEntry->lastPage;
        MAKE_PAGEID(pid, volNo, pageNo);
        e = BfM_GetTrain(&pid, &apage, PAGE_BUF);
        if (e < eNOERROR) ERRB1(e, &pFid, PAGE_BUF);

        // 마지막 object
        /* OUT the previous object of a current object */
        volNo = apage->header.pid.volNo;
        pageNo = apage->header.pid.pageNo;
        MAKE_OBJECTID(*prevOID, volNo, pageNo, i, apage->slot[-i].unique);

        /* OUT the object header of previous object */
        i = apage->header.nSlots - 1;
        offset = apage->slot[-i].offset;
        obj = &(apage->data[offset]);
        objHdr = &obj->header; 
        
        e = BfM_FreeTrain(&pFid, PAGE_BUF);
        if (e < eNOERROR) ERR(e);
        e = BfM_FreeTrain(&pid, PAGE_BUF);
        if (e < eNOERROR) ERR(e);

        return(eNOERROR);
    }
    /* 파라미터로주어진curOID가 NULL 이 아닌경우,
     * – curOID에 대응하는 object를 탐색함
     * – Slot array 상에서, 탐색한 object의 이전 object의 ID를 반환함
     *   » 탐색한object가 page의 첫 번째 object인 경우,
     *    • 이전page의마지막object의 ID를 반환함
     *   » 탐색한object가 file의 첫 번째 page의 첫 번째 object인 경우,
     *   • EOS (End Of Scan) 를 반환함*/
    else {
        volNo = curOID->volNo;
        pageNo = curOID->pageNo;
        MAKE_PAGEID(pid, volNo, pageNo);
        e = BfM_GetTrain(&pid, &apage, PAGE_BUF);
        if (e < eNOERROR) ERRB1(e, &pFid, PAGE_BUF);

        // index of first object
        for (firstObjIdx = 0; firstObjIdx < apage->header.nSlots; firstObjIdx++) {
            if (apage->slot[-firstObjIdx].offset != EMPTYSLOT) break;
        }

        // Slot array 상에서, 탐색한 object의 이전 object의 ID를 반환함
        // 탐색한 object가 page의 첫 번째 object인 경우
        if (curOID->slotNo==firstObjIdx) {
            // file의 첫번째 page인 경우
            if (catEntry->firstPage == apage->header.pid.pageNo) {
                e = BfM_FreeTrain(&pFid, PAGE_BUF);
                if (e < eNOERROR) ERR(e);
                e = BfM_FreeTrain(&pid, PAGE_BUF);
                if (e < eNOERROR) ERR(e);

                return(EOS);
            }
            // file의 첫번째 page가 아닌 경우
            else {
                // curOID->pageNo 페이지 해제
                e = BfM_FreeTrain(&pid, PAGE_BUF);
                if (e < eNOERROR) ERR(e);

                // 이전 page의 마지막 object의 ID를 반환함
                volNo = curOID->volNo;
                pageNo = apage->header.prevPage;
                MAKE_PAGEID(pid, volNo, pageNo);
                e = BfM_GetTrain(&pid, &apage, PAGE_BUF);
                if (e < eNOERROR) ERRB1(e, &pFid, PAGE_BUF);
                
                /* OUT the next Object of a current Object */
                volNo = apage->header.pid.volNo;
                pageNo = apage->header.pid.pageNo;
                MAKE_OBJECTID(*prevOID, volNo, pageNo, i, apage->slot[-i].unique);
        
                /* OUT the object header of next object */
                i = apage->header.nSlots - 1;
                offset = apage->slot[-i].offset;
                obj = &(apage->data[offset]);
                objHdr = &obj->header; 
                
                e = BfM_FreeTrain(&pFid, PAGE_BUF);
                if (e < eNOERROR) ERR(e);
                e = BfM_FreeTrain(&pid, PAGE_BUF);
                if (e < eNOERROR) ERR(e);

                return(eNOERROR);
            }
        }
        // 탐색한 object가 page의 첫번째 object가 아닌 경우,
        else {
            for (i = curOID->slotNo - 1; i >= 0; i--) {
                if (apage->slot[-i].offset != EMPTYSLOT) {
                    /* OUT the next Object of a current Object */
                    volNo = apage->header.pid.volNo;
                    pageNo = apage->header.pid.pageNo;
                    MAKE_OBJECTID(*prevOID, volNo, pageNo, i, apage->slot[i].unique);
            
                    /* OUT the object header of next object */
                    offset = apage->slot[-i].offset;
                    obj = &(apage->data[offset]);
                    objHdr = &obj->header; 
                    
                    e = BfM_FreeTrain(&pFid, PAGE_BUF);
                    if (e < eNOERROR) ERR(e);
                    e = BfM_FreeTrain(&pid, PAGE_BUF);
                    if (e < eNOERROR) ERR(e);

                    return(eNOERROR);
                }
            }
        }
    }

    return(EOS);
    
} /* EduOM_PrevObject() */
