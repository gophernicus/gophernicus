# Convert filetypes.conf to filetypes.h.

BEGIN {
    print "#define FILETYPES \\"
}
/^[^#]/ {
    for (i = 2; i <= NF; ++i) {
	printf "\t\"%s\", \"%s\", \\\n", $i, $1
    }
}
END {
    printf "\tNULL, NULL\n"
}
