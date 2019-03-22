#!/bin/sh
#\
LD_LIBRARY_PATH=../build_dir; export LD_LIBRARY_PATH
#\
exec /usr/bin/tclsh8.6 "$0" "$@"

load ../build_dir/tcllogbuf.so
source ../tcl/logbuf_tcl.tcl

set intval 5535
set lb [logbuf_get 0x1234]
logbuf_int32 lb 1 $intval

puts "test getarg int: [logbuf_get_arg [dict get $lb buf] 16 1]=$intval"

set lb [logbuf_get 0x7b]
logbuf_int32 lb 1 1234
logbuf_fmt lb "test fmt message4 (embedded fmt string): int value %d"

set r {
[LOG_MSG_1]
INDEX = 0x7b
FMTSTR = test fmt message1 (res string): int value %d
}

puts [logbuf_format_message -res $r [dict get $lb buf]]
puts [logbuf_format_message "test fmt message2 (cmdline fmt string): int value %d" [dict get $lb buf]]
puts [logbuf_format_message -res $r "test fmt message3 (both res & cmdline fmt string): int value %d" [dict get $lb buf]]
puts [logbuf_format_message [dict get $lb buf]]
