#!/bin/sh
#\
LD_LIBRARY_PATH=../build_dir; export LD_LIBRARY_PATH
#\
exec /usr/bin/tclsh8.6 "$0" "$@"

load ../build_dir/tcllogbuf.so
source ../tcl/logbuf_tcl.tcl

set lb [logbuf_get 0x1234]
logbuf_int32 lb 1 5535

puts [logbuf_get_arg [dict get $lb buf] 16 1]
