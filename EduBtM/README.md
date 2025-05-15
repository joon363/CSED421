# EduBtM

This is a project 4 of CSED421

## Authentification setting

In auth.yaml

```
student_id: <your student id>
password: <your registration password>
```


To check correctness

```
cd auth_test
bash auth_test.sh
```

## Testing

### Downloading solution.txt

Due to restriction on a file upload size, solution.txt file is not uploaded.

Before running the autograder, please download [solution.txt](https://drive.google.com/file/d/1HCTrobKdYA8w86gcSBUAVO7kT13rox9_/view?usp=sharing)

### Description

![Test](test_structure.png)

Autograder runs 1) correctness test and 2) performance test.

Correctness Test

- Outputs error
- Workloads : 15

Performance Test

- Outputs runtime
- Workloads : 15

```
cd test
bash autograding.sh
```

Actual test is done by comparison between test/solution.txt and log.txt.

For help your debugging, the autograder gives detailed failure analysis for each tests.

### Workload API

- INSERT {key}: insert object with given key

- DELETE {key}: delete object with given key

- SCAN {start operation} {key} {end operation} {key}: fetch & fetch next

### How To Dump a Page

To show detailed description about page, please use dump page API like below.


```
printf("****************************** Dump  ******************************\n");
printf("pageNo of rootPid : %d\n", rootPid.pageNo);
dumpPage.volNo = volId;
printf("Enter the pageNo : ");
if (scanf("%d", &(dumpPage.pageNo)) == 0)
{
  while(getchar() != '\n');
  printf("Wrong number!!!\n");
  break;
}

e = dumpBtreePage(&dumpPage, kdesc);
if (e == eBADBTREEPAGE_BTM) printf("The page (PID: ( %d, %d )) does not exist in the B+ tree index.\n", dumpPage.volNo, dumpPage. pageNo);
        else if (e < eNOERROR) ERR(e);
```

### Scan Rule

There are four forms of scans

- SCAN startOp key endOp key
- SCAN startOp key EOF/BOF
- SCAN EOF/BOF endOp key
- SCAN EOF/BOF EOF/BOF

EOF means end of file (the largest object in the file) while BOF means begin of file (the smallest object in the file).

Direction of a scan is determined by second operation(a.k.a end operation)

```
If end operation is one of GT, GE or BOF, then do backward scan

Else do forward scan.
```

Function calls for scanning are fetch and fetchNext.

If we call scan, BtM first calls fetch and retrieves a cursor. Using the cursor BtM calls fetchNext to scan through.

Then how to find appropriate key for fetch? well, fetching condition is quiet ridiculous.  

It is clear to find the nearest key, meeting the starting operation. Then what about end condition? 

```
If end operation is one of LT, LE, GT, GE, EQ, we need to check against the end condition

Else we don't need to check it.
```

#### Examples

Assume we have datas **11, 12, ..., 100**.

For example, consider **SCAN GE 10 LE 100**

This scan starts with 11 which is greater than 10 and **less than** 100. Note that end operation is LE, thus we have to check whether selected value satisfies end condition.

Since the end operation is LE, we do forward scan; => 11, 12, ...., 100

In other words, fetch() returns 11, and fetchNext() returns 12, 13, ..., 100.

Consider another example **SCAN BOF EOF**

This scan starts with the smallest object and ends with the greatest value.

In this case, fetch returns 11, which is the smallest. Note that end operation is EOF, thus we don't need to check against it.

Since the end operation is EOF, we do forward scan. Therefore fetchNext() returns 12, 13, ... 100.

## Report

Write into [REPORT.md](REPORT.md)
