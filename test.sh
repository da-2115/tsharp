#!/bin/bash

# Test Suite for T# Programming Language
# Dylan Armstrong, 2026

# Color codes for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Verbose flag
VERBOSE=${1:-}

# Counters
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0
declare -a PASSED_TESTS
declare -a FAILED_TESTS

# Build the T# compiler
echo -e "${BLUE}Building T# Compiler${NC}"
cmake . > /dev/null 2>&1 || {
    echo -e "${RED}CMake configuration failed${NC}"
    exit 1
}

make > /dev/null 2>&1 || {
    echo -e "${RED}Build failed${NC}"
    exit 1
}

if [ ! -f "./tsharp" ]; then
    echo -e "${RED}Compiler binary not found${NC}"
    exit 1
fi

echo -e "${GREEN}Build successful${NC}\n"

# Helper function to run a test
run_test() {
    local test_name=$1
    local file_path=$2
    local expected_output=$3
    
    TESTS_RUN=$((TESTS_RUN + 1))
    
    # Run the T# program and capture output
    local actual_output
    actual_output=$(./tsharp "$file_path" 2>&1)
    local exit_code=$?
    
    # Compare output
    if [ "$actual_output" = "$expected_output" ] && [ $exit_code -eq 0 ]; then
        echo -e "${GREEN}✓ PASS${NC}: $test_name"
        TESTS_PASSED=$((TESTS_PASSED + 1))
        PASSED_TESTS+=("$test_name")
    else
        echo -e "${RED}✗ FAIL${NC}: $test_name"
        FAILED_TESTS+=("$test_name")
        if [ "$VERBOSE" = "-v" ] || [ "$VERBOSE" = "--verbose" ]; then
            echo -e "  Expected: ${YELLOW}${expected_output}${NC}"
            echo -e "  Got:      ${YELLOW}${actual_output}${NC}"
        fi
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi
}

# Create temporary test directory
TEST_DIR=$(mktemp -d)
trap "rm -rf $TEST_DIR" EXIT

echo -e "${BLUE}Running T# Feature Tests${NC}\n"

# Test 1: Hello World
cat > "$TEST_DIR/test_hello.tsharp" << 'EOF'
void main() {
    println("Hello World!")
}
EOF
run_test "Hello World" "$TEST_DIR/test_hello.tsharp" "Hello World!"

# Test 2: Basic arithmetic
cat > "$TEST_DIR/test_arithmetic.tsharp" << 'EOF'
void main() {
    int a = 5
    int b = 3
    println(a + b)
}
EOF
run_test "Basic Arithmetic (5+3)" "$TEST_DIR/test_arithmetic.tsharp" "8"

# Test 3: String concatenation
cat > "$TEST_DIR/test_strings.tsharp" << 'EOF'
void main() {
    string name = "T#"
    println(name)
}
EOF
run_test "String Declaration" "$TEST_DIR/test_strings.tsharp" "T#"

# Test 4: If/Else statements
cat > "$TEST_DIR/test_if.tsharp" << 'EOF'
void main() {
    int x = 10
    if (x > 5) {
        println("x is greater than 5")
    } else {
        println("x is not greater than 5")
    }
}
EOF
run_test "If/Else Statement" "$TEST_DIR/test_if.tsharp" "x is greater than 5"

# Test 5: For loop
cat > "$TEST_DIR/test_for.tsharp" << 'EOF'
void main() {
    for (int i = 1; i <= 3; i++) {
        println(i)
    }
}
EOF
run_test "For Loop (1 to 3)" "$TEST_DIR/test_for.tsharp" $'1\n2\n3'

# Test 6: While loop
cat > "$TEST_DIR/test_while.tsharp" << 'EOF'
void main() {
    int i = 0
    while (i < 3) {
        println(i)
        i++
    }
}
EOF
run_test "While Loop" "$TEST_DIR/test_while.tsharp" $'0\n1\n2'

# Test 7: Function with return value
cat > "$TEST_DIR/test_function.tsharp" << 'EOF'
int add(int a, int b) {
    return a + b
}

void main() {
    println(add(3, 4))
}
EOF
run_test "Function with Return Value" "$TEST_DIR/test_function.tsharp" "7"

# Test 8: Factorial
cat > "$TEST_DIR/test_factorial.tsharp" << 'EOF'
void main() {
    println(factorial(5))
}
EOF
run_test "Factorial(5)" "$TEST_DIR/test_factorial.tsharp" "120"

# Test 9: Math functions
cat > "$TEST_DIR/test_math.tsharp" << 'EOF'
void main() {
    println(abs(-42))
}
EOF
run_test "Math - Absolute Value" "$TEST_DIR/test_math.tsharp" "42"

# Test 10: Power function
cat > "$TEST_DIR/test_pow.tsharp" << 'EOF'
void main() {
    println(pow(2, 3))
}
EOF
run_test "Math - Power (2^3)" "$TEST_DIR/test_pow.tsharp" "8"

# Test 11: Multiple statements
cat > "$TEST_DIR/test_multiple.tsharp" << 'EOF'
void main() {
    println("Line 1")
    println("Line 2")
    println("Line 3")
}
EOF
run_test "Multiple Statements" "$TEST_DIR/test_multiple.tsharp" $'Line 1\nLine 2\nLine 3'

# Test 12: Nested loops
cat > "$TEST_DIR/test_nested.tsharp" << 'EOF'
void main() {
    for (int i = 1; i <= 2; i++) {
        for (int j = 1; j <= 2; j++) {
            println(i)
        }
    }
}
EOF
run_test "Nested Loops" "$TEST_DIR/test_nested.tsharp" $'1\n1\n2\n2'

# Test 13: Variable assignment and increment
cat > "$TEST_DIR/test_increment.tsharp" << 'EOF'
void main() {
    int x = 5
    x++
    println(x)
}
EOF
run_test "Increment Operator" "$TEST_DIR/test_increment.tsharp" "6"

# Test 14: Boolean values
cat > "$TEST_DIR/test_bool.tsharp" << 'EOF'
void main() {
    bool flag = true
    if (flag) {
        println("true")
    }
}
EOF
run_test "Boolean Values" "$TEST_DIR/test_bool.tsharp" "true"

# Test 15: Comparison operators
cat > "$TEST_DIR/test_comparison.tsharp" << 'EOF'
void main() {
    if (5 < 10 && 10 > 3) {
        println("Both conditions true")
    }
}
EOF
run_test "Comparison Operators with AND" "$TEST_DIR/test_comparison.tsharp" "Both conditions true"

# Test 16: Class and object instantiation
cat > "$TEST_DIR/test_class.tsharp" << 'EOF'
class Point {
    public int x
    public int y

    public (int a, int b) {
        this.x = a
        this.y = b
    }

    public string Display() {
        return "(" + this.x + "," + this.y + ")"
    }
}

void main() {
    Point p(3, 4)
    println(p.Display())
}
EOF
run_test "Class Instantiation" "$TEST_DIR/test_class.tsharp" "(3,4)"

# Test 17: Do-while loop
cat > "$TEST_DIR/test_do_while.tsharp" << 'EOF'
void main() {
    int i = 0
    do {
        println(i)
        i++
    } while (i < 3)
}
EOF
run_test "Do-While Loop" "$TEST_DIR/test_do_while.tsharp" $'0\n1\n2'

# Test 18: Switch statement
cat > "$TEST_DIR/test_switch.tsharp" << 'EOF'
void main() {
    int day = 2
    switch (day) {
        case 1:
            println("Monday")
            break
        case 2:
            println("Tuesday")
            break
        default:
            println("Other")
    }
}
EOF
run_test "Switch Statement" "$TEST_DIR/test_switch.tsharp" "Tuesday"

# Test 19: Array initialization and access
cat > "$TEST_DIR/test_array.tsharp" << 'EOF'
void main() {
    int arr[3]
    arr[0] = 5
    arr[1] = 10
    arr[2] = 15
    println(arr[0])
    println(arr[1])
    println(arr[2])
}
EOF
run_test "Array Initialization" "$TEST_DIR/test_array.tsharp" $'5\n10\n15'

# Test 20: String concatenation
cat > "$TEST_DIR/test_string_concat.tsharp" << 'EOF'
void main() {
    string s1 = "Hello"
    string s2 = "World"
    println(s1 + " " + s2)
}
EOF
run_test "String Concatenation" "$TEST_DIR/test_string_concat.tsharp" "Hello World"

# Edge Cases

# Edge Case 1: Zero values
cat > "$TEST_DIR/test_edge_zero.tsharp" << 'EOF'
void main() {
    int zero = 0
    println(zero)
    println(zero + 5)
    println(zero * 100)
}
EOF
run_test "Edge Case - Zero Values" "$TEST_DIR/test_edge_zero.tsharp" $'0\n5\n0'

# Edge Case 2: Negative numbers
cat > "$TEST_DIR/test_edge_negative.tsharp" << 'EOF'
void main() {
    int neg = -10
    println(neg)
    println(neg + 20)
    println(abs(neg))
}
EOF
run_test "Edge Case - Negative Numbers" "$TEST_DIR/test_edge_negative.tsharp" $'-10\n10\n10'

# Edge Case 3: Large numbers
cat > "$TEST_DIR/test_edge_large.tsharp" << 'EOF'
void main() {
    int large = 1000000
    println(large)
    println(large + 1)
}
EOF
run_test "Edge Case - Large Numbers" "$TEST_DIR/test_edge_large.tsharp" $'1000000\n1000001'

# Edge Case 4: Division by checking modulo
cat > "$TEST_DIR/test_edge_modulo.tsharp" << 'EOF'
void main() {
    println(10 % 3)
    println(20 % 5)
    println(7 % 2)
}
EOF
run_test "Edge Case - Modulo Operations" "$TEST_DIR/test_edge_modulo.tsharp" $'1\n0\n1'

# Edge Case 5: Empty loop
cat > "$TEST_DIR/test_edge_empty_loop.tsharp" << 'EOF'
void main() {
    for (int i = 0; i < 0; i++) {
        println("Never runs")
    }
    println("After loop")
}
EOF
run_test "Edge Case - Empty Loop" "$TEST_DIR/test_edge_empty_loop.tsharp" "After loop"

# Edge Case 6: Nested conditionals
cat > "$TEST_DIR/test_edge_nested_if.tsharp" << 'EOF'
void main() {
    int x = 5
    if (x > 0) {
        if (x < 10) {
            if (x == 5) {
                println("Five")
            }
        }
    }
}
EOF
run_test "Edge Case - Nested Conditionals" "$TEST_DIR/test_edge_nested_if.tsharp" "Five"

# Edge Case 7: Multiple logical operators
cat > "$TEST_DIR/test_edge_complex_logic.tsharp" << 'EOF'
void main() {
    if ((5 > 3) && (10 < 20) && (2 == 2)) {
        println("All true")
    }
}
EOF
run_test "Edge Case - Complex Logic" "$TEST_DIR/test_edge_complex_logic.tsharp" "All true"

# Edge Case 8: Empty string operations
cat > "$TEST_DIR/test_edge_string_ops.tsharp" << 'EOF'
void main() {
    string empty = ""
    string text = "T#"
    println(empty + text)
    println(text)
}
EOF
run_test "Edge Case - String Operations" "$TEST_DIR/test_edge_string_ops.tsharp" $'T#\nT#'

# Edge Case 9: Factorial of 0 and 1
cat > "$TEST_DIR/test_edge_factorial.tsharp" << 'EOF'
void main() {
    println(factorial(0))
    println(factorial(1))
    println(factorial(3))
}
EOF
run_test "Edge Case - Factorial Edge Cases" "$TEST_DIR/test_edge_factorial.tsharp" $'1\n1\n6'

# Edge Case 10: Math functions with edge values
cat > "$TEST_DIR/test_edge_math.tsharp" << 'EOF'
void main() {
    println(sqrt(0))
    println(sqrt(1))
    println(abs(0))
    println(min(5, 5))
    println(max(5, 5))
}
EOF
run_test "Edge Case - Math Functions" "$TEST_DIR/test_edge_math.tsharp" $'0\n1\n0\n5\n5'

# Test with existing examples
echo -e "\nRunning example tests...\n"

if [ -f "examples/test.tsharp" ]; then
    run_test "Example - Hello World" "examples/test.tsharp" "Hello World!"
fi

if [ -f "examples/dylan.tsharp" ]; then
    expected=$'1\n4\n729'
    run_test "Example - Dylan Function" "examples/dylan.tsharp" "$expected"
fi

# Test demo.tsharp by checking it runs and contains expected output
if [ -f "examples/demo.tsharp" ]; then
    demo_output=$(./tsharp "examples/demo.tsharp" 2>&1)
    demo_pass=true
    missing_checks=""

    echo -e "\nDemo output:\n"
    echo -e "${YELLOW}$demo_output${NC}"
    echo ""
    
    # Check for key output markers and math function results
    if ! echo "$demo_output" | grep -q "T# Feature Demo"; then
        demo_pass=false
        missing_checks="$missing_checks\n  - Missing: 'T# Feature Demo'"
    fi
    if ! echo "$demo_output" | grep -q "Data Types"; then
        demo_pass=false
        missing_checks="$missing_checks\n  - Missing: 'Data Types'"
    fi
    if ! echo "$demo_output" | grep -q "Math functions"; then
        demo_pass=false
        missing_checks="$missing_checks\n  - Missing: 'Math functions'"
    fi
    if ! echo "$demo_output" | grep -q "sqrt(16): 4"; then
        demo_pass=false
        missing_checks="$missing_checks\n  - Missing: 'sqrt(16): 4'"
    fi
    if ! echo "$demo_output" | grep -q "abs(-5): 5"; then
        demo_pass=false
        missing_checks="$missing_checks\n  - Missing: 'abs(-5): 5'"
    fi
    if ! echo "$demo_output" | grep -q "factorial(5): 120"; then
        demo_pass=false
        missing_checks="$missing_checks\n  - Missing: 'factorial(5): 120'"
    fi
    if ! echo "$demo_output" | grep -q "pow(2, 8): 256"; then
        demo_pass=false
        missing_checks="$missing_checks\n  - Missing: 'pow(2, 8): 256'"
    fi
    if ! echo "$demo_output" | grep -q "Classes & Inheritance"; then
        demo_pass=false
        missing_checks="$missing_checks\n  - Missing: 'Classes & Inheritance'"
    fi
    if ! echo "$demo_output" | grep -q "Arrays & Generics"; then
        demo_pass=false
        missing_checks="$missing_checks\n  - Missing: 'Arrays & Generics'"
    fi
    if ! echo "$demo_output" | grep -q "Done."; then
        demo_pass=false
        missing_checks="$missing_checks\n  - Missing: 'Done.'"
    fi
    
    if [ "$demo_pass" = true ]; then
        echo -e "${GREEN}✓ PASS${NC}: Example - Comprehensive Demo"
        TESTS_PASSED=$((TESTS_PASSED + 1))
        PASSED_TESTS+=("Example - Comprehensive Demo")
    else
        echo -e "${RED}✗ FAIL${NC}: Example - Comprehensive Demo"
        echo -e "  Checks:$missing_checks"
        FAILED_TESTS+=("Example - Comprehensive Demo")
        TESTS_FAILED=$((TESTS_FAILED + 1))
    fi
    TESTS_RUN=$((TESTS_RUN + 1))
fi

# Print summary
echo -e "\nTest Summary"
echo -e "Total Tests:  $TESTS_RUN"
echo -e "${GREEN}Passed:       $TESTS_PASSED${NC}"
if [ $TESTS_FAILED -gt 0 ]; then
    echo -e "${RED}Failed:       $TESTS_FAILED${NC}"
else
    echo -e "${GREEN}Failed:       $TESTS_FAILED${NC}"
fi

# Calculate percentage
if [ $TESTS_RUN -gt 0 ]; then
    PASS_RATE=$((TESTS_PASSED * 100 / TESTS_RUN))
    echo -e "Pass Rate:    ${YELLOW}${PASS_RATE}%${NC}"
fi

# Display test lists if verbose or if there are failures
if [ "$VERBOSE" = "-v" ] || [ "$VERBOSE" = "--verbose" ] || [ $TESTS_FAILED -gt 0 ]; then
    echo -e "\nPassed Tests (${#PASSED_TESTS[@]})"
    for test in "${PASSED_TESTS[@]}"; do
        echo -e "${GREEN}✓${NC} $test"
    done
    
    if [ $TESTS_FAILED -gt 0 ]; then
        echo -e "\nFailed Tests (${#FAILED_TESTS[@]})"
        for test in "${FAILED_TESTS[@]}"; do
            echo -e "${RED}✗${NC} $test"
        done
    fi
fi

# Exit with appropriate code
if [ $TESTS_FAILED -gt 0 ]; then
    echo -e "\n${RED}Some tests failed!${NC}"
    echo -e "Run with '${YELLOW}./test.sh -v${NC}' for verbose output"
    exit 1
else
    echo -e "\n${GREEN}All tests passed! ✓${NC}"
    exit 0
fi
