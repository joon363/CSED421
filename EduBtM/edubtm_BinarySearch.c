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
 * Module: edubtm_BinarySearch.c
 *
 * Description :
 *  This file has three function about searching. All these functions use the
 *  binary search algorithm. If the entry is found successfully, it returns 
 *  TRUE and the correct position as an index, otherwise, it returns FALSE and
 *  the index whose key value is the largest in the given page but less than
 *  the given key value in the function edubtm_BinarSearchInternal; in the
 *  function edubtm_BinarySearchLeaf() the index whose key value is the smallest
 *  in the given page but larger than the given key value.
 *
 * Exports:
 *  Boolean edubtm_BinarySearchInternal(BtreeInternal*, KeyDesc*, KeyValue*, Two*)
 *  Boolean edubtm_BinarySearchLeaf(BtreeLeaf*, KeyDesc*, KeyValue*, Two*)
 */


#include "EduBtM_common.h"
#include "EduBtM_Internal.h"



/*@================================
 * edubtm_BinarySearchInternal()
 *================================*/
/*
 * Function:  Boolean edubtm_BinarySearchInternal(BtreeInternal*, KeyDesc*,
 *                                             KeyValue*, Two*)
 *
 * Description:
 * (Following description is for original ODYSSEUS/COSMOS BtM.
 *  For ODYSSEUS/EduCOSMOS EduBtM, refer to the EduBtM project manual.)
 *
 *  Search the internal entry of which value equals to or less than the given
 *  key value.
 *
 * Returns:
 *  Result of search: TRUE if the same key is found, otherwise FALSE
 *
 * Side effects:
 *  1) parameter idx : slot No of the slot having the key equal to or
 *                     less than the given key value
 *                     
 */
Boolean edubtm_BinarySearchInternal(
    BtreeInternal 	*ipage,		/* IN Page Pointer to an internal page */
    KeyDesc       	*kdesc,		/* IN key descriptor */
    KeyValue      	*kval,		/* IN key value */
    Two          	*idx)		/* OUT index to be returned */
{
    Two  		low;		/* low index */
    Two  		mid;		/* mid index */
    Two  		high;		/* high index */
    Four 		cmp;		/* result of comparison */
    btm_InternalEntry 	*entry;	/* an internal entry */

    
    /* Error check whether using not supported functionality by EduBtM */
    int i;
    for(i=0; i<kdesc->nparts; i++)
    {
        if(kdesc->kpart[i].type!=SM_INT && kdesc->kpart[i].type!=SM_VARSTRING)
            ERR(eNOTSUPPORTED_EDUBTM);
    }
    // Basic Binary Search with index
    low = 0;
    high = ipage->hdr.nSlots - 1;

    while (low <= high){
        mid = (low + high) / 2;
        entry = (btm_InternalEntry*)&ipage->data[ipage->slot[-mid]];
        cmp = edubtm_KeyCompare(kdesc, kval, &entry->klen);
        switch (cmp)
        {
        case GREATER:
            low = mid + 1;
            break;
        case LESS:
            high = mid - 1;
            break;        
        default: // EQUAL
            *idx = mid;
            return TRUE; // found!
            break;
        }
    }

    *idx = high; 
    return FALSE; // not found
    /* note: 주어진 키보다 작은게 하나도 없다면, high=mid-1; 했을때 
    low=0, high=-1 에서 종료됨. 따라서 *idx=-1과 동일한 효과를 낸다.
    */
    
} /* edubtm_BinarySearchInternal() */



/*@================================
 * edubtm_BinarySearchLeaf()
 *================================*/
/*
 * Function: Boolean edubtm_BinarySearchLeaf(BtreeLeaf*, KeyDesc*,
 *                                        KeyValue*, Two*)
 *
 * Description:
 * (Following description is for original ODYSSEUS/COSMOS BtM.
 *  For ODYSSEUS/EduCOSMOS EduBtM, refer to the EduBtM project manual.)
 *
 *  Search the leaf item of which value equals to or less than the given
 *  key value.
 *
 * Returns:
 *  Result of search: TRUE if the same key is found, FALSE otherwise
 *
 * Side effects:
 *  1) parameter idx: slot No of the slot having the key equal to or
 *                    less than the given key value
 */
Boolean edubtm_BinarySearchLeaf(
    BtreeLeaf 		*lpage,		/* IN Page Pointer to a leaf page */
    KeyDesc   		*kdesc,		/* IN key descriptor */
    KeyValue  		*kval,		/* IN key value */
    Two       		*idx)		/* OUT index to be returned */
{
    Two  		low;		/* low index */
    Two  		mid;		/* mid index */
    Two  		high;		/* high index */
    Four 		cmp;		/* result of comparison */
    btm_LeafEntry 	*entry;		/* a leaf entry */


    /* Error check whether using not supported functionality by EduBtM */
    int i;
    for(i=0; i<kdesc->nparts; i++)
    {
        if(kdesc->kpart[i].type!=SM_INT && kdesc->kpart[i].type!=SM_VARSTRING)
            ERR(eNOTSUPPORTED_EDUBTM);
    }
    /*
    파라미터로주어진key 값과같은key 값을갖는index entry가존재하는경우,
     – 해당index entry의 slot 번호 및 TRUE를 반환함*/
    /*
    파라미터로주어진key 값과같은key 값을갖는index entry가존재하지않는경우,
    – 파라미터로주어진key 값보다작은key 값을갖는index entry들중
      가장큰key 값을갖는index entry의 slot 번호 및 FALSE를 반환함. 
      주어진key 값보다 작은key 값을갖는entry가 없을경우slot 번호로-1을 반환함.
    */

    // Basic Binary Search with index
    low = 0;
    high = lpage->hdr.nSlots - 1;

    while (low <= high){
        mid = (low + high) / 2;
        entry = (btm_LeafEntry*)&lpage->data[lpage->slot[-mid]];
        cmp = edubtm_KeyCompare(kdesc, kval, &entry->klen);
        switch (cmp)
        {
        case GREATER:
            low = mid + 1;
            break;
        case LESS:
            high = mid - 1;
            break;        
        default: // EQUAL
            *idx = mid;
            return TRUE; // found!
            break;
        }
    }

    *idx = high; 
    return FALSE; // not found
    /* note: 주어진 키보다 작은게 하나도 없다면, high=mid-1; 했을때 
    low=0, high=-1 에서 종료됨. 따라서 *idx=-1과 동일한 효과를 낸다.
    */
} /* edubtm_BinarySearchLeaf() */
