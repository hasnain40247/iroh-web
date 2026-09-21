#!/usr/bin/env bash
# Test runner for cIroh.
# Usage: ./tests/run_tests.sh [suite]
# suite defaults to "len" — the len() native function fix.
#
# Test convention (per tests/<suite>/):
#   name.iroh          — program to run
#   name.expected      — expected stdout (output test, must exit 0)
#   name.expected_err  — expected stderr substring (error test, must exit 70)
#
# A test named 00_regression_existing uses code/test2.iroh as the source.

set -uo pipefail

SUITE="${1:-len}"
TESTS_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$TESTS_DIR/.." && pwd)"
SUITE_DIR="$TESTS_DIR/$SUITE"
BINARY="$ROOT/ciroh"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BOLD='\033[1m'
RESET='\033[0m'

pass=0
fail=0
errors=()

# ── build ──────────────────────────────────────────────────────────────────────
echo -e "${BOLD}Building cIroh...${RESET}"
cd "$ROOT"
if ! gcc -Wall -Wextra main.c frontend/*.c runtime/*.c debug/debug.c \
        -o ciroh -lm 2>&1; then
  echo -e "${RED}BUILD FAILED — cannot run tests.${RESET}"
  exit 1
fi
echo -e "${GREEN}Build OK${RESET}"
echo ""

# ── helpers ───────────────────────────────────────────────────────────────────
run_output_test() {
  local name="$1"
  local src="$2"
  local expected_file="$3"

  local actual actual_exit
  actual=$("$BINARY" "$src" 2>/dev/null) || true
  "$BINARY" "$src" > /dev/null 2>&1; actual_exit=$?

  local expected
  expected=$(cat "$expected_file")

  if [ "$actual" = "$expected" ] && [ "$actual_exit" = "0" ]; then
    echo -e "  ${GREEN}PASS${RESET}  $name"
    pass=$((pass + 1))
  else
    echo -e "  ${RED}FAIL${RESET}  $name"
    if [ "$actual_exit" != "0" ]; then
      echo "        exit code: $actual_exit (expected 0)"
    fi
    if [ "$actual" != "$expected" ]; then
      echo "        expected: $(echo "$expected" | tr '\n' '|')"
      echo "        got:      $(echo "$actual"   | tr '\n' '|')"
    fi
    fail=$((fail + 1))
    errors+=("$name")
  fi
}

run_error_test() {
  local name="$1"
  local src="$2"
  local expected_err_file="$3"

  local actual_stderr actual_exit
  actual_stderr=$("$BINARY" "$src" 2>&1 1>/dev/null) || true
  "$BINARY" "$src" > /dev/null 2>&1; actual_exit=$?

  local expected_msg
  expected_msg=$(cat "$expected_err_file")

  local ok=1
  [ "$actual_exit" = "70" ] || ok=0
  echo "$actual_stderr" | grep -qF "$expected_msg" || ok=0

  if [ "$ok" = "1" ]; then
    echo -e "  ${GREEN}PASS${RESET}  $name"
    pass=$((pass + 1))
  else
    echo -e "  ${RED}FAIL${RESET}  $name"
    if [ "$actual_exit" != "70" ]; then
      echo "        exit code: $actual_exit (expected 70)"
    fi
    if ! echo "$actual_stderr" | grep -qF "$expected_msg"; then
      echo "        expected stderr to contain: \"$expected_msg\""
      echo "        got stderr:                 \"$actual_stderr\""
    fi
    fail=$((fail + 1))
    errors+=("$name")
  fi
}

# ── run tests ─────────────────────────────────────────────────────────────────
echo -e "${BOLD}Running suite: $SUITE${RESET}"
echo ""

for iroh_file in "$SUITE_DIR"/*.iroh; do
  [ -f "$iroh_file" ] || continue
  base="${iroh_file%.iroh}"
  name="$(basename "$base")"

  if [ "$name" = "00_regression_existing" ]; then
    src="$ROOT/code/test2.iroh"
  else
    src="$iroh_file"
  fi

  if [ -f "$base.expected_err" ]; then
    run_error_test "$name" "$src" "$base.expected_err"
  elif [ -f "$base.expected" ]; then
    run_output_test "$name" "$src" "$base.expected"
  else
    echo -e "  ${YELLOW}SKIP${RESET}  $name (no .expected file)"
  fi
done

# ── summary ───────────────────────────────────────────────────────────────────
echo ""
echo -e "${BOLD}Results: ${GREEN}$pass passed${RESET}${BOLD}, ${RED}$fail failed${RESET}"

if [ "${#errors[@]}" -gt 0 ]; then
  echo ""
  echo "Failed tests:"
  for e in "${errors[@]}"; do
    echo "  - $e"
  done
  echo ""
  echo -e "${BOLD}Fix needed in runtime/vm.c — lenNative() must:${RESET}"
  echo "  1. Accept exactly 1 argument  → runtimeError(\"len() expects 1 argument.\")"
  echo "  2. ObjList  → return NUMBER_VAL(AS_LIST(args[0])->items.count)"
  echo "  3. ObjString → return NUMBER_VAL(AS_STRING(args[0])->length)"
  echo "  4. Any other type → runtimeError(\"len() argument must be a string or list.\")"
  exit 1
fi

exit 0
