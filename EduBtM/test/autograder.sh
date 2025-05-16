#!/bin/bash

# Settings
path=""

if [ $# -eq 1 ]; then
    path=$1
else
    path="../"
fi

target_output_path="${path}log.txt"
echo "{\"Test\": 0 }" > result.json

echo -e "\n=====================BUILD======================\n"

(cd ${path}; make clean; make)
if [ $? -ne 0 ]; then
    echo -e "\n=====================FAIL======================\n"
    (cd ${path}; make clean > /dev/null)
    exit 0
fi

echo -e "\n====================BUILD DONE==================\n"


echo -e "\n======================RUN=======================\n"

(cd ${path}; ./EduBtM_Test)
if [ $? -ne 0 ]; then
    (cd ${path}; make clean > /dev/null)
    echo -e "\n=====================FAIL======================\n"
    exit 0
fi

python3 checker.py ${target_output_path}
score=$?
if [ $score -eq 1 ]; then
    (cd ${path}; make clean > /dev/null)
    echo -e "\n=====================FAIL======================\n"
    exit 0
fi

echo -e "\n====================TEST SCORE===================\n"

echo -e "SCORE  : ${score}/100 \n"

echo "{\"Test\": ${score} }" > result.json

(cd ${path}; make clean > /dev/null)