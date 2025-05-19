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
    /*파라미터로주어진startCompOp가 SM_BOF일 경우,
    – B+ tree 색인의 첫 번째object (가장 작은 key 값을 갖는 leaf index entry) 를 검색함. */
    if (startCompOp == SM_BOF){
        e = edubtm_FirstObject(root, kdesc, stopKval, stopCompOp, cursor);
        if (e < eNOERROR) ERR(e);
    }
    /*파라미터로주어진startCompOp가 SM_EOF일 경우,
    – B+ tree 색인의 마지막 object (가장 큰 key 값을 갖는 leaf index entry) 를검색함. */ 
    else if (startCompOp == SM_EOF){
        e = edubtm_LastObject(root, kdesc, stopKval, stopCompOp, cursor);
        if (e < eNOERROR) ERR(e);      
    } 
    
    /* 이외의경우, edubtm_Fetch()를 호출하여 B+ tree 색인에서 검색 조건을 만족하는 첫번째
    <object의 key, object ID> pair가 저장된 leaf index entry를 검색함 */
    else{
        e = edubtm_Fetch(root, kdesc, startKval, startCompOp, stopKval, stopCompOp, cursor);
        if (e < eNOERROR) ERR(e);
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
    Two                 tmp;
    Two                 invalidCondition;


    /* Error check whether using not supported functionality by EduBtM */
    int i;
    for(i=0; i<kdesc->nparts; i++)
    {
        if(kdesc->kpart[i].type!=SM_INT && kdesc->kpart[i].type!=SM_VARSTRING)
            ERR(eNOTSUPPORTED_EDUBTM);
    }

    e = BfM_GetTrain(root, (char**)&apage, PAGE_BUF);
    if (e < eNOERROR) ERR(e);

    if (apage->any.hdr.type & INTERNAL) {
        edubtm_BinarySearchInternal(apage, kdesc, startKval, &idx);

        if (idx >= 0) {
            iEntryOffset = apage->bi.slot[-idx];
            iEntry = (btm_InternalEntry*)&apage->bi.data[iEntryOffset];
            MAKE_PAGEID(child, root->volNo, iEntry->spid);
        } else {
            MAKE_PAGEID(child, root->volNo, apage->bi.hdr.p0);
        }

        e = edubtm_Fetch(&child, kdesc, startKval, startCompOp, stopKval, stopCompOp, cursor);
        if (e < eNOERROR) ERR(e);
        
        e = BfM_FreeTrain(root, PAGE_BUF);
        if (e < eNOERROR) ERR(e);
    }
    else if (apage->any.hdr.type & LEAF) {
        found = edubtm_BinarySearchLeaf(apage, kdesc, startKval, &idx);
        leafPid = root;
        cursor->flag = (One)CURSOR_ON;
        switch (startCompOp){
        case SM_EQ:
            slotNo = idx;
            if (!found){
                cursor->flag = (One)CURSOR_EOS;
            }
            break;

        case SM_LE:
            slotNo = idx;
            break;
        case SM_GE:
            slotNo = idx;
            if (!found){ // lower than key
                slotNo += 1;
            }
            break;
        case SM_LT:
            slotNo = idx; // lower than key only if not found
            if (found){
                slotNo -= 1;
            }
            break;
        case SM_GT:
            slotNo = idx + 1;

        default:
            break;
        }

        if (slotNo < 0){
            if (apage->bl.hdr.prevPage == NIL){
                cursor->flag = (One)CURSOR_EOS;
            } else{
                MAKE_PAGEID(prevPid, root->volNo, apage->bl.hdr.prevPage);
                leafPid = &prevPid;
            }

        } else if(slotNo >= apage->bl.hdr.nSlots){

            if (apage->bl.hdr.nextPage == NIL){
                cursor->flag = (One)CURSOR_EOS;
            } else{
                MAKE_PAGEID(nextPid, root->volNo, apage->bl.hdr.nextPage);
                leafPid = &nextPid;
                slotNo = 0;
            }
        }

        e = BfM_FreeTrain(root, PAGE_BUF);
        if (e < 0) ERR(e);

        e = BfM_GetTrain(leafPid, (char**)&apage, PAGE_BUF);
        if (e < 0) ERR(e);

        if (cursor->flag != CURSOR_EOS){ // determine EOS or not
            
            if (slotNo < 0){
                slotNo = apage->bl.hdr.nSlots - 1;
            }

            cursor->slotNo = slotNo;
            cursor->leaf = *leafPid;

            lEntryOffset = apage->bl.slot[-slotNo];
            lEntry = (btm_LeafEntry*)&apage->bl.data[lEntryOffset];
			alignedKlen = ALIGNED_LENGTH(lEntry->klen);
            memcpy(&cursor->key, &lEntry->klen, sizeof(KeyValue));
            // memcpy(&cursor->oid, &lEntry->kval + alignedKlen, sizeof(ObjectID));
            cursor->oid = *(ObjectID*)&lEntry->kval[alignedKlen];
            
            invalidCondition = FALSE;
            cmp = edubtm_KeyCompare(kdesc, &cursor->key, stopKval);

            switch(stopCompOp){
                case SM_LT:
                    invalidCondition = cmp != LESS;
                    break;
                case SM_LE:
                    invalidCondition = (cmp == GREATER);
                    break;
                case SM_GT:
                    invalidCondition = cmp != GREATER;
                    break;
                case SM_GE:
                    invalidCondition = (cmp == LESS);
                    break;
                default:
                    break;
            }

            if (invalidCondition){
                cursor->flag = CURSOR_EOS;
            }
        }

        e = BfM_FreeTrain(leafPid, PAGE_BUF);
        if(e < 0) ERR(e);
    }

    return(eNOERROR);

    
} /* edubtm_Fetch() */
