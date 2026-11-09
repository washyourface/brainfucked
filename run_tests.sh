
#!/usr/bin/env bash

BIN="./bfckd"
TEST_DIR="./tests"

pass=0
fail=0

tmpdir=$(mktemp -d) || exit 1
trap 'rm -rf "$tmpdir"' EXIT

run_tests() {
    for src in "$TEST_DIR"/*.bf; do
        [ -f "$src" ] || continue

        expected="${src}.expected"

        if [ ! -f "$expected" ]; then
            echo "WITHOUT EXPECTED: $src"
            fail=$((fail + 1))
            continue
        fi

        # Extract expected exit code
        expected_exit=$(awk '
            /^\[exit\]$/ {
                getline
                print
                exit
            }
        ' "$expected")

        if [ -z "$expected_exit" ]; then
            echo "INVALID EXPECTED EXIT: $expected"
            fail=$((fail + 1))
            continue
        fi

        # Extract expected stdout
        awk '
            /^\[stdout\]$/ { capture=1; next }
            /^\[stderr\]$/ { capture=0 }
            capture { print }
        ' "$expected" > "$tmpdir/expected.stdout"

        # Extract expected stderr
        awk '
            /^\[stderr\]$/ { capture=1; next }
            capture { print }
        ' "$expected" > "$tmpdir/expected.stderr"

        # Remove structural final newline
        truncate -s -1 "$tmpdir/expected.stdout"
        truncate -s -1 "$tmpdir/expected.stderr"

        input="${src}.stdin"

        if [ ! -f "$input" ]; then
            input="/dev/null"
        fi

        # Execute program
        "$BIN" "$src" \
            < "$input" \
            > "$tmpdir/actual.stdout" \
            2> "$tmpdir/actual.stderr"
        actual_exit=$?

        test_passed=true

        # Compare exit code
        if [ "$actual_exit" != "$expected_exit" ]; then
            echo "  EXIT mismatch"
            echo "    Expected: $expected_exit"
            echo "    Actual:   $actual_exit"
            test_passed=false
        fi

        # Compare stdout
        if ! cmp -s "$tmpdir/expected.stdout" "$tmpdir/actual.stdout"; then
            echo "  STDOUT mismatch"
            echo "    Expected:"
            od -An -tx1c "$tmpdir/expected.stdout"
            echo "    Actual:"
            od -An -tx1c "$tmpdir/actual.stdout"
            test_passed=false
        fi

        # Compare stderr
        if ! cmp -s "$tmpdir/expected.stderr" "$tmpdir/actual.stderr"; then
            echo "  STDERR mismatch"
            echo "    Expected:"
            od -An -tx1c "$tmpdir/expected.stderr"
            echo "    Actual:"
            od -An -tx1c "$tmpdir/actual.stderr"
            test_passed=false
        fi

        if [ "$test_passed" = true ]; then
            echo "PASS: $src"
            pass=$((pass + 1))
        else
            echo "FAIL: $src"
            fail=$((fail + 1))
        fi
    done

    echo
    echo "Results: $pass passed, $fail failed"

    [ "$fail" -eq 0 ]
}

run_tests
