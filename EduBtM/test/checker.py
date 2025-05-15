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
    next_test_substr = "Setting"

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
readUntilContains(solution_file, "Setting")
readUntilContains(output_file, "Setting")

test_list = ["RAND A", "RAND_B", "RAND_C", "RAND_D", "RAND_E", "MII_A", "MII_B", "MII_C", "MII_D", "MII_E", "EMAIL_A", "EMAIL_B", "EMAIL_C", "EMAIL_D", "EMAIL_E"]

for test_num in range(len(test_list)):
    test = test_list[test_num]
    test_score = 6

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

sys.exit(score + 10)