set logbuf_consts(MID) 1
set logbuf_consts(GRP) 3
set logbuf_consts(I32) 16
set logbuf_consts(I64) 17
set logbuf_consts(STR) 18
set logbuf_consts(DATA) 19
set logbuf_consts(FMT) 20

proc logbuf_int32 {var argn i} {
 upvar $var lb
 dict append lb buf [binary format "ccn" $::logbuf_consts(I32) $argn $i]
}

proc logbuf_int64 {var argn i} {
 upvar $var lb
 dict append lb buf [binary format "ccm" $::logbuf_consts(I64) $argn $i]
}

proc logbuf_data {var argn d} {
 upvar $var lb
 dict append lb buf [binary format "ccna*" $::logbuf_consts(DATA) $argn [string length $d] $d]
}

proc logbuf_string {var argn d} {
 upvar $var lb
 dict append lb buf [binary format "cca*c" $::logbuf_consts(STR) $argn $d 0]
}

proc logbuf_fmt {var d} {
 upvar $var lb
 dict append lb buf [binary format "cca*c" $::logbuf_consts(FMT) 0 $d 0]
}

proc logbuf_fmtstrn {var argn d} {
 upvar $var lb
 dict append lb buf [binary format "cca*c" $::logbuf_consts(FMT) $argn $d 0]
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

 if {$grp eq ""} {
  if {$lc ne ""} {dict set r counter $lc}
  dict set r buf [binary format "cn" $::logbuf_consts(MID) $mid]
  return $r
 }

 if {!([dict get $lc mask] & $grp)} {return ""}

 dict set r counter $lc
 dict set r buf [binary format "cncn" $::logbuf_consts(MID) $mid $::logbuf_consts(GRP) $grp]
 return $r
}

proc logbuf_send {var} {
 upvar $var lb
 [dict get lb counter send] [dict get lb buf]
 dict unset lb
}
