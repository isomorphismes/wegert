#!/usr/bin/env bash
set -euo pipefail
export LC_ALL=C
# Inspect the ZIP envelope, not apksigner's exit status. A broken signature
# also fails verification and must never be mistaken for an unsigned APK.
# This source-build lane produces ordinary single-disk ZIPs without comments.
# ZIP64, comments, trailing data and ambiguous envelopes fail closed.
[[ $# -eq 1 ]] || { echo 'usage: verify-unsigned.sh APK' >&2; exit 2; }
apk=$1
[[ -f $apk ]] || { echo "APK missing: $apk" >&2; exit 1; }
fail() { printf 'Unsigned APK rejected: %s\n' "$*" >&2; exit 1; }
size=$(stat -c '%s' -- "$apk")
(( size >= 22 && size < 4294967295 )) || fail 'unsupported ZIP size'
eocd=$((size - 22))
bytes=$(od -An -v -tu1 -j "$eocd" -N 22 -- "$apk")
read -r -a footer <<< "${bytes//$'\n'/ }"
[[ ${#footer[@]} -eq 22 ]] || fail 'truncated ZIP footer'
[[ ${footer[*]:0:4} == '80 75 5 6' ]] || fail 'missing comment-free ZIP footer'
[[ ${footer[*]:4:4} == '0 0 0 0' && ${footer[*]:20:2} == '0 0' ]] || fail 'multidisk ZIP or comment'
count=$((footer[10] + 256 * footer[11]))
(( count > 0 && count < 65535 && count == footer[8] + 256 * footer[9] )) || fail 'invalid ZIP entry count'
directory_size=$((footer[12] + (footer[13] << 8) + (footer[14] << 16) + (footer[15] << 24)))
directory_offset=$((footer[16] + (footer[17] << 8) + (footer[18] << 16) + (footer[19] << 24)))
(( directory_offset >= 16 && directory_size > 0 && directory_offset + directory_size == eocd )) || fail 'invalid central directory bounds'
# AOSP specifies this 16-byte magic immediately before the central directory
# for APK signing blocks (v2 and newer), even when their signatures are bad.
magic=$(od -An -v -tx1 -j "$((directory_offset - 16))" -N 16 -- "$apk" | tr -d ' \n')
[[ $magic != 41504b2053696720426c6f636b203432 ]] || fail 'APK signing block present'
unzip -t "$apk" >/dev/null
entries=$(unzip -Z1 "$apk")
if grep -Eiq '^META-INF/([^/]+[.](RSA|DSA|EC|SF)|SIG-[^/]+)$' <<< "$entries"; then
    fail 'JAR signature records present'
fi
duplicates=$(printf '%s\n' "$entries" | sort | uniq -d)
[[ -z $duplicates ]] || fail 'duplicate ZIP entry names'
printf 'Unsigned APK envelope verified: %s\n' "$apk"
