# EduBtM Report

Name: JunHyeok Park

Student id: 20220312

# Design For Problem Solving

## Low Level

### Some Macros
- Dealing with File Example

    ```
    e = BfM_GetTrain(catObjForFile, (char**)&catPage, PAGE_BUF);
    if (e < eNOERROR) ERR(e);
    GET_PTR_TO_CATENTRY_FOR_BTREE(catObjForFile, catPage, catEntry);
    MAKE_PHYSICALFILEID(pFid, catEntry->fid.volNo, catEntry->firstPage);
    ```
- Dealing with Page Example

    ```
    e = BfM_GetTrain(&curPid, (char**)&apage, PAGE_BUF);
    if (e < eNOERROR) ERR(e);
    ```
- Memory Layouts

    - InternalEntry
        ```
        --------------------------------------
        |     spid    |   klen   |   kval[]  |
        --------------------------------------
        ShortPageID       Two      entry->klen
        ```

    - LeafEntry
        ```
        -----------------------------------------------------
        |  nObjects |  klen  |   key   |  value(Object ID)  |
        -----------------------------------------------------
            Two        Two   (aligned)klen    ObjectID
        ```

    - Key
        ```
        -----------------------------
                type   |   (len)   |   (data)
                -----------------------------
                4 bytes    2 bytes     len bytes
        ```

    - LeafItem
        ```
        ----------------------------------------------------
         oid      | nObjects |  klen  | char kval[MAXKEYLEN]
         ObjectID |    Two   |  Two   |
        ----------------------------------------------------
                             [           KeyValue          ]
        ```

# Mapping Between Implementation And the Design

- EduBtM_CreateIndex

    btm_AllocPage()를 호출하여 색인 file의 첫 번째 page를 할당받고, 할당받은page를 root page로 초기화함 (edubtm_InitLeaf)

    - edubtm_InitLeaf: Leaf 페이지로 초기화, 헤더 설정함.

- EduBtM_DropIndex

    색인file에서 B+ tree 색인을 삭제함

    - edubtm_FreePages: 
    
        Page header의 type에서 해당 page가 deallocate 될 page임을 나타내는 bit를set 및 나머지 bit들을 unset함.

- EduBtM_InsertObject

    edubtm_Insert()를 호출하여 새로운 object에 대한 <object의 key, object ID> pair를 
    B+ tree 색인에 삽입함.
    Root page에서 split이 발생하여 새로운 root page 생성이필요한경우, edubtm_root_insert()를 호출하여 이를처리함

    - edubtm_Insert: 
        - edubtm_BinarySearchInternal: low, mid, high를 사용해 이진 탐색을 시행한다. 
            - edubtm_KeyCompare: KeyDesc를 따라 순회하면서 키의 대소비교를 한다.
        - edubtm_InsertInternal
            - edubtm_CompactInternalPage
            - edubtm_SplitInternal
        - edubtm_InsertLeaf
            - edubtm_BinarySearchLeaf
            - edubtm_CompactLeafPage
            - edubtm_SplitLeaf

- EduBtM_DeleteObject

    edubtm_Delete()를 호출하여 삭제할 object에 대한 object의key, object ID> pair를 B+ tree 색인에서 삭제함
    - edubtm_Delete
        - 파라미터로 주어진 root page가 internal page인 경우, 삭제할<object의 key, object ID> pair가 저장된 leaf page를 찾기 위해 다음으로 방문할 자식 page를 결정함. 결정된자식page를root page로 하는 B+ subtree에서 <object의 key, object ID> pair를 삭제하기 위해 재귀적으로edubtm_Delete()를 호출함.
        - Underflow 발생시 Underflow가 발생한 자식 page의 부모 page (파라미터로 주어진 root page) 에서 overflow가발생한경우,edubtm_InsertInternal()을 호출하여 overflow로 인해 삽입되지 못한 internal index entry를 부모 page에 삽입함. edubtm_InsertInternal() 호출 결과로서 부모 page가 split 되므로, out parameter인 h를 TRUE로 설정함.
        - 파라미터로 주어진 root page가 root page인 경우 edubtm_DeleteLeaf를 호출하면 된다. dirty 처리와 비트 세팅 전부 함.
            - edubtm_DeleteLeaf
                - 삭제할<object의 key, object ID> pair가 저장된 index entry의 offset이 저장된 slot을 삭제하고 헤더를 갱신함.
                - Slot array 중간에 삭제된 빈 slot이 없도록 slot array를 compact 함.
                - Leaf page에서 underflow가 발생한 경우 (page의 data 영역중자유영역의크기> (page의data 영역의전체크기/ 2)), out parameter인 f를 TRUE로 설정함
                    - BL_FREE(apage) >= BL_HALF
    
    Root page에서 underflow가 발생한 경우, btm_root_delete()를 호출하여 이를처리함.
    
    Root page에서 split이 발생한 경우, edubtm_root_insert()를 호출하여이를처리함.
    - edubtm_root_insert
        
        새로운page를 할당받고, 기존 root page를 할당 받은 page로 복사함. 이후 기존 root page를 새로운 root page로서 초기화함(edubtm_InitInternal).
        - edubtm_InitInternal: Internal 페이지로 초기화, 헤더 설정함.

        할당받은page와 root page split으로 생성된 page가 새로운 root page의 자식 page들이 되도록 설정함.
        - Split으로 생성된 page를 가리키는 internal index entry를 새로운 root page에 삽입함
        - 새로운root page의 header의 p0 변수에 할당 받은 page의 번호를 저장함
        - 새로운root page의 두 자식 page들이leaf인 경우, 두 자식 page들간의doubly linked list를 설정함. Split으로 생성된 page가 할당 받은 page의 다음 page가되도록설정함

- EduBtM_Fetch
    - 파라미터로주어진startCompOp가 SM_BOF일 경우, B+ tree 색인의 첫 번째object (가장 작은 key 값을 갖는 leaf index entry) 를 검색함.
    - 파라미터로주어진startCompOp가 SM_EOF일 경우, B+ tree 색인의 마지막 object (가장 큰 key 값을 갖는 leaf index entry) 를검색함.  
    - 이외의경우, edubtm_Fetch()를 호출하여 B+ tree 색인에서 검색 조건을 만족하는 첫번째 <object의 key, object ID> pair가 저장된 leaf index entry를 검색함

- EduBtM_FetchNext
    - B+ tree 색인에서 검색 조건을만족하는현재 object의 다음 object를 검색하고, 검색된 object 를가리키는cursor를 반환함
    - edubtm_FetchNext()를 호출하여 B+ tree 색인에서 검색조건을 만족하는현재leaf index entry의 다음 leaf index entry를 검색함
    - 검색된leaf index entry를 가리키는 cursor를 반환함
    - edubtm_FetchNext
        - B+ tree 색인에서 검색 조건을만족하는현재leaf index entry의 다음leaf index entry를 검색하고, 검색된 leaf index entry를 가 리키는cursor를 반환함. 
        - 검색 조건이 SM_GT, SM_GE, SM_BOF 일경우key 값이작아지는방향으로backward scan을 하며
        - 그 외의경우key 값이커지는방향으로forward scan을 한다.