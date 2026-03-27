
***************
Assembly errors
***************

Source files
============

error: non-constant argument supplied to TIMES
----------------------------------------------

If this error occurs around an ``istruc`` block, it's usually because the
preprocessor didn't find the corresponding ``struc`` block. If it's found in an
include file, make sure that file is included at the top.

* Ensure all necessary includes are put at the top of the source file.

SEGFAULT when executing the first instruction under a label
-----------------------------------------------------------

This can occur, for example, if you jump to a label defined in another source
file which isn't placed under a ``section .text`` directive.

* Ensure all section directives don't end with a semicolon.
* Ensure all instructions are written following a ``section .text`` directive.
