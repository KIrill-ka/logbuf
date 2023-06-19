set logbuf_consts(MID) 1
set logbuf_consts(GRP) 3
set logbuf_consts(I32) 16
set logbuf_consts(I64) 17
set logbuf_consts(STR) 18
set logbuf_consts(DATA) 19
set logbuf_consts(FMT) 20
set logbuf_consts(TIME) 21

proc _logbuf_put_type_argn {var type argn} {
 upvar $var lb
 if {$argn < 255} {
  dict append lb buf [binary format "cc" $type $argn]
 } else {
  dict append lb buf [binary format "ccn" $type 255 $argn]
 }
}

proc logbuf_int32 {var argn i} {
 upvar $var lb
 _logbuf_put_type_argn lb $::logbuf_consts(I32) $argn
 dict append lb buf [binary format "n" $i]
}

proc logbuf_int64 {var argn i} {
 upvar $var lb
 _logbuf_put_type_argn lb $::logbuf_consts(I64) $argn
 dict append lb buf [binary format "m" $i]
}

proc logbuf_data {var argn d} {
 upvar $var lb
 _logbuf_put_type_argn lb $::logbuf_consts(DATA) $argn
 dict append lb buf [binary format "na*" [string length $d] $d]
}

proc logbuf_string {var argn d} {
 upvar $var lb
 _logbuf_put_type_argn lb $::logbuf_consts(STR) $argn
 dict append lb buf [binary format "a*c" $d 0]
}

proc logbuf_u8string {var argn d} {
 upvar $var lb
 _logbuf_put_type_argn lb $::logbuf_consts(STR) $argn
 dict append lb buf [binary format "a*c" [encoding convertto utf-8 $d] 0]
}

proc logbuf_fmt {var d} {
 upvar $var lb
 dict append lb buf [binary format "cca*c" $::logbuf_consts(FMT) 0 $d 0]
}

proc logbuf_fmtstrn {var argn d} {
 upvar $var lb
 _logbuf_put_type_argn lb $::logbuf_consts(FMT) $argn
 dict append lb buf [binary format "a*c" $d 0]
}

proc logbuf_time {var argn t} {
 upvar $var lb
 _logbuf_put_type_argn lb $::logbuf_consts(TIME) $argn
 dict append lb buf [binary format "m" $t]
}

proc logbuf_get args {
 set l [llength $args]

 if {$l == 1} {
  set mid [lindex $args 0]
  set grp ""
  set lc ""
 } elseif {$l == 2} {
  set lc [lindex $args 0]
  set mid [lindex $args 1]
  set grp ""
 } elseif {$l == 3} {
  set lc [lindex $args 0]
  set grp [lindex $args 1]
  set mid [lindex $args 2]
 } else {
   error "Wrong # args: should be \"logbuf_get ?lc? ?grp? mid\""
 }

 set time [expr {[clock microseconds] * 1000}]
 if {$grp eq ""} {
  if {$lc ne ""} {dict set r counter $lc}
  dict set r buf [binary format "cn" $::logbuf_consts(MID) $mid]
  logbuf_time r 0 $time
  return $r
 }

 if {$lc ne ""} {
  if {!([dict get $lc mask] & $grp)} {return ""}
  dict set r counter $lc
 }

 dict set r buf [binary format "cncm" $::logbuf_consts(MID) $mid $::logbuf_consts(GRP) $grp]
 logbuf_time r 0 $time
 return $r
}

proc logbuf_send {var} {
 upvar $var lb
 [dict get lb counter send] [dict get lb buf]
 dict unset lb
}
