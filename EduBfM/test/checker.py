import sys

# global var
skip_all_test = False

# Functions
def skipAllIfNull(line):
    if not line:
        global skip_all_test
        skip_all_test = True
        return True

    return False

## File should not EOF
def readUntilContains(file, substring):
    line = file.readline()

    if skipAllIfNull(line):
        return

    while substring not in line:
        line = file.readline()
        if skipAllIfNull(line):
            break

def checkUntilNewTest(sol_file, out_file):
    next_test_substr = "*Test"

    sol_line = sol_file.readline()
    out_line = out_file.readline()

    while True:
        # Check EOF
        if not sol_line or not out_line:
            global skip_all_test
            skip_all_test = True
            if not sol_line and not out_line:
                return True
            else:
                return False

        # Check Not Same
        if sol_line != out_line:
            # Move to next test
            if next_test_substr not in sol_line:
                readUntilContains(sol_file, next_test_substr)
            if next_test_substr not in out_line:
                readUntilContains(out_file, next_test_substr)
            return False
        else:
            if next_test_substr in sol_line:
                return True
            else:
                sol_line = sol_file.readline()
                out_line = out_file.readline()

    return False

# Settings
output_file_path = sys.argv[1]

solution_file = open("solution.txt", 'r')
output_file = open(output_file_path, 'r')

score = 0

# Actual run
readUntilContains(solution_file, "*Test 1_1")
readUntilContains(output_file, "*Test 1_1")

test_list = ["1_1", "1_2", "1_3", "1_4", "1_5", "1_6", "2_1", "3_1", "3_2", "3_3"]
score_list = [5, 5, 5, 5, 10, 10, 20, 15, 15, 10] 

for test_num in range(len(test_list)):
    test = test_list[test_num]
    test_score = score_list[test_num]

    print("\n=====================TEST " + test + "===================\n")
    if skip_all_test:
        print("SKIP")
    else:
        if checkUntilNewTest(solution_file, output_file):
            print("PASS")
            score += test_score
        else:
            print("FAIL")

solution_file.close()
output_file.close()

sys.exit(score)
