#!/system/bin/sh

set -e

test_dir=/data/nativetest/jemalloc_unittests

$test_dir/bitmap
$test_dir/ckh
$test_dir/hash
$test_dir/junk
$test_dir/mallctl
$test_dir/math
$test_dir/mq
$test_dir/mtx
$test_dir/ql
$test_dir/qr
$test_dir/quarantine
$test_dir/rb
$test_dir/rtree
$test_dir/SFMT
$test_dir/stats
$test_dir/tsd
$test_dir/util
$test_dir/zero
