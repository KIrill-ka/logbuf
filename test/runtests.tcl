#!/usr/bin/tclsh8.6

set tests {
 build_dir/logtest
 build_dir/logtest-fmt
 build_dir/logtest-argn32
 ./tcltest.tcl
 build_dir/logtest-nested-fmt
 build_dir/logtest-res
 build_dir/logtest-pref
}
set verbose 0


proc runtest {t} {
 set fail_count 0
 set r [exec $t]
 set lines [split $r "\n"]
 foreach l $lines {
  if {$::verbose > 1} {puts $l}
  if {![regexp {^test#([0-9][0-9])[.]([1-2]) (.*)$} $l m num subnum txt]} continue
  if {$subnum == 1} {
   set samples($num) $txt
  } elseif {$subnum == 2} {
   set tests($num) $txt
  } else {
   error "unexpected test number #$num.$subnum"
  }
 }
 foreach num [lsort -integer [array names samples]] {
  if {$samples($num) ne $tests($num)} {
   puts "$num: failed"
   puts "> $samples($num)"
   puts "> $tests($num)"
   incr fail_count
  } else {
   if {$::verbose > 0} {puts "$num: ok"}
  }
 }
 return $fail_count
}

namespace import ::tcl::mathop::*

if {[lindex $argv 0] eq "-v"} {
 set verbose 1
 set v [lindex $argv 1]
 if {$v ne ""} {set verbose $v}
}

set fail_count 0
foreach t $tests {
 incr fail_count [runtest $t]
}

exit $fail_count
