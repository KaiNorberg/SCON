#!/bin/bash

set -e

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
NC='\033[0m'

SOURCE_DIR="tools/reduct-cli"

CMAKE_GEN_OPTS=""
if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" ]]; then
    CMAKE_GEN_OPTS="-G Ninja"
fi

FUZZ_TIME="${FUZZ_TIME:-60}"

CRASH_DIR="tests/fuzz_crashes"
CORPUS_DIR="tests/fuzz_corpus"

echo -e "Step 1: Compiling with Sanitizers..."

cmake $CMAKE_GEN_OPTS -S "$SOURCE_DIR" -B tests/build_san \
    -DCMAKE_BUILD_TYPE=Debug \
    -DREDUCT_USE_SANITIZERS=ON

cmake --build tests/build_san
REDUCT_BIN="tests/build_san/reduct"

if [ -f "${REDUCT_BIN}.exe" ]; then
    REDUCT_BIN="${REDUCT_BIN}.exe"
fi

echo -e "${GREEN}Build successful: $REDUCT_BIN created.${NC}"
echo "----------------------------------------------------"
echo "Step 2: Running tests from /tests directory..."

TEST_COUNT=0
FAIL_COUNT=0

for test_file in $(find tests/ -maxdepth 1 -name "*.rdt"); do
    TEST_COUNT=$((TEST_COUNT + 1))
    echo "Testing $test_file ... "

    if ASAN_OPTIONS=halt_on_error=1 "$REDUCT_BIN" "$test_file"; then
        echo ""
        echo -e "${GREEN}PASS${NC}"
    else
        echo ""
        echo -e "${RED}FAIL${NC}"
        FAIL_COUNT=$((FAIL_COUNT + 1))
    fi
done

if [ $FAIL_COUNT -eq 0 ]; then
    echo -e "${GREEN}All $TEST_COUNT regression tests passed.${NC}"
else
    echo -e "${RED}$FAIL_COUNT/$TEST_COUNT regression tests failed.${NC}"
fi

echo "----------------------------------------------------"
echo "Step 3: Building fuzzer target..."

if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" || "$OSTYPE" == "darwin"* ]]; then
    echo -e "${YELLOW}SKIP: Fuzzing is bypassed on Windows and macOS.${NC}"
    [ $FAIL_COUNT -eq 0 ] && exit 0 || exit 1
fi

if ! command -v clang &> /dev/null; then
    echo -e "${YELLOW}SKIP: clang not found. Install clang to enable fuzzing.${NC}"
    [ $FAIL_COUNT -eq 0 ] && exit 0 || exit 1
fi

cmake $CMAKE_GEN_OPTS -S "$SOURCE_DIR" -B tests/build_fuzz \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_C_COMPILER=clang \
    -DREDUCT_USE_FUZZER=ON

cmake --build tests/build_fuzz
FUZZ_BIN="tests/build_fuzz/fuzz"

echo -e "${GREEN}Build successful: fuzz_reduct_parse created.${NC}"
echo "----------------------------------------------------"
echo "Step 4: Seeding fuzzer corpus from .rdt files..."

rm -rf "$CORPUS_DIR" "$CRASH_DIR"
mkdir -p "$CORPUS_DIR" "$CRASH_DIR"

RDT_COUNT=0
for rdt_file in $(find tests/ bench/ -name "*.rdt" 2>/dev/null); do
    cp -f "$rdt_file" "$CORPUS_DIR/"
    RDT_COUNT=$((RDT_COUNT + 1))
done
echo -e "${GREEN}Seeded corpus with $RDT_COUNT .rdt file(s).${NC}"

echo "----------------------------------------------------"
echo "Step 5: Running fuzzer for ${FUZZ_TIME}s..."
echo ""

FUZZ_EXIT=0

set +e
"$FUZZ_BIN" "$CORPUS_DIR"    \
    -artifact_prefix="$CRASH_DIR/" \
    -max_total_time="$FUZZ_TIME"   \
    -timeout=10                    \
    2>&1
FUZZ_EXIT=$?
set -e

echo ""
echo "===================================================="

if [ $FAIL_COUNT -eq 0 ]; then
    echo -e "  Regression tests : ${GREEN}PASSED ($TEST_COUNT/$TEST_COUNT)${NC}"
else
    echo -e "  Regression tests : ${RED}FAILED ($((TEST_COUNT - FAIL_COUNT))/$TEST_COUNT passed)${NC}"
fi

CRASH_FILES=("$CRASH_DIR"/crash-* "$CRASH_DIR"/timeout-* "$CRASH_DIR"/leak-*)
CRASH_COUNT=0
for f in "${CRASH_FILES[@]}"; do
    [ -f "$f" ] && CRASH_COUNT=$((CRASH_COUNT + 1))
done

if [ $CRASH_COUNT -eq 0 ]; then
    echo -e "  Fuzzer (${FUZZ_TIME}s)     : ${GREEN}NO CRASHES${NC}"
else
    echo -e "  Fuzzer (${FUZZ_TIME}s)     : ${RED}$CRASH_COUNT CRASH(ES) FOUND${NC}"
fi

echo "===================================================="

[ $FAIL_COUNT -eq 0 ] && [ $CRASH_COUNT -eq 0 ] && exit 0 || exit 1