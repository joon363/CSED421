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

    /* 새로운page를 할당받음 */
    e = btm_AllocPage(catObjForFile, &fpage->hdr.pid, &newPid);
    if (e < eNOERROR) ERR(e);
    e = BfM_GetNewTrain(&newPid, (char**)&npage, PAGE_BUF);
    if (e < eNOERROR) ERR(e);

    /* 할당받은page를 Internal page로 초기화함*/
    e = edubtm_InitInternal(&newPid, FALSE, FALSE);
    if (e < eNOERROR) ERR(e);

    maxLoop = fpage->hdr.nSlots + 1;
    sum = 0;
    j = maxLoop - 1;
    flag = TRUE;
    // calculate numbers(j) of entries just over half of the fpage.
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
    
    itemEntryLen = sizeof(Two) + sizeof(Two) + ALIGNED_LENGTH(item->klen) + sizeof(ObjectID); 

    // calculate numbers(j) of entries just over half of the fpage.
    sum = 0;
    j = 0;
    maxLoop = fpage->hdr.nSlots + 1;
    for(i = 0; i < maxLoop && sum <= BL_HALF; i++){

        if (i == high + 1){
            // item slotNo (high) is inside fpage
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
    // fpage has only j entries now.
    // slots from slot[j] should be moved into npage.
    fpage->hdr.nSlots = j;

    // remaining entries of fpage (j~nSlots) should be copied into npage.
    for (k = 0; i < maxLoop; i++, k++){
        nEntryOffset = npage->hdr.free;
        npage->slot[-k] = nEntryOffset;
        nEntry = &npage->data[nEntryOffset];
        
        if (i == high + 1){
            // item slotNo (high) is inside npage
            nEntry->klen = item->klen;
            nEntry->nObjects = item->nObjects;
            
            entryLen = itemEntryLen;
            memcpy(nEntry->kval, item->kval, item->klen);
            memcpy(&nEntry->kval[ALIGNED_LENGTH(item->klen)], &item->oid, OBJECTID_SIZE);
        }
        else{
            // copy fpage entry into npage from slot[j]
            fEntryOffset = fpage->slot[-j];
            fEntry = &fpage->data[fEntryOffset];
            entryLen = sizeof(Two) + sizeof(Two) + ALIGNED_LENGTH(fEntry->klen) + sizeof(ObjectID);
            memcpy(nEntry, fEntry, entryLen);
            /* look at page 16 of manual.
             * we are removing fEntry from fpage.
             * if fEntry is the last object of data area just before conti-free area,
             * starting address of continuous free area should be reduced. */
            if (fEntryOffset + entryLen == fpage->hdr.free)
                fpage->hdr.free -= entryLen;
            else
            // else, just increase unused area.
                fpage->hdr.unused += entryLen;
            j++;
        }
        // update continuous free area address.
        npage->hdr.free += entryLen;
    }
    npage->hdr.nSlots = k;

    // this flag means that item slotNo (high) is inside fpage
    // therefore, we need to insert input item into fpage at high.
    if(flag){
        // shift other pages
        for(i = fpage->hdr.nSlots - 1; i > high; i--){
            fpage->slot[-(i+1)] = fpage->slot[-i];
        }
        // compact page if you need to.
        if (BI_CFREE(fpage) < itemEntryLen){
            edubtm_CompactLeafPage(fpage, NIL);
        }
        
        // we will add the item at the end of free area.
        fpage->slot[-(high + 1)] = fpage->hdr.free;

        // copy item into the end of free area.
        fEntry = &fpage->data[fpage->hdr.free];
        fEntry->nObjects = item->nObjects;
        fEntry->klen = item->klen;
        memcpy(fEntry->kval, item->kval, item->klen);        
        memcpy(&fEntry->kval[ALIGNED_LENGTH(item->klen)], &item->oid, OBJECTID_SIZE);

        // now update the continuous free area address.
        fpage->hdr.free += sizeof(Two) + sizeof(Two) + ALIGNED_LENGTH(item->klen) + sizeof(ObjectID);
        fpage->hdr.nSlots ++;
    }


    /*할당받은page를leaf page들간의 doubly linked list에 추가함
    – 할당받은page가overflow가 발생한 page의 다음 page가 되도록 추가함 */
    
    
    // 0) f <-> f'
    // 1) n -> f'
    npage->hdr.nextPage = fpage->hdr.nextPage;

    // 2) f <- n
    npage->hdr.prevPage = fpage->hdr.pid.pageNo;

    // 3) f -> n
    fpage->hdr.nextPage = npage->hdr.pid.pageNo;

    // 4) n <- f' (if f' exists) 
    if (npage->hdr.nextPage != NIL){
        // acquire n.next (it was f')
        MAKE_PAGEID(nextPid, npage->hdr.pid.volNo, npage->hdr.nextPage);
        e = BfM_GetTrain(&nextPid, &mpage, PAGE_BUF);
        if(e < eNOERROR) ERR(e);
        
        mpage->hdr.prevPage = npage->hdr.pid.pageNo;

        e = BfM_SetDirty(&nextPid, PAGE_BUF);
        if(e < eNOERROR) ERR(e);
        e = BfM_FreeTrain(&nextPid, PAGE_BUF);
        if(e < eNOERROR) ERR(e);
    }
    // f <-> n <-> f'

    /*할당받은page를가리키는internal index entry를 생성함
    – Discriminator key 값 := 할당 받은 page의 첫 번째 index entry (slot 번호= 0) 의key 값
    – 자식page의 번호:= 할당받은page (npage)의 번호*/
    nEntry = &npage->data[npage->slot[0]];
    ritem->klen = nEntry->klen;
    memcpy(ritem->kval, nEntry->kval, nEntry->klen);
    ritem->spid = npage->hdr.pid.pageNo;

    // Split된 page가 ROOT일 경우, type을 LEAF로 변경함
    if (fpage->hdr.type & ROOT)
        fpage->hdr.type = LEAF;

    e = BfM_SetDirty(&newPid, PAGE_BUF);
    if(e < eNOERROR) ERR(e);
    e = BfM_FreeTrain(&newPid, PAGE_BUF);
    if(e < eNOERROR) ERR(e);

    return(eNOERROR);
    
} /* edubtm_SplitLeaf() */
