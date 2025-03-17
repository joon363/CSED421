#!/bin/bash

# Settings
path=""
output_file="result.txt"

if [ $# -eq 1 ]; then
    path=$1
else
    path="../"
fi

target_output_path="${path}${output_file}"

echo -e "\n=====================BUILD======================\n"

(cd ${path}; make clean; make)
if [ $? -ne 0 ]; then
    echo -e "\n=====================FAIL======================\n"
    exit 0
fi

echo -e "\n====================BUILD DONE==================\n"


echo -e "\n======================RUN=======================\n"

(cd ${path}; ./EduBfM_Test a > ${output_file})
if [ $? -ne 0 ]; then
    echo -e "\n=====================FAIL======================\n"
    exit 0
fi

echo -e "\n=====================RUN DONE===================\n"

echo -e "\n===================AUTOGRADING==================\n"

python3 checker.py ${target_output_path}
score=$?

echo -e "\n====================TEST SCORE===================\n"

echo -e "SCORE  : ${score}/100 \n"

exit ${score}