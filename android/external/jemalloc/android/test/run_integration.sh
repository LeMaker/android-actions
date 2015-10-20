#!/system/bin/sh

set -e

test_dir=/data/nativetest/jemalloc_integrationtests

$test_dir/aligned_alloc
$test_dir/allocated
$test_dir/chunk
$test_dir/mallocx
$test_dir/MALLOCX_ARENA
$test_dir/posix_memalign
$test_dir/rallocx
$test_dir/thread_arena
$test_dir/thread_tcache_enabled
$test_dir/xallocx
