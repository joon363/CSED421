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
 * Module: edubtm_Insert.c
 *
 * Description : 
 *  This function edubtm_Insert(...) recursively calls itself until the type
 *  of a root page becomes LEAF.  If the given root page is an internal,
 *  it recursively calls itself using a proper child.  If the result of
 *  the call occur spliting, merging, or redistributing the children, it
 *  may insert, delete, or replace its own internal item, and if the given
 *  root page may be merged, splitted, or redistributed, it affects the
 *  return values.
 *
 * Exports:
 *  Four edubtm_Insert(ObjectID*, PageID*, KeyDesc*, KeyValue*, ObjectID*,
 *                  Boolean*, Boolean*, InternalItem*, Pool*, DeallocListElem*)
 *  Four edubtm_InsertLeaf(ObjectID*, PageID*, BtreeLeaf*, KeyDesc*, KeyValue*,
 *                      ObjectID*, Boolean*, Boolean*, InternalItem*)
 *  Four edubtm_InsertInternal(ObjectID*, BtreeInternal*, InternalItem*,
 *                          Two, Boolean*, InternalItem*)
 */


#include <string.h>
#include "EduBtM_common.h"
#include "BfM.h"
#include "OM_Internal.h"	/* for SlottedPage containing catalog object */
#include "EduBtM_Internal.h"



/*@================================
 * edubtm_Insert()
 *================================*/
/*
 * Function: Four edubtm_Insert(ObjectID*, PageID*, KeyDesc*, KeyValue*,
 *                           ObjectID*, Boolean*, Boolean*, InternalItem*,
 *                           Pool*, DeallocListElem*)
 *
 * Description:
 * (Following description is for original ODYSSEUS/COSMOS BtM.
 *  For ODYSSEUS/EduCOSMOS EduBtM, refer to the EduBtM project manual.)
 *
 *  If the given root is a leaf page, it should get the correct entry in the
 *  leaf. If the entry is already in the leaf, it simply insert it into the
 *  entry and increment the number of ObjectIDs.  If it is not in the leaf it
 *  makes a new entry and insert it into the leaf.
 *  If there is not enough spage in the leaf, the page should be splitted.  The
 *  overflow page may be used or created by this routine. It is created when
 *  the size of the entry is greater than a third of a page.
 * 
 *  'h' is TRUE if the given root page is splitted and the entry item will be
 *  inserted into the parent page.  'f' is TRUE if the given page is not half
 *  full because of creating a new overflow page.
 *
 * Returns:
 *  Error code
 *    eBADBTREEPAGE_BTM
 *    some errors caused by function calls
 */
Four edubtm_Insert(
    ObjectID                    *catObjForFile,         /* IN catalog object of B+-tree file */
    PageID                      *root,                  /* IN the root of a Btree */
    KeyDesc                     *kdesc,                 /* IN Btree key descriptor */
    KeyValue                    *kval,                  /* IN key value */
    ObjectID                    *oid,                   /* IN ObjectID which will be inserted */
    Boolean                     *f,                     /* OUT whether it is merged by creating a new overflow page */
    Boolean                     *h,                     /* OUT whether it is splitted */
    InternalItem                *item,                  /* OUT Internal Item which will be inserted */
                                                        /*     into its parent when 'h' is TRUE */
    Pool                        *dlPool,                /* INOUT pool of dealloc list */
    DeallocListElem             *dlHead)                /* INOUT head of the dealloc list */
{
    Four                        e;                      /* error number */
    Boolean                     lh;                     /* local 'h' */
    Boolean                     lf;                     /* local 'f' */
    Two                         idx;                    /* index for the given key value */
    PageID                      newPid;                 /* a new PageID */
    KeyValue                    tKey;                   /* a temporary key */
    InternalItem                litem;                  /* a local internal item */
    BtreePage                   *apage;                 /* a pointer to the root page */
    btm_InternalEntry           *iEntry;                /* an internal entry */
    Two                         iEntryOffset;           /* starting offset of an internal entry */
    SlottedPage                 *catPage;               /* buffer page containing the catalog object */
    sm_CatOverlayForBtree       *catEntry;              /* pointer to Btree file catalog information */
    PhysicalFileID              pFid;                   /* B+-tree file's FileID */


    /* Error check whether using not supported functionality by EduBtM */
    int i;
    for(i=0; i<kdesc->nparts; i++)
    {
        if(kdesc->kpart[i].type!=SM_INT && kdesc->kpart[i].type!=SM_VARSTRING)
            ERR(eNOTSUPPORTED_EDUBTM);
    }

    *f = *h = FALSE;
    lf = lh = FALSE;

    e = BfM_GetTrain(root, (char**)&apage, PAGE_BUF);
    if (e < eNOERROR) ERR(e);

    if(apage->any.hdr.type & INTERNAL){
        /*  
        – 새로운<object의 key, object ID> pair를 삽입할 leaf page를 찾기위해
          다음으로방문할자식page를결정함
        */
        if(edubtm_BinarySearchInternal(apage, kdesc, kval, &idx)){
            iEntryOffset = apage->bi.slot[-idx];
            iEntry = (btm_InternalEntry*)&apage->bi.data[iEntryOffset];
            MAKE_PAGEID(newPid, root->volNo, iEntry->spid);
        }
        else 
            MAKE_PAGEID(newPid, root->volNo, apage->bi.hdr.p0);

        /*
        – 결정된자식page를 root page로 하는 B+ subtree에 새로운 <object의 key, object ID> pair를 삽입하기 위해 
          재귀적으로 edubtm_Insert()를 호출함
        */
        e = edubtm_Insert(catObjForFile, &newPid, kdesc, kval, oid, &lf, &lh, &litem, dlPool, dlHead);
        if (e < eNOERROR) ERR(e);

        // – 결정된자식page에서split이 발생한 경우, 
        if(lh){
            /* 해당 split으로 생성된새로운page를가리키는internal index entry를 파라미터로주어진root page에 삽입함
                » 해당index entry의 삽입 위치 (slot 번호) 를 결정함
                    • Slot array에 저장된 index entry의 offset들이 index entry의 key 순으로 정렬되어야 함 */
            tKey.len = litem.klen;
            memcpy(tKey.val, litem.kval, litem.klen);
            edubtm_BinarySearchInternal(&(apage->bi), kdesc, &tKey, &idx);
            /* » edubtm_InsertInternal()을 호출하여 결정된 slot 번호로index entry를 삽입함
            – 파라미터로주어진root page에서 split이 발생한 경우, 해당split으로 생성된 새로운 page를 가리키는 internal index entry를 반환함 */
            e = edubtm_InsertInternal(catObjForFile, &(apage->bi), &litem, idx, h, item);
            if (e < eNOERROR) ERR(e);
        }
    }
    else{
        /*edubtm_InsertLeaf()를 호출하여 해당 page에 새로운 <object의 key, object ID> pair를 삽입함
        – Split이 발생한 경우, 해당 split으로 생성된 새로운 page를 가리키는internal index entry를 반환함*/
        e = edubtm_InsertLeaf(catObjForFile, root, &apage->bl, kdesc, kval, oid, f, h, item);
        if (e < eNOERROR) ERR(e);
    }

    e = BfM_SetDirty(root, PAGE_BUF);
    if (e < eNOERROR) ERR(e);
    e = BfM_FreeTrain(root, PAGE_BUF);
    if (e < eNOERROR) ERR(e);

    return(eNOERROR);
    
}   /* edubtm_Insert() */



/*@================================
 * edubtm_InsertLeaf()
 *================================*/
/*
 * Function: Four edubtm_InsertLeaf(ObjectID*, PageID*, BtreeLeaf*, KeyDesc*,
 *                               KeyValue*, ObjectID*, Boolean*, Boolean*,
 *                               InternalItem*)
 *
 * Description:
 * (Following description is for original ODYSSEUS/COSMOS BtM.
 *  For ODYSSEUS/EduCOSMOS EduBtM, refer to the EduBtM project manual.)
 *
 *  Insert into the given leaf page an ObjectID with the given key.
 *
 * Returns:
 *  Error code
 *    eDUPLICATEDKEY_BTM
 *    eDUPLICATEDOBJECTID_BTM
 *    some errors causd by function calls
 *
 * Side effects:
 *  1) f : TRUE if the leaf page is underflowed by creating an overflow page
 *  2) h : TRUE if the leaf page is splitted by inserting the given ObjectID
 *  3) item : item to be inserted into the parent
 */
Four edubtm_InsertLeaf(
    ObjectID                    *catObjForFile, /* IN catalog object of B+-tree file */
    PageID                      *pid,           /* IN PageID of Leag Page */
    BtreeLeaf                   *page,          /* INOUT pointer to buffer page of Leaf page */
    KeyDesc                     *kdesc,         /* IN Btree key descriptor */
    KeyValue                    *kval,          /* IN key value */
    ObjectID                    *oid,           /* IN ObjectID which will be inserted */
    Boolean                     *f,             /* OUT whether it is merged by creating */
                                                /*     a new overflow page */
    Boolean                     *h,             /* OUT whether it is splitted */
    InternalItem                *item)          /* OUT Internal Item which will be inserted */
                                                /*     into its parent when 'h' is TRUE */
{
    Four                        e;              /* error number */
    Two                         i;
    Two                         idx;            /* index for the given key value */
    LeafItem                    leaf;           /* a Leaf Item */
    Boolean                     found;          /* search result */
    btm_LeafEntry               *entry;         /* an entry in a leaf page */
    Two                         entryOffset;    /* start position of an entry */
    Two                         alignedKlen;    /* aligned length of the key length */
    PageID                      ovPid;          /* PageID of an overflow page */
    Two                         entryLen;       /* length of an entry */
    Two                         neededSpace;
    ObjectID                    *oidArray;      /* an array of ObjectIDs */
    Two                         oidArrayElemNo; /* an index for the ObjectID array */


    /* Error check whether using not supported functionality by EduBtM */
    for(i=0; i<kdesc->nparts; i++)
    {
        if(kdesc->kpart[i].type!=SM_INT && kdesc->kpart[i].type!=SM_VARSTRING)
            ERR(eNOTSUPPORTED_EDUBTM);
    }

    
    /*@ Initially the flags are FALSE */
    *h = *f = FALSE;
    
    /*
    • 새로운index entry의 삽입 위치 (slot 번호) 를 결정함
        – Slot array에 저장된 index entry의 offset들이 index entry의 key 순으로 정렬되어야 함
        – 새로운index entry의 key 값과 동일한 key 값을 갖는 index entry가 존재하는 경우
          eDUPLICATEDKEY_BTM error 를 반환함
    */
    if (edubtm_BinarySearchLeaf(page, kdesc, kval, &idx)) 
        ERR(eDUPLICATEDKEY_BTM);

    // 새로운index entry 삽입을 위해 필요한 자유 영역의 크기를계산함
    alignedKlen = 0;
    for(i=0; i<kdesc->nparts; i++)
    {
        alignedKlen += (kdesc->kpart[i].type == SM_INT) ? \
            ALIGNED_LENGTH(kdesc->kpart[i].length) : ALIGNED_LENGTH(kval->len);
    }
    /*
    -----------------------------------------------------
    |  nObjects |  klen  |   key   |  value(Object ID)  |
    -----------------------------------------------------
        Two        Two   (aligned)klen    ObjectID
    */
    entryLen = sizeof(Two)+ sizeof(Two)+ alignedKlen+ sizeof(ObjectID);
    // Align 된 key 영역을 고려한 새로운 index entry의 크기 + slot의 크기
    neededSpace = entryLen+ sizeof(Two);
    
    // • Page에 여유 영역이있는경우,
    if (BL_FREE(page) >= neededSpace){
        // – 필요시page를compact 함  
        if (BL_CFREE(page) < neededSpace)
            edubtm_CompactLeafPage(page, NIL);
        
        // » 결정된slot 번호를갖는slot을 사용하기 위해slot array를 재배열함
        for(i = page->hdr.nSlots - 1; i > idx; i--){
            page->slot[-(i+1)] = page->slot[-i];
        }
        // » 결정된slot 번호를갖는slot에 새로운 index entry의 offset을 저장함
        entryOffset = page->hdr.free;
        page->slot[-(idx+1)] = entryOffset;

        // » Page의 contiguous free area에 새로운 index entry를 복사함
        entry = (btm_LeafEntry*)&page->data[entryOffset];
        entry->nObjects = 1; // 유일key를 사용하는EduBtM에서는 각key 값 갖는 object는 한개씩만 존재함
        entry->klen = kval->len;
        memcpy(entry->kval, kval->val, alignedKlen);
        memcpy(&entry->kval[alignedKlen], oid, sizeof(ObjectID));

        // Page의 header을 갱신함
        page->hdr.free = page->hdr.free + entryLen;
        page->hdr.nSlots ++;
    }
    /*• Page에 여유 영역이없는경우(page overflow),
        – edubtm_SplitLeaf()를 호출하여 page를 split 함
        – Split으로 생성된 새로운 leaf page를 가리키는 internal index entry를 반환함 */
    else{
        /* LeafItem
        ----------------------------------------------------
         oid      | nObjects |  klen  | char kval[MAXKEYLEN]
         ObjectID |    Two   |  Two   |
        ----------------------------------------------------
                             [           KeyValue          ]
        */
        leaf.oid = *oid;
        leaf.nObjects = 1;
        memcpy(&leaf.klen, kval, sizeof(KeyValue));
        e = edubtm_SplitLeaf(catObjForFile, pid, page, idx, &leaf, item);
        if (e < eNOERROR) ERR(e);
        *h = TRUE; // is Splitted
    }

    return(eNOERROR);
    
} /* edubtm_InsertLeaf() */



/*@================================
 * edubtm_InsertInternal()
 *================================*/
/*
 * Function: Four edubtm_InsertInternal(ObjectID*, BtreeInternal*, InternalItem*, Two, Boolean*, InternalItem*)
 *
 * Description:
 * (Following description is for original ODYSSEUS/COSMOS BtM.
 *  For ODYSSEUS/EduCOSMOS EduBtM, refer to the EduBtM project manual.)
 *
 *  This routine insert the given internal item into the given page. If there
 *  is not enough space in the page, it should split the page and the new
 *  internal item should be returned for inserting into the parent.
 *
 * Returns:
 *  Error code
 *    some errors caused by function calls
 *
 * Side effects:
 *  h:	TRUE if the page is splitted
 *  ritem: an internal item which will be inserted into parent
 *          if spliting occurs.
 */
Four edubtm_InsertInternal(
    ObjectID            *catObjForFile, /* IN catalog object of B+-tree file */
    BtreeInternal       *page,          /* INOUT Page Pointer */
    InternalItem        *item,          /* IN Iternal item which is inserted */
    Two                 high,           /* IN index in the given page */
    Boolean             *h,             /* OUT whether the given page is splitted */
    InternalItem        *ritem)         /* OUT if the given page is splitted, the internal item may be returned by 'ritem'. */
{
    Four                e;              /* error number */
    Two                 i;              /* index */
    Two                 entryOffset;    /* starting offset of an internal entry */
    Two                 alignedKlen;    
    Two                 entryLen;       /* length of the new entry */
    Two                 neededSpace;
    btm_InternalEntry   *entry;         /* an internal entry of an internal page */


    
    /*@ Initially the flag are FALSE */
    *h = FALSE;
    
    // 새로운index entry 삽입을 위해 필요한 자유 영역의 크기를계산함
    /*
    ------------------------------------------------
    |     spid    |     klen     |      kval[]     |
    ------------------------------------------------
    ShortPageID       Two            entry->klen
    */
    alignedKlen = ALIGNED_LENGTH(sizeof(Two)+entry->klen);
    entryLen = sizeof(ShortPageID)+ alignedKlen;
    // Align 된 key 영역을 고려한 새로운 index entry의 크기 + slot의 크기
    neededSpace = entryLen+ sizeof(Two);
    
    // • Page에 여유 영역이있는경우,
    if (BL_FREE(page) >= neededSpace){
        // – 필요시page를compact 함  
        if (BL_CFREE(page) < neededSpace)
            edubtm_CompactInternalPage(page, NIL);
        
        // » 결정된slot 번호를갖는slot을 사용하기 위해slot array를 재배열함
        for(i = page->hdr.nSlots - 1; i > high; i--){
            page->slot[-(i+1)] = page->slot[-i];
        }
        // » 결정된slot 번호를갖는slot에 새로운 index entry의 offset을 저장함
        entryOffset = page->hdr.free;
        page->slot[-(high+1)] = entryOffset;

        // » Page의 contiguous free area에 새로운 index entry를 복사함
        entry = (btm_InternalEntry*)&page->data[entryOffset];
        entry->spid = item->spid; // 유일key를 사용하는EduBtM에서는 각key 값 갖는 object는 한개씩만 존재함
        entry->klen = item->klen;
        memcpy(entry->kval, item->kval, item->klen);

        // Page의 header을 갱신함
        page->hdr.free = page->hdr.free + entryLen;
        page->hdr.nSlots ++;
    }
    /*• Page에 여유 영역이없는경우(page overflow),
        – edubtm_SplitInternal()를 호출하여 page를 split 함
        – Split으로 생성된 새로운 internal page를 가리키는 internal index entry를 반환함 */
    else{
        e = edubtm_SplitInternal(catObjForFile, page, high, item, ritem);
        if (e < eNOERROR) ERR(e);
        *h = TRUE; // is Splitted
    }

    return(eNOERROR);
    
} /* edubtm_InsertInternal() */

