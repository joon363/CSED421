#ifndef _UTIL_HASH_
#define _UTIL_HASH_

#include "uthash.h"
#include "EduBtM_common.h"
#include "EduBtM_basictypes.h"
#include "EduBtM_TestModule.h"
#include "EduBtM.h"

struct objectMapStruct {
    Four 		intKey;
	char		stringKey[MAXKEY];
	ObjectID	oid;
    UT_hash_handle hh; /* makes this structure hashable */
};

extern const struct objectMapStruct *objectMap;

Boolean isExist(Four, Four, char[MAXKEY]);
void addObject(Four, Four , char[MAXKEY], ObjectID );
struct objectMapStruct *findObject(Four, Four, char[MAXKEY]);
void deleteObject(struct objectMapStruct *);
void deleteAll();
void sort(Four);
int stringSort(struct objectMapStruct *, struct objectMapStruct *);
int intSort(struct objectMapStruct *, struct objectMapStruct *);
struct objectMapStruct* fetch(Four, Four, Four, Four, char[MAXKEY], Four, char[MAXKEY]);
struct objectMapStruct* fetchNext(Four, Four, struct objectMapStruct* , Four , char [MAXKEY]);
Four compare(Four , Four , Four , char[MAXKEY], char[MAXKEY]);
Boolean sameObject(Four , struct objectMapStruct *, Four , char [MAXKEY], ObjectID );

#endif