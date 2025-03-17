# EduBfM Report

Name: JunHyeok Park

Student id: 20220312

# Problem Analysis

We need to implement simple buffer architecture with Second-Chance Replacement policy and hash table.

# Design For Problem Solving

## High Level

The Design is very straightforward. 
There are Two (NUM_BUF_TYPES) Buffer Pools, and each buffer has BI_NBUFS entries.
hashTable deal with collision by adding elements in Linked List.

## Low Level

Disk:
- Use RDsM_ReadTrain and RDsM_WriteTrain

Buffer Pool: 
- Use macros defined in EduBfM_Internal.h
- BI_BUFFERPOOL, BI_BUFSIZE, BI_NEXTVICTIM, BI_BUFFER
- BI_NBUFS, BI_KEY, BI_FIXED, BI_BITS, BI_NEXTHASHENTRY

Hash Table:
- Use macros defined in EduBfM_Internal.h
- BI_HASHTABLE, BI_HASHTABLEENTRY, HASHTABLESIZE


# Mapping Between Implementation And the Design
in order of ppt slide.
## GetTrain
- if there is a page(edubfm_LookUp(trainId,type)!=NIL), just increase fixed by 1, set REFER bit.
- else, call AllocTrain, ReadTrain to get data and initialize table data, insert to table.

## FreeTrain
- if there is page (edubfm_LookUp(trainId,type)!=NIL), decrease fixed by 1.
- if fixed is lower than zero, print some statements and set fixed to 0. 
```
printf("fixed counter is less than 0!!!\n");
printf("trainId = {%d, %d}\n", BI_KEY(type, index).volNo, BI_KEY(type, index).pageNo);
BI_FIXED(type, index) = 0;
```
## SetDirty
- lookup for the buffer table with ID, then set dirty bit.
- bit operation | is used.
```
index=edubfm_LookUp(trainId,type);
if(index!=NIL){
    BI_BITS(type, index) |=DIRTY;
}
else return eNOTFOUND_BFM;
```

## FlushAll
- For all buffer Pools ```for(type = 0; type < NUM_BUF_TYPES; type++)```,
- For all buffers, ```for (i = 0; i < BI_NBUFS(type); i++) ```
- Flush Active Buffers 
```
if (BI_BITS(type, i) !=ALL_0 ) {
    // Flush Active Buffers
    e = edubfm_FlushTrain(&BI_KEY(type, i), type);
    if (e != eNOERROR) return e;
}edubfm_FlushTrain(&BI_KEY(type, i), type);
```
- it is OK to select ```!=ALL_0``` Buffers only because FlushTrain checks Dirty bit.

## DiscardAll
- simple. Reset key and bits to NIL and 0x0 in buffer pool
- then call deleteAll function (flush table)
```
for (type = 0; type < NUM_BUF_TYPES; type++) {
    for (i = 0; i < BI_NBUFS(type); i++) {
        // 각 bufTable의 모든 element들을 초기화함– key: set pageNo to NIL(-1)
        SET_NILBFMHASHKEY(BI_KEY(type, i));
        BI_BITS(type, i)=ALL_0;
    }
}
// 각hashTable에 저장된 모든 entry (즉, array index) 들을 삭제함
e = edubfm_DeleteAll();
```
# Internal Functions
## ReadTrain
- simply call RDsm_ReadTrain with given arguments
```
bufSize = BI_BUFSIZE(type);
e = RDsM_ReadTrain(trainId,aTrain,bufSize);
if (e != eNOERROR) return e;
```

## AllocTrain
- it works with second-chance algorithm.
- to implement this, I chose to use modular operation so that victim can chosen in clockwise manner. 
- ```i=(i + 1) % BI_NBUFS(type)```
```
if (BI_BITS(type, i) & REFER) {
    BI_BITS(type, i) &= ~REFER;
}
else {
    victim = i;
    BI_NEXTVICTIM(type) = (victim + 1) % BI_NBUFS(type);
    // 선정된 buffer element에 저장되어 있던 page/train이 수정된 경우, 기존 buffer element의 내용을 disk로 flush함
    if (BI_BITS(type, victim) & DIRTY) {
        edubfm_FlushTrain(&BI_KEY(type, victim), type);
    }
    // 선정된 buffer element와 관련된 데이터 구조를 초기화함
    BI_BITS(type, victim) = ALL_0;

    // 선정된 buffer element의 array index (hashTable entry) 를 hashTable에서 삭제함
    edubfm_Delete(&BI_KEY(type, victim), type);
    break;
}
```
- if reference bit is set, give a chance and continue.
- else, flush this buffer and delete from table.

## FlushTrain
- lookup first
- flush if dirty bit is set ```BI_BITS(type, index) & DIRTY```
  - then write back to disk with ```RDsM_WriteTrain```
- then set dirty bit back to 0 ```BI_BITS(type, index) &= ~DIRTY;```
# edubfm_Hash.c
## Insert
- First, calculate key ```hashValue = BFM_HASH(key,type);```.
- if target exists in hash table, set old one to next entry (linked list)
    ```
    BI_HASHTABLEENTRY(type, hashValue) = index;
    BI_NEXTHASHENTRY(type, index) = i;
    ```
- else just set table entry with index. 
``` BI_HASHTABLEENTRY(type, hashValue) = index;```

## Delete
- if there is no linked list at the entry, just set entry to -1.
``` BI_HASHTABLEENTRY(type, hashValue) = NIL;```
- else, traverse linked list and reconnect link.
```
prev = i;
i = BI_NEXTHASHENTRY(type, i);
if (EQUALKEY(&BI_KEY(type, i), key)) {
    BI_NEXTHASHENTRY(type, prev) = BI_NEXTHASHENTRY(type, i);
    return eNOERROR;
}
```
## DeleteAll
- simply delete every entry in all buffer pools and table entries.
```
for (type = 0; type < NUM_BUF_TYPES; type++) {
    tableSize = HASHTABLESIZE(type);
    for (i = 0; i < tableSize; i++) {
        // Delete Entry
        BI_HASHTABLEENTRY(type,i) = NIL;
    }
}
```
## LookUp
- simply traverse linked list and return entry.
```
hashValue = BFM_HASH(key, type);
i = BI_HASHTABLEENTRY(type, hashValue);

while (i != NIL) {
    if (EQUALKEY(&BI_KEY(type, i), key)) return i;
    i = BI_NEXTHASHENTRY(type, i);
}
```
