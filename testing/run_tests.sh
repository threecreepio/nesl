#!/usr/bin/env bash
# Run the regression tests against the built nesl binary.
# Exits 0 if all tests pass, 1 if any test fails or crashes.

set -u

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# Locate binary: prefer an env var, else default to the configured Release build.
BIN="${NESL_BIN:-$REPO_ROOT/build/Release/nesl}"
if [[ ! -x "$BIN" ]]; then
    echo "ERROR: nesl binary not found at $BIN" >&2
    echo "Set NESL_BIN=... or build with: cmake --build $REPO_ROOT/build/Release" >&2
    exit 2
fi

ROM="example.nes"
if [[ ! -f "$SCRIPT_DIR/$ROM" ]]; then
    echo "ERROR: test ROM missing: $SCRIPT_DIR/$ROM" >&2
    exit 2
fi

cd "$SCRIPT_DIR"
cp "$ROM" "/tmp/nesl_test_$ROM"

PASS=0
FAIL=0
RESULTS=()

# Each test: name | expected exit code (0 = clean pass, 139 = SIGSEGV on unfixed)
TESTS=(
    "test_emu_print_nontype.lua"
    "test_savestate_rejects_userdata.lua"
    "test_screenshot_no_extension.lua"
    "test_screenshot_long_path.lua"
    "test_rom_load_oob_boundary.lua"
    "test_loadines_rejects_zero_prg.lua"
    "test_rom_filename_and_input_get.lua"
)

for t in "${TESTS[@]}"; do
    out=$(timeout 10 "$BIN" "$t" 2>&1)
    code=$?
    if [[ $code -eq 0 ]]; then
        echo "PASS  $t"
        PASS=$((PASS+1))
        RESULTS+=("PASS $t")
    else
        echo "FAIL  $t (exit=$code)"
        echo "  --- output:"
        echo "$out" | sed 's/^/  /'
        FAIL=$((FAIL+1))
        RESULTS+=("FAIL $t exit=$code")
    fi
    rm -f /tmp/nesl_test_$ROM tenmb.bin noext_*
done

echo
echo "=============================="
echo "Tests: $((PASS+FAIL)), Passed: $PASS, Failed: $FAIL"
echo "=============================="
for r in "${RESULTS[@]}"; do
    echo "  $r"
done

[[ $FAIL -eq 0 ]] && exit 0 || exit 1
