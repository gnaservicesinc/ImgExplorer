#!/bin/sh
set -eu

program=$1
fixtures=$2
transcript="$fixtures/menu-transcript.txt"
diff_file="$fixtures/menu-diff.txt"
image_file="$fixtures/menu-image.txt"

rm -f "$transcript" "$diff_file" "$image_file"

printf '1\n%s/rgb8_a.png\n2\n%s/rgb8_b.png\n3\n4\n%s\n5\n%s\n0\n' \
    "$fixtures" "$fixtures" "$diff_file" "$image_file" |
    "$program" >"$transcript"

grep -q 'Loaded image A: 2x2 RGB from 8-bit PNG' "$transcript"
grep -q 'Loaded image B: 2x2 RGB from 8-bit PNG' "$transcript"
grep -q 'Differing channel values: 1 / 12 (8.333333%)' "$transcript"
grep -q 'Pixels containing a difference: 1 / 4 (25.000000%)' "$transcript"
grep -q 'Wrote' "$transcript"

test "$(wc -l <"$diff_file" | tr -d ' ')" = 2
test "$(wc -l <"$image_file" | tr -d ' ')" = 2
grep -q "$(printf '\t')" "$diff_file"
grep -q ',' "$image_file"

echo "Interactive menu test passed."
