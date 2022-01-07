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
BEGIN { FS = ";"; errs = 0 }

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
    if (prevop) {
	# XXX complain if prevop/var not set, or unreasonable?
	if (length(prevop) > 1)
	     prevop = substr(prevop,1,1) "v" substr(prevop,2)
	else
	     prevop = "v" prevop
	if (!seen[prevop]) {
	    print ".globl", prevop
	    seen[$1]++
	}
	print prevop ";" prevvar
	prevop = prevvar = ""
    }
    next
}

prevop {
    if (!seen[prevop]) {
	print ".globl", prevop
	seen[prevop]++
    }
    if (prevvar)
	print prevop ";" prevvar
    else
	print prevop
    prevop = prevvar = ""
}

/^[pl][0-9][0-9]*:/ { print; next }

# opcode
/^[a-zA-Z]/ {
    if ($1 ~ /a$/) {		# auto (a/ia)
	prevop = $1
	prevvar = $2
	next
    }
    # extern (x/ix)
    if ($1 ~ /x$/ && $2 ~ /^\.[a-z]/) {	# 'x;pN' generated for string lits
	if (!seen[$2]) {
	    print ".globl", $2
	    seen[$2]++
	}
	prevop = $1
	prevvar = $2
	next
    }

    # output extern request for opcode
    if (!seen[$1]) {
	print ".globl", $1
	seen[$1]++
    }
}

# B global storage or function
# make global, if not already seen
/^\..*:/ {
    lbl = substr($1, 1, index($1, ":")-1)
    if (!seen[lbl]) {
	print ".globl", lbl
	seen[lbl]++
    }
}

{ print }
