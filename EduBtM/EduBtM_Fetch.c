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
 * Module: EduBtM_Fetch.c
 *
 * Description :
 *  Find the first object satisfying the given condition.
 *  If there is no such object, then return with 'flag' field of cursor set
 *  to CURSOR_EOS. If there is an object satisfying the condition, then cursor
 *  points to the object position in the B+ tree and the object identifier
 *  is returned via 'cursor' parameter.
 *  The condition is given with a key value and a comparison operator;
 *  the comparison operator is one among SM_BOF, SM_EOF, SM_EQ, SM_LT, SM_LE, SM_GT, SM_GE.
 *
 * Exports:
 *  Four EduBtM_Fetch(PageID*, KeyDesc*, KeyValue*, Four, KeyValue*, Four, BtreeCursor*)
 */


#include <string.h>
#include "EduBtM_common.h"
#include "BfM.h"
#include "EduBtM_Internal.h"


/*@ Internal Function Prototypes */
Four edubtm_Fetch(PageID*, KeyDesc*, KeyValue*, Four, KeyValue*, Four, BtreeCursor*);



/*@================================
 * EduBtM_Fetch()
 *================================*/
/*
 * Function: Four EduBtM_Fetch(PageID*, KeyDesc*, KeyVlaue*, Four, KeyValue*, Four, BtreeCursor*)
 *
 * Description:
 * (Following description is for original ODYSSEUS/COSMOS BtM.
 *  For ODYSSEUS/EduCOSMOS EduBtM, refer to the EduBtM project manual.)
 *
 *  Find the first object satisfying the given condition. See above for detail.
 *
 * Returns:
 *  error code
 *    eBADPARAMETER_BTM
 *    some errors caused by function calls
 *
 * Side effects:
 *  cursor  : The found ObjectID and its position in the Btree Leaf
 *            (it may indicate a ObjectID in an  overflow page).
 */
Four EduBtM_Fetch(
    PageID   *root,		/* IN The current root of the subtree */
    KeyDesc  *kdesc,		/* IN Btree key descriptor */
    KeyValue *startKval,	/* IN key value of start condition */
    Four     startCompOp,	/* IN comparison operator of start condition */
    KeyValue *stopKval,		/* IN key value of stop condition */
    Four     stopCompOp,	/* IN comparison operator of stop condition */
    BtreeCursor *cursor)	/* OUT Btree Cursor */
{
    int i;
    Four e;		   /* error number */

    
    if (root == NULL) ERR(eBADPARAMETER_BTM);

    /* Error check whether using not supported functionality by EduBtM */
    for(i=0; i<kdesc->nparts; i++)
    {
        if(kdesc->kpart[i].type!=SM_INT && kdesc->kpart[i].type!=SM_VARSTRING)
            ERR(eNOTSUPPORTED_EDUBTM);
    }
    

    return(eNOERROR);

} /* EduBtM_Fetch() */



/*@================================
 * edubtm_Fetch()
 *================================*/
/*
 * Function: Four edubtm_Fetch(PageID*, KeyDesc*, KeyVlaue*, Four, KeyValue*, Four, BtreeCursor*)
 *
 * Description:
 * (Following description is for original ODYSSEUS/COSMOS BtM.
 *  For ODYSSEUS/EduCOSMOS EduBtM, refer to the EduBtM project manual.)
 *
 *  Find the first object satisfying the given condition.
 *  This function handles only the following conditions:
 *  SM_EQ, SM_LT, SM_LE, SM_GT, SM_GE.
 *
 * Returns:
 *  Error code *   
 *    eBADCOMPOP_BTM
 *    eBADBTREEPAGE_BTM
 *    some errors caused by function calls
 */
Four edubtm_Fetch(
    PageID              *root,          /* IN The current root of the subtree */
    KeyDesc             *kdesc,         /* IN Btree key descriptor */
    KeyValue            *startKval,     /* IN key value of start condition */
    Four                startCompOp,    /* IN comparison operator of start condition */
    KeyValue            *stopKval,      /* IN key value of stop condition */
    Four                stopCompOp,     /* IN comparison operator of stop condition */
    BtreeCursor         *cursor)        /* OUT Btree Cursor */
{
    Four                e;              /* error number */
    Four                cmp;            /* result of comparison */
    Two                 idx;            /* index */
    PageID              child;          /* child page when the root is an internla page */
    Two                 alignedKlen;    /* aligned size of the key length */
    BtreePage           *apage;         /* a Page Pointer to the given root */
    BtreeOverflow       *opage;         /* a page pointer if it necessary to access an overflow page */
    Boolean             found;          /* search result */
    PageID              *leafPid;       /* leaf page pointed by the cursor */
    Two                 slotNo;         /* slot pointed by the slot */
    PageID              ovPid;          /* PageID of the overflow page */
    PageNo              ovPageNo;       /* PageNo of the overflow page */
    PageID              prevPid;        /* PageID of the previous page */
    PageID              nextPid;        /* PageID of the next page */
    ObjectID            *oidArray;      /* array of the ObjectIDs */
    Two                 iEntryOffset;   /* starting offset of an internal entry */
    btm_InternalEntry   *iEntry;        /* an internal entry */
    Two                 lEntryOffset;   /* starting offset of a leaf entry */
    btm_LeafEntry       *lEntry;        /* a leaf entry */


    /* Error check whether using not supported functionality by EduBtM */
    int i;
    for(i=0; i<kdesc->nparts; i++)
    {
        if(kdesc->kpart[i].type!=SM_INT && kdesc->kpart[i].type!=SM_VARSTRING)
            ERR(eNOTSUPPORTED_EDUBTM);
    }

    e = BfM_GetTrain(root, (char**)&apage, PAGE_BUF);
    if (e < eNOERROR) ERR(e);

    if(apage->any.hdr.type & INTERNAL){
        /*  
        – 검색조건을만족하는첫번째 <object의 key, object ID> pair가 저장된 leaf page를 찾기위해
          다음으로 방문할 자식page를결정함
        */
        if(edubtm_BinarySearchInternal(apage, kdesc, startKval, &idx)){
            iEntryOffset = apage->bi.slot[-idx];
            iEntry = (btm_InternalEntry*)&apage->bi.data[iEntryOffset];
            MAKE_PAGEID(child, root->volNo, iEntry->spid);
        }
        else 
            MAKE_PAGEID(child, root->volNo, apage->bi.hdr.p0);

        /*
        – 결정된자식page를 root page로 하는 B+ subtree에서 검색 조건을 만족하는 첫 번째 <object의 key, object ID> 
        pair가 저장된 leaf index entry를 검색하기 위해 재귀적으로 edubtm_Fetch()를 호출함
        */
        e = edubtm_Fetch(&child, kdesc, startKval, startCompOp, stopKval, stopCompOp, cursor);
        if (e < eNOERROR) ERR(e);
    }
    else{
        /*검색조건을만족하는첫번째<object의key, object ID> pair가 저장된 index entry를 검색함*/
        found = edubtm_BinarySearchLeaf(apage, kdesc, startKval, &idx);
        cursor->flag = CURSOR_ON;
        slotNo = idx;
        switch (startCompOp){
            case SM_EQ:
                if (!found) cursor->flag = CURSOR_EOS;
                break;
            case SM_LT:
                if (!found) slotNo --; //found여도 어차피 하나 작게 옴.
                break;
            case SM_LE:
                break;
            case SM_GT:
                slotNo ++;
                break;
            case SM_GE:
                if (!found) slotNo ++;
                break;
            default:
                break;
        }
        
        // Index값에 따라서 앞, 뒤페이지를 탐색함.
        leafPid = root;
        e = BfM_FreeTrain(root, PAGE_BUF);
        if (e < eNOERROR) ERR(e);

        if (slotNo < 0){
            if (apage->bl.hdr.prevPage == NIL){
                cursor->flag = CURSOR_EOS;
            } else{
                MAKE_PAGEID(*leafPid, root->volNo, apage->bl.hdr.prevPage);
                e = BfM_GetTrain(leafPid, (char**)&apage, PAGE_BUF);
                if (e < eNOERROR) ERR(e);
                slotNo = apage->bl.hdr.nSlots - 1;
            }
        } else if(slotNo >= apage->bl.hdr.nSlots){
            if (apage->bl.hdr.nextPage == NIL){
                cursor->flag = CURSOR_EOS;
            } else{
                MAKE_PAGEID(*leafPid, root->volNo, apage->bl.hdr.nextPage);
                e = BfM_GetTrain(leafPid, (char**)&apage, PAGE_BUF);
                if (e < eNOERROR) ERR(e);
                slotNo = 0;
            }
        }

        // Stop Condition 적용
        if (cursor->flag == CURSOR_ON){ 
            lEntryOffset = apage->bl.slot[-slotNo];
            lEntry = (btm_LeafEntry*)&apage->bl.data[lEntryOffset];
			alignedKlen = ALIGNED_LENGTH(lEntry->klen);
            cursor->oid = *(ObjectID*)&lEntry->kval[alignedKlen];
            cursor->key.len = lEntry->klen;
            memcpy(cursor->key.val, lEntry->kval, lEntry->klen);
            cursor->leaf = *leafPid;
            cursor->slotNo = slotNo;
            cmp = edubtm_KeyCompare(kdesc, &cursor->key, stopKval);
            if ((stopCompOp == SM_LT && cmp != LESS) ||
                (stopCompOp == SM_LE && cmp == GREATER) ||
                (stopCompOp == SM_GT && cmp != GREATER) ||
                (stopCompOp == SM_GE && cmp == LESS)){
                cursor->flag = CURSOR_EOS;
            }
        }
        
        e = BfM_FreeTrain(leafPid, PAGE_BUF);
        if (e < eNOERROR) ERR(e);
    }
    return(eNOERROR);
    
} /* edubtm_Fetch() */

