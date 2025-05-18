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
 * Module: edubtm_FreePages.c
 *
 * Description :
 *  Free all pages which were related with the given page. 
 *
 * Exports:
 *  Four edubtm_FreePages(FileID*, PageID*, Pool*, DeallocListElem*)
 */


#include "EduBtM_common.h"
#include "Util.h"
#include "BfM.h"
#include "EduBtM_Internal.h"



/*@================================
 * edubtm_FreePages()
 *================================*/
/*
 * Function: Four edubtm_FreePages(FileID*, PageID*, Pool*, DeallocListElem*)
 *
 * Description :
 * (Following description is for original ODYSSEUS/COSMOS BtM.
 *  For ODYSSEUS/EduCOSMOS EduBtM, refer to the EduBtM project manual.)
 *
 *  Free all pages which were related with the given page. If the given page
 *  is an internal page, recursively free all child pages before it is freed.
 *  In a leaf page, examine all leaf items whether it has an overflow page list
 *  before it is freed. If it has, recursively call itself by using the first
 *  overflow page. In an overflow page, it recursively calls itself if the
 *  'nextPage' exist.
 *
 * Returns:
 *  error code
 *    eBADBTREEPAGE_BTM
 *    some errors caused by function calls
 */
Four edubtm_FreePages(
    PhysicalFileID      *pFid,          /* IN FileID of the Btree file */
    PageID              *curPid,        /* IN The PageID to be freed */
    Pool                *dlPool,        /* INOUT pool of dealloc list elements */
    DeallocListElem     *dlHead)        /* INOUT head of the dealloc list */
{
    Four                e;              /* error number */
    Two                 i;              /* index */
    Two                 alignedKlen;    /* aligned length of the key length */
    PageID              tPid;           /* a temporary PageID */
    PageID              ovPid;          /* a temporary PageID of an overflow page */
    BtreePage           *apage;         /* a page pointer */
    BtreeOverflow       *opage;         /* page pointer to a buffer holding an overflow page */
    Two                 iEntryOffset;   /* starting offset of an internal entry */
    Two                 lEntryOffset;   /* starting offset of a leaf entry */
    btm_InternalEntry   *iEntry;        /* an internal entry */
    btm_LeafEntry       *lEntry;        /* a leaf entry */
    DeallocListElem     *dlElem;        /* an element of dealloc list */

    /* Page header의 type에서 해당 page가 deallocate 될 page임을 나타내는 bit를set
     및 나머지bit들을 unset 함*/
    e = BfM_GetTrain(curPid, (char**)&apage, PAGE_BUF);
    if (e < eNOERROR) ERR(e);

    if(apage->any.hdr.type & INTERNAL){
        /*If the given page is an internal page, recursively free all child pages before it is freed.*/
        MAKE_PAGEID(tPid, curPid->volNo, apage->bi.hdr.p0);
        e = edubtm_FreePages(pFid, &tPid, dlPool, dlHead);
        if (e < eNOERROR) ERR(e);

        for (i = 0; i<apage->bi.hdr.nSlots; i++){
            iEntryOffset = apage->bi.slot[-i];
            iEntry = (btm_InternalEntry*)(&apage->bi.data[iEntryOffset]);

            MAKE_PAGEID(tPid, curPid->volNo, iEntry->spid);
            e = edubtm_FreePages(pFid, &tPid, dlPool, dlHead);
            if (e < eNOERROR) ERR(e);
        }
    }
    /* Note: EduBtM에서는 Overflow 고려 안함.*/
    
    // Allocate a new dealloc list element from the dlPool given as a parameter
    e = Util_getElementFromPool(dlPool, &dlElem);
    if (e < eNOERROR) ERR(e);
    // Store the information about the pages to be deallocated into the element allocated.
    dlElem->type = DL_PAGE;
    dlElem->elem.pid = tPid;
    // Insert the element into the dealloc list as the first element.
    dlElem->next = dlHead->next;
    dlHead->next = dlElem; 
    
    return(eNOERROR);
    
}   /* edubtm_FreePages() */
