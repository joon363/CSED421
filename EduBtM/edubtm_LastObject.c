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
 * Module: edubtm_LastObject.c
 *
 * Description : 
 *  Find the last ObjectID of the given Btree.
 *
 * Exports:
 *  Four edubtm_LastObject(PageID*, KeyDesc*, KeyValue*, Four, BtreeCursor*) 
 */


#include <string.h>
#include "EduBtM_common.h"
#include "BfM.h"
#include "EduBtM_Internal.h"



/*@================================
 * edubtm_LastObject()
 *================================*/
/*
 * Function:  Four edubtm_LastObject(PageID*, KeyDesc*, KeyValue*, Four, BtreeCursor*) 
 *
 * Description : 
 * (Following description is for original ODYSSEUS/COSMOS BtM.
 *  For ODYSSEUS/EduCOSMOS EduBtM, refer to the EduBtM project manual.)
 *
 *  Find the last ObjectID of the given Btree. The 'cursor' will indicate
 *  the last ObjectID in the Btree, and it will be used as successive access
 *  by using the Btree.
 *
 * Returns:
 *  error code
 *    eBADPAGE_BTM
 *    some errors caused by function calls
 *
 * Side effects:
 *  cursor : the last ObjectID and its position in the Btree
 */
Four edubtm_LastObject(
    PageID   		*root,		/* IN the root of Btree */
    KeyDesc  		*kdesc,		/* IN key descriptor */
    KeyValue 		*stopKval,	/* IN key value of stop condition */
    Four     		stopCompOp,	/* IN comparison operator of stop condition */
    BtreeCursor 	*cursor)	/* OUT the last BtreeCursor to be returned */
{
    int			i;
    Four 		e;		/* error number */
    Four 		cmp;		/* result of comparison */
    BtreePage 		*apage;		/* pointer to the buffer holding current page */
    BtreeOverflow 	*opage;		/* pointer to the buffer holding overflow page */
    PageID 		curPid;		/* PageID of the current page */
    PageID 		child;		/* PageID of the child page */
    PageID 		ovPid;		/* PageID of the current overflow page */
    PageID 		nextOvPid;	/* PageID of the next overflow page */
    Two 		lEntryOffset;	/* starting offset of a leaf entry */
    Two 		iEntryOffset;	/* starting offset of an internal entry */
    btm_LeafEntry 	*lEntry;	/* a leaf entry */
    btm_InternalEntry 	*iEntry;	/* an internal entry */
    Two                 klen;    /* length of the key length */
    Four 		alignedKlen;	/* aligned length of the key length */
    Two slotIdx;
        

    if (root == NULL) ERR(eBADPAGE_BTM);

    /* Error check whether using not supported functionality by EduBtM */
    for(i=0; i<kdesc->nparts; i++)
    {
        if(kdesc->kpart[i].type!=SM_INT && kdesc->kpart[i].type!=SM_VARSTRING)
            ERR(eNOTSUPPORTED_EDUBTM);
    }
    
    /*B+ tree 색인에서 마지막 object (가장 큰 key값을 갖는leaf index entry) 를 검색함*/
    // find rightmost leaf
    curPid = *root;
    e = BfM_GetTrain(&curPid, (char**)&apage, PAGE_BUF);
    if (e < eNOERROR) ERR(e);
    while(!(apage->any.hdr.type & LEAF)){
        slotIdx = apage->bi.hdr.nSlots - 1;
        iEntryOffset = apage->bi.slot[-slotIdx];
        iEntry = (btm_InternalEntry*)&apage->bi.data[iEntryOffset];
        MAKE_PAGEID(child, curPid.volNo, iEntry->spid);

        e = BfM_FreeTrain(&curPid, PAGE_BUF);
        if (e < eNOERROR) ERR(e);
        e = BfM_GetTrain(&child, (char**)&apage, PAGE_BUF);
        if (e < eNOERROR) ERR(e);
        curPid = child;
    }
    
    /* B+ tree 색인의 마지막 leaf page의 마지막 index entry (slot 번호 = nSlots- 1) 를 
    가리키는 cursor를 반환함 */
    slotIdx = apage->bl.hdr.nSlots - 1;
    lEntryOffset = apage->bl.slot[-slotIdx];
    lEntry = (btm_LeafEntry*)&apage->bl.data[lEntryOffset];
    alignedKlen = ALIGNED_LENGTH(lEntry->klen);

    cursor->key.len = lEntry->klen;
    memcpy(cursor->key.val, lEntry->kval, lEntry->klen);

    cursor->leaf = curPid;
    cursor->oid = *(ObjectID*)&lEntry->kval[alignedKlen];
    cursor->slotNo = slotIdx;
    /* note: cursor->overflow: 
    중복key 사용시 동일key 값을갖는object들의ID (OID) 들이 
    저장된page의 page ID로서, 유일 key만을 사용하는 EduBtM에서는 overflow 사용하지않음*/
    
    cmp = edubtm_KeyCompare(kdesc, stopKval, &cursor->key);
    // 검색종료key 값이마지막object의 key 값 보다 크거나, 
    // key 값은 같으나 검색종료연산이SM_GT인경우 CURSOR_EOS 반환
    cursor->flag = (cmp==GREATER || (cmp==EQUAL && stopCompOp == SM_GT)) ?
        CURSOR_EOS:CURSOR_ON;
    
    e = BfM_FreeTrain(&curPid, PAGE_BUF);
    if (e < eNOERROR) ERR(e);
    return(eNOERROR);
    
} /* edubtm_LastObject() */
