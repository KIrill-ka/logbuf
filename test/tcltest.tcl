#!/bin/sh
#\
LD_LIBRARY_PATH=../build_dir; export LD_LIBRARY_PATH
#\
exec /usr/bin/tclsh8.6 "$0" "$@"

load ../build_dir/tcllogbuf.so
source ../tcl/logbuf_tcl.tcl
namespace import tcl::mathop::*

set intval 5535
set intval64 0x800000000
set lb [logbuf_get "" 321 1234]
logbuf_int32 lb 1 $intval
logbuf_int64 lb 1001 $intval64
logbuf_string lb 2 "str"
logbuf_fmtstrn lb 3000000000 "fmtstr"
logbuf_data lb 4 "data"
set t [* [clock microseconds] 1000]
logbuf_time lb 5 $t

puts -nonewline "test#20.1 test getarg: 1234 321 $intval [+ $intval64 0] str fmtstr data "
puts            [clock format [/ $t 1000000000]]
puts -nonewline "test#20.2 test getarg: "
puts -nonewline "[logbuf_get_arg [dict get $lb buf] $logbuf_consts(MID)] "
puts -nonewline "[logbuf_get_arg [dict get $lb buf] $logbuf_consts(GRP)] "
puts -nonewline "[logbuf_get_arg [dict get $lb buf] $logbuf_consts(I32) 1] "
puts -nonewline "[logbuf_get_arg [dict get $lb buf] $logbuf_consts(I64) 1001] "
puts -nonewline "[logbuf_get_arg [dict get $lb buf] $logbuf_consts(STR) 2] "
puts -nonewline "[logbuf_get_arg [dict get $lb buf] $logbuf_consts(FMT) 3000000000] "
puts -nonewline "[logbuf_get_arg [dict get $lb buf] $logbuf_consts(DATA) 4] "
set t1 [logbuf_get_arg [dict get $lb buf] $logbuf_consts(TIME) 5]
puts            [clock format [/ $t1 1000000000]]


set lb [logbuf_get 0x7b]
logbuf_int32 lb 1 1234
logbuf_fmt lb "test#22.2 fmt message1 (embedded fmt string): int value %d"

set r {
[LOG_MSG_1]
INDEX = 0x7b
FMTSTR = test#23.2 fmt message2 (res string): int value %d
}

puts "test#22.1 fmt message1 (embedded fmt string): int value 1234"
puts [logbuf_format_message [dict get $lb buf]]
puts "test#23.1 fmt message2 (res string): int value 1234"
puts [logbuf_format_message -res $r [dict get $lb buf]]
puts "test#24.1 fmt message3 (cmdline fmt string): int value 1234"
puts [logbuf_format_message "test#24.2 fmt message3 (cmdline fmt string): int value %d" [dict get $lb buf]]
puts "test#25.1 fmt message4 (both res & cmdline fmt string): int value 1234"
puts [logbuf_format_message -res $r "test#25.2 fmt message4 (both res & cmdline fmt string): int value %d" [dict get $lb buf]]
