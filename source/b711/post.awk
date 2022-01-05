# post-processing for b11.c output
# Phil Budne, January 2021
#
# the original PDP-11 'bc' compiler output "intermediate language"
# that was turned into assembler by 'ba'
#
# this script does two jobs:
# 1. adds .extrn requests as needed for both interpreter ops and extrns
# 2. performs l-valuization for var refs after the fact.
#
# using awk is almost certainly an anachronism!
BEGIN { FS = ";"; errs = 0; print "\tjmp 9f" }

# look for compiler errors; PDP-7 had no stderr,
# when did the convention for fd 2 appear??
# B runtime doesn't have a "ferr"
/^.. [0-9]*$/ {
     print > "/dev/stderr"
     errs++
     next
}

# PLV output after the fact to say previous auto/ext var ref should push lvalue
# must appear IMMEDIATELY after the var ref!
/^PLV$/ {
     # XXX complain if prevop/var not set???
     if (length(prevop) > 1)
	  prevop = substr(prevop,1,1) "v" substr(prevop,2)
     else
	  prevop = "v" prevop
     print prevop ";" prevvar
     prevop = prevvar = ""
     next
}

prevop && prevvar {
     print prevop ";" prevvar
     prevop = prevvar = ""
}

/^l[0-9][0-9]*:/ { print; next }

# opcode
/^[a-zA-Z]/ {
     # output extern request for opcode
     if (!seen[$1]) {
	  print ".globl", $1
	  seen[$1]++
     }
     if ($1 ~ /a$/) {		# auto (a/ia)
	  prevop = $1
	  prevvar = $2
	  next
     }
     if ($1 ~ /x$/) {		# extern (x/ix)
	  if ($2 ~ /^\.[a-z]/) {
	       print ".globl", $2
	       seen[$2]++
	  }
	  prevop = $1
	  prevvar = $2
	  next
     }
}

# B function declaration
# make global, if not already seen
/^\..*:\.\+2/ {
     lbl = substr($1, 1, index($1, ":")-1)
     if (!seen[lbl]) {
	  print ".globl", lbl
	  seen[lbl]++
     }
}

{ print }

END {
    print "9:"
    print ".globl chain"
    print "\tjsr r5,chain"
    print "\t0"
    exit $errs
}
