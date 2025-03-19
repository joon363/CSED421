# EduOM

This is a project 3 of CSED421

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

## Run mode

```
# Run my code with press enter
./EduOM_Test 
# Run my code without press enter
./EduOM_Test a 
```

## Testing

Testing will generate `result.txt` and compare with `test/solution.txt`

```
cd test
bash autograding.sh
```

## Report

Write into [REPORT.md](REPORT.md)
