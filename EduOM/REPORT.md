# EduOM Report

Name: JunHyeok Park

Student id: 20220312

# Problem Analysis

In this project, we implement Object manager system. 
Each file has sevarel pages, and each page is slotted.
Each object is inside the page, and its information is at page.
Object can be Created, Destroyed, Searched, Read.
Page can obtain free space by compaction.
Main consideration is edge cases; last object in the page, last page in the file, first object in the page, etc.

# Design For Problem Solving

## High Level

Implemented functions:
- CreateObject
    - createObject
    - CompactPage
- DestroyObject
- NextObject, PrevObject
- ReadObject

Pre-defined API functions
- RDsM_AllocTrains
- RDsM_PageIdToExtNo
- BfM_GetTrain
- BfM_GetNewTrain
- BfM_FreeTrain
- BfM_SetDirty
- Util_getElementFromPool
- om_GetUnique
- om_FileMapAddPage
- om_FileMapDeletePage
- om_PutInAvailSpaceList
- om_RemoveFromAvailSpaceList

## Low Level

### Some Macros
- Dealing with File Example

    ```
    volNo = catObjForFile->volNo;
    pageNo = catObjForFile->pageNo;
    MAKE_PHYSICALFILEID(pFid, volNo, pageNo);
    e = BfM_GetTrain(&pFid, &catPage, PAGE_BUF);
    if(e < eNOERROR) ERR(e);
    GET_PTR_TO_CATENTRY_FOR_DATA(catObjForFile, catPage, catEntry);
    ```
- Dealing with Page Example

    ```
    volNo = curOID->volNo;
    pageNo = apage->header.prevPage;
    MAKE_PAGEID(pid, volNo, pageNo);
    e = BfM_GetTrain(&pid, &apage, PAGE_BUF);
    if (e < eNOERROR) ERR(e);
    ```
- Dealing with Object Example
    ``` 
    /* OUT the next Object of a current Object */
    volNo = pid.volNo;
    pageNo = pid.pageNo;
    MAKE_OBJECTID(*nextOID, volNo, pageNo, i, apage->slot[-i].unique);

    /* OUT the object header of next object */
    offset = apage->slot[-i].offset;
    obj = &(apage->data[offset]);
    objHdr = &obj->header; 
    ```
### Implementation Dependencies
- CreateObject
    - needs createObject
    - needs CompactPage
    - needs DestroyObject for test cases to be ran.


# Mapping Between Implementation And the Design

in order of ppt slide.
## CreateObject
first, calculate the needed space of the object.

```neededSpace = sizeof(ObjectHdr) + alignedLen + sizeof(SlottedPageSlot);```

- if nearObj != NULL, check if that page has enough free space.

    SP_FREE() indicates it. 

    - if there are enough free space but no contiguous free area (SP_CFREE), compaction should be done.
    
    - else, Allocate new page using RDsM_AllocTrains to create object.

- if nearObj == NULL, find somewhere in the file using availSpaceList.
    ```
    if (neededSpace <= SP_10SIZE){
        availListPageNo = catEntry->availSpaceList10;
        MAKE_PAGEID(pid, pFid.volNo, catEntry->availSpaceList10);
    }  
    else if (neededSpace <= SP_20SIZE){
        availListPageNo = catEntry->availSpaceList10;
        MAKE_PAGEID(pid, pFid.volNo, catEntry->availSpaceList10);
    }    
    ...
    ```
    - if there is any availSpaceList, add into it.
    - else, add it to last page of file or add one more page.

- Now, set Object Headers and page headers, copy data into approciate slot.

```memcpy(obj->data, data, length);```

- Then, put page into available space list using API function.

```
e = om_PutInAvailSpaceList(catObjForFile, &pid, apage);
```


## CompactPage
pretty straightforward.
just move every objects from the beginning  of the page.
```
for (i = 0; i < tpage.header.nSlots; i++) {
    if (tpage.slot[-i].offset == EMPTYSLOT || (slotNo!=NIL && slotNo == i)) continue;
    obj = &(tpage.data[tpage.slot[-i].offset]);
    alignedLen= ALIGNED_LENGTH(obj->header.length);
    len = alignedLen + sizeof(ObjectHdr);
    memcpy((apage->data)+apageDataOffset, obj, len);
    apage->slot[-i].offset = apageDataOffset;
    apageDataOffset += len;
}
```

## DestroyObject
- Remove From AvailSpaceList, reset offset to EMPTYSLOT.
- if the object was last one in the page, deallocate page.
    - modify dl pool linked list pointers to do this.
```
// – Page를 file 구성 page들로 이루어진 list에서 삭제함
e = om_FileMapDeletePage(catObjForFile, &pid);
if (e < eNOERROR) ERR(e);

//  – 해당 page를 deallocate 함
//      » 파라미터로 주어진 dlPool에서 새로운 dealloc list element 한 개를 할당 받음
//          • Dealloc list: deallocate 할 page들의 linked list
e = Util_getElementFromPool(dlPool, &dlElem);
if (e < eNOERROR) ERR(e);

// » 할당받은 element에 deallocate 할 page 정보를 저장함
dlElem->type = DL_PAGE;
dlElem->elem.pid = pid;

// » Deallocate 할 page 정보가 저장된 element를 dealloc list의 첫 번째 element로 삽입함
// 링크드 리스트 기반 구현
dlElem->next = dlHead->next;
dlHead->next = dlElem; 
```
## NextObject, ## PrevObject
- Corner Cases are important.
prev
- return previous object
- if the object was first one in the page, return last one of prev page.
- if the object was first one in the file, return nothing.
next
- return next object
- if the object was last one in the page, return first one of next page.
- if the object was last one in the last page of the file, return nothing.

returning object example
```
/* OUT the next Object of a current Object */
volNo = pid.volNo;
pageNo = pid.pageNo;
MAKE_OBJECTID(*nextOID, volNo, pageNo, i, apage->slot[-i].unique);

/* OUT the object header of next object */
offset = apage->slot[-i].offset;
obj = &(apage->data[offset]);
objHdr = &obj->header; 
```

## ReadObject
- if length is REMAINDER: return all data from starting point.
- else, return only amount of length.
- read by calling memcpy to OUTPUT pointer buf.

```
length = (length==REMAINDER)? obj->header.length-start: length;
memcpy(buf, &(obj->data[start]), length);
```