#!/bin/sh
set -eu

bin=${1:-./numcsv}
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

fail=0

check() {
    name=$1
    shift
    if ! "$bin" "$@" >"$tmp/actual" 2>"$tmp/stderr"; then
        printf 'FAIL %s: command failed\n' "$name"
        cat "$tmp/stderr"
        fail=1
        return
    fi
    if ! diff -u "$tmp/expected" "$tmp/actual"; then
        printf 'FAIL %s: unexpected output\n' "$name"
        fail=1
    fi
}

cat >"$tmp/basic.csv" <<'EOF'
value
1
2
3
4
EOF
cat >"$tmp/basic.expected" <<'EOF'
count: 4
min: 1
max: 4
mean: 2.5
median: 2.5
EOF
check basic -H -c 1 "$tmp/basic.csv"

cat >"$tmp/quoted.csv" <<'EOF'
name,amount
a,"10"
b,20
EOF
cat >"$tmp/quoted.expected" <<'EOF'
count: 2
min: 10
max: 20
mean: 15
median: 15
EOF
check quoted -H -c 2 "$tmp/quoted.csv"

cat >"$tmp/quotedcomma.csv" <<'EOF'
id,value
1,"a,b"
2,3
EOF
cat >"$tmp/quotedcomma.expected" <<'EOF'
count: 2
min: 1
max: 2
mean: 1.5
median: 1.5
EOF
check quotedcomma -H -c 1 "$tmp/quotedcomma.csv"

cat >"$tmp/multiline.csv" <<'EOF'
id,value
1,"hello
world"
2,3
EOF
cat >"$tmp/multiline.expected" <<'EOF'
count: 2
min: 1
max: 2
mean: 1.5
median: 1.5
EOF
check multiline -H -c 1 "$tmp/multiline.csv"

cat >"$tmp/invalid.csv" <<'EOF'
values
1
x

2.5
EOF
cat >"$tmp/invalid.expected" <<'EOF'
count: 2
min: 1
max: 2.5
mean: 1.75
median: 1.75
EOF
check invalid "$tmp/invalid.csv"

: >"$tmp/empty.csv"
if "$bin" "$tmp/empty.csv" >"$tmp/empty.out" 2>"$tmp/empty.err"; then
    printf 'FAIL empty: expected failure\n'
    fail=1
else
    :
fi

if [ "$fail" -ne 0 ]; then
    exit 1
fi
