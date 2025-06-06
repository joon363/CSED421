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
 * Module: EduBfM_FlushAll.c
 *
 * Description :
 *  Flush dirty buffers holding trains.
 *
 * Exports:
 *  Four EduBfM_FlushAll(void)
 */


#include "EduBfM_common.h"
#include "EduBfM_Internal.h"



/*@================================
 * EduBfM_FlushAll()
 *================================*/
/*
 * Function: Four EduBfM_FlushAll(void)
 *
 * Description :
 * (Following description is for original ODYSSEUS/COSMOS BfM.
 *  For ODYSSEUS/EduCOSMOS EduBfM, refer to the EduBfM project manual.)
 *
 *  Flush dirty buffers holding trains.
 *  A dirty buffer is one with the dirty bit set.
 *
 * Returns:
 *  error code
 */
Four EduBfM_FlushAll(void)
{
    Four        e;                      /* error */
    Two         i;                      /* index */
    Four        type;                   /* buffer type */

    // For All Type of Buffer Pools
    for (type = 0; type < NUM_BUF_TYPES; type++) {
        // For All Buffers
        for (i = 0; i < BI_NBUFS(type); i++) {
            if (BI_BITS(type, i) !=ALL_0 ) {
                // Flush Active Buffers
                e = edubfm_FlushTrain(&BI_KEY(type, i), type);
                if (e != eNOERROR) return e;
            }
        }
    }

    return( eNOERROR );
    
}  /* EduBfM_FlushAll() */

