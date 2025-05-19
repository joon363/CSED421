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
 * Module: edubtm_Split.c
 *
 * Description : 
 *  This file has three functions about 'split'.
 *  'edubtm_SplitInternal(...) and edubtm_SplitLeaf(...) insert the given item
 *  after spliting, and return 'ritem' which should be inserted into the
 *  parent page.
 *
 * Exports:
 *  Four edubtm_SplitInternal(ObjectID*, BtreeInternal*, Two, InternalItem*, InternalItem*)
 *  Four edubtm_SplitLeaf(ObjectID*, PageID*, BtreeLeaf*, Two, LeafItem*, InternalItem*)
 */


#include <string.h>
#include "EduBtM_common.h"
#include "BfM.h"
#include "EduBtM_Internal.h"



/*@================================
 * edubtm_SplitInternal()
 *================================*/
/*
 * Function: Four edubtm_SplitInternal(ObjectID*, BtreeInternal*,Two, InternalItem*, InternalItem*)
 *
 * Description:
 * (Following description is for original ODYSSEUS/COSMOS BtM.
 *  For ODYSSEUS/EduCOSMOS EduBtM, refer to the EduBtM project manual.)
 *
 *  At first, the function edubtm_SplitInternal(...) allocates a new internal page
 *  and initialize it.  Secondly, all items in the given page and the given
 *  'item' are divided by halves and stored to the two pages.  By spliting,
 *  the new internal item should be inserted into their parent and the item will
 *  be returned by 'ritem'.
 *
 *  A temporary page is used because it is difficult to use the given page
 *  directly and the temporary page will be copied to the given page later.
 *
 * Returns:
 *  error code
 *    some errors caused by function calls
 *
 * Note:
 *  The caller should call BfM_SetDirty() for 'fpage'.
 */
Four edubtm_SplitInternal(
    ObjectID                    *catObjForFile,         /* IN catalog object of B+ tree file */
    BtreeInternal               *fpage,                 /* INOUT the page which will be splitted */
    Two                         high,                   /* IN slot No. for the given 'item' */
    InternalItem                *item,                  /* IN the item which will be inserted */
    InternalItem                *ritem)                 /* OUT the item which will be returned by spliting */
{
    Four                        e;                      /* error number */
    Two                         i;                      /* slot No. in the given page, fpage */
    Two                         j;                      /* slot No. in the splitted pages */
    Two                         k;                      /* slot No. in the new page */
    Two                         maxLoop;                /* # of max loops; # of slots in fpage + 1 */
    Four                        sum;                    /* the size of a filled area */
    Boolean                     flag=FALSE;             /* TRUE if 'item' become a member of fpage */
    PageID                      newPid;                 /* for a New Allocated Page */
    BtreeInternal               *npage;                 /* a page pointer for the new allocated page */
    Two                         fEntryOffset;           /* starting offset of an entry in fpage */
    Two                         nEntryOffset;           /* starting offset of an entry in npage */
    Two                         entryLen;               /* length of an entry */
    btm_InternalEntry           *fEntry;                /* internal entry in the given page, fpage */
    btm_InternalEntry           *nEntry;                /* internal entry in the new page, npage*/
    Boolean                     isTmp;

    e = btm_AllocPage(catObjForFile, &fpage->hdr.pid, &newPid);
    if (e < 0) ERR(e);

    e = edubtm_InitInternal(&newPid, FALSE, FALSE);
    if (e < 0) ERR(e);

    e = BfM_GetNewTrain(&newPid, (char**)&npage, PAGE_BUF);
    if (e < 0) ERR(e);

    maxLoop = fpage->hdr.nSlots + 1;
    sum = 0;
    j = maxLoop - 1;
    flag = TRUE;
    // get sorted array
    for(i = maxLoop - 1; i >= 0 && sum <= BI_HALF; i--){
        if (i == high + 1){
            flag = FALSE;
            entryLen = sizeof(ShortPageID) + ALIGNED_LENGTH(sizeof(Two) + item->klen);
        }else{
            fEntryOffset = fpage->slot[-j];
            fEntry = (btm_InternalEntry*)&fpage->data[fEntryOffset];
            entryLen = sizeof(ShortPageID) + ALIGNED_LENGTH(sizeof(Two) + fEntry->klen);
            j--;
        }
        fpage->hdr.unused += entryLen + sizeof(SlotNo);
        sum += entryLen + sizeof(SlotNo);
    }
    fpage->hdr.nSlots = j + 1;

    k = 0;
    npage->hdr.p0 = ((btm_InternalEntry*)&fpage->data[fpage->slot[-(j+1)]])->spid;
    for(i = j + 2; i < maxLoop; i++, k++){
        nEntryOffset = npage->hdr.free;
        npage->slot[-k] = nEntryOffset;
        nEntry = (btm_InternalEntry*)&npage->data[nEntryOffset];
        if( i == high + 1 && flag == TRUE){
            nEntry->klen = item->klen;
            nEntry->spid = item->spid;
            memcpy(nEntry->kval, item->kval, item->klen);
            entryLen = sizeof(ShortPageID) + ALIGNED_LENGTH(sizeof(Two) + item->klen);
        } else{
            fEntryOffset = fpage->slot[-i];
            fEntry = (btm_InternalEntry*)&fpage->data[fEntryOffset];
            nEntry->klen = fEntry->klen;
            nEntry->spid = fEntry->spid;
            entryLen = sizeof(ShortPageID) + ALIGNED_LENGTH(sizeof(Two) + fEntry->klen);
            memcpy(nEntry->kval, fEntry->kval, fEntry->klen);
        }
        npage->hdr.free += entryLen + sizeof(SlotNo);
    }
    
    return(eNOERROR);
    
} /* edubtm_SplitInternal() */



/*@================================
 * edubtm_SplitLeaf()
 *================================*/
/*
 * Function: Four edubtm_SplitLeaf(ObjectID*, PageID*, BtreeLeaf*, Two, LeafItem*, InternalItem*)
 *
 * Description: 
 * (Following description is for original ODYSSEUS/COSMOS BtM.
 *  For ODYSSEUS/EduCOSMOS EduBtM, refer to the EduBtM project manual.)
 *
 *  The function edubtm_SplitLeaf(...) is similar to edubtm_SplitInternal(...) except
 *  that the entry of a leaf differs from the entry of an internal and the first
 *  key value of a new page is used to make an internal item of their parent.
 *  Internal pages do not maintain the linked list, but leaves do it, so links
 *  are properly updated.
 *
 * Returns:
 *  Error code
 *  eDUPLICATEDOBJECTID_BTM
 *    some errors caused by function calls
 *
 * Note:
 *  The caller should call BfM_SetDirty() for 'fpage'.
 */
Four edubtm_SplitLeaf(
    ObjectID                    *catObjForFile, /* IN catalog object of B+ tree file */
    PageID                      *root,          /* IN PageID for the given page, 'fpage' */
    BtreeLeaf                   *fpage,         /* INOUT the page which will be splitted */
    Two                         high,           /* IN slotNo for the given 'item' */
    LeafItem                    *item,          /* IN the item which will be inserted */
    InternalItem                *ritem)         /* OUT the item which will be returned by spliting */
{
    Four                        e;              /* error number */
    Two                         i;              /* slot No. in the given page, fpage */
    Two                         j;              /* slot No. in the splitted pages */
    Two                         k;              /* slot No. in the new page */
    Two                         maxLoop;        /* # of max loops; # of slots in fpage + 1 */
    Four                        sum;            /* the size of a filled area */
    PageID                      newPid;         /* for a New Allocated Page */
    PageID                      nextPid;        /* for maintaining doubly linked list */
    BtreeLeaf                   tpage;          /* a temporary page for the given page */
    BtreeLeaf                   *npage;         /* a page pointer for the new page */
    BtreeLeaf                   *mpage;         /* for doubly linked list */
    btm_LeafEntry               *itemEntry;     /* entry for the given 'item' */
    btm_LeafEntry               *fEntry;        /* an entry in the given page, 'fpage' */
    btm_LeafEntry               *nEntry;        /* an entry in the new page, 'npage' */
    ObjectID                    *iOidArray;     /* ObjectID array of 'itemEntry' */
    ObjectID                    *fOidArray;     /* ObjectID array of 'fEntry' */
    Two                         fEntryOffset;   /* starting offset of 'fEntry' */
    Two                         nEntryOffset;   /* starting offset of 'nEntry' */
    Two                         oidArrayNo;     /* element No in an ObjectID array */
    Two                         alignedKlen;    /* aligned length of the key length */
    Two                         itemEntryLen;   /* length of entry for item */
    Two                         entryLen;       /* entry length */
    Boolean                     flag;
    Boolean                     isTmp;
 
    /* 새로운page를 할당받음 */
    e = btm_AllocPage(catObjForFile, root, &newPid);
    if (e < eNOERROR) ERR(e);
    e = BfM_GetNewTrain(&newPid, (char**)&npage, PAGE_BUF);
    if (e < eNOERROR) ERR(e);

    /* 할당받은page를 leaf page로 초기화함*/
    e = edubtm_InitLeaf(&newPid, FALSE, FALSE);
    if (e < eNOERROR) ERR(e);
    
    /* 기존index entry들 및 삽입할 index entry를 key 순으로 정렬하여
    overflow가 발생한 page 및 할당 받은 page에 나누어 저장함
    – 먼저, overflow가 발생한 page에 데이터 영역을 50% 이상 채우는 수의index entry들을 저장함
    – 나머지index entry들을 할당 받은 page에 저장함
    – 각page의 header를 갱신함*/
    maxLoop = fpage->hdr.nSlots + 1;
    itemEntryLen = sizeof(Two) + sizeof(Two) + ALIGNED_LENGTH(item->klen) + sizeof(ObjectID); 
    sum = 0;
    j = 0;

    // get sorted array
    for(i = 0; i < maxLoop && sum <= BL_HALF; i++){

        if (i == high + 1){
            flag = TRUE;
            entryLen = itemEntryLen; 
        }else{
            fEntryOffset = fpage->slot[-j];
            fEntry = &fpage->data[fEntryOffset];
            entryLen = sizeof(Two) + sizeof(Two) + ALIGNED_LENGTH(fEntry->klen) + sizeof(ObjectID); 
            j++;
        }
        sum += (entryLen + sizeof(Two));
    }
    fpage->hdr.nSlots = j;

    for (k = 0; i < maxLoop; i++, k++){
        nEntryOffset = npage->hdr.free;
        npage->slot[-k] = nEntryOffset;
        nEntry = &npage->data[nEntryOffset];
        
        if (i == high + 1){
            nEntry->klen = item->klen;
            nEntry->nObjects = item->nObjects;
            
            entryLen = itemEntryLen;
            memcpy(nEntry->kval, item->kval, item->klen);
            memcpy(&nEntry->kval[ALIGNED_LENGTH(item->klen)], &item->oid, OBJECTID_SIZE);
            // *(ObjectID*)&nEntry->kval[entryLen] = item->oid;
        }
        else{
            fEntryOffset = fpage->slot[-j];
            fEntry = &fpage->data[fEntryOffset];
            entryLen = sizeof(Two) + sizeof(Two) + ALIGNED_LENGTH(fEntry->klen) + sizeof(ObjectID);
            
            memcpy(nEntry, fEntry, entryLen);
            
            if (fEntryOffset + entryLen == fpage->hdr.free)
                fpage->hdr.free -= entryLen;
            else
                fpage->hdr.unused += entryLen;

            j++;
        }

        npage->hdr.free += entryLen;
    }
    npage->hdr.nSlots = k;

    if(flag){
        for(i = fpage->hdr.nSlots - 1; i > high; i--){
            fpage->slot[-(i+1)] = fpage->slot[-i];
        }

        if (BI_CFREE(fpage) < itemEntryLen){
            edubtm_CompactLeafPage(fpage, NIL);
        }

        fpage->slot[-(high + 1)] = fpage->hdr.free;

        fEntry = &fpage->data[fpage->hdr.free];
        fEntry->nObjects = item->nObjects;
        fEntry->klen = item->klen;

        memcpy(fEntry->kval, item->kval, item->klen);        
        memcpy(&fEntry->kval[ALIGNED_LENGTH(item->klen)], &item->oid, OBJECTID_SIZE);

        fpage->hdr.free += sizeof(Two) + sizeof(Two) + ALIGNED_LENGTH(item->klen) + sizeof(ObjectID);
        fpage->hdr.nSlots ++;
    }

    nEntry = &npage->data[npage->slot[0]];
    ritem->spid = npage->hdr.pid.pageNo;
    ritem->klen = nEntry->klen;
    memcpy(ritem->kval, nEntry->kval, nEntry->klen);

    if (fpage->hdr.type & ROOT)
        fpage->hdr.type = LEAF;

    // initialize page list
    npage->hdr.nextPage = fpage->hdr.nextPage;
    npage->hdr.prevPage = fpage->hdr.pid.pageNo;
    fpage->hdr.nextPage = npage->hdr.pid.pageNo;

    if (npage->hdr.nextPage != NIL){
        MAKE_PAGEID(nextPid, npage->hdr.pid.volNo, npage->hdr.nextPage);

        e = BfM_GetTrain(&nextPid, &mpage, PAGE_BUF);
        if(e < 0) ERR(e);
        
        mpage->hdr.prevPage = npage->hdr.pid.pageNo;

        e = BfM_SetDirty(&nextPid, PAGE_BUF);
        if(e < 0) ERR(e);

        e = BfM_FreeTrain(&nextPid, PAGE_BUF);
        if(e < 0) ERR(e);
    }

    e = BfM_SetDirty(&newPid, PAGE_BUF);
    if(e < 0) ERR(e);

    e = BfM_FreeTrain(&newPid, PAGE_BUF);
    if(e < 0) ERR(e);

    return(eNOERROR);
    
} /* edubtm_SplitLeaf() */
