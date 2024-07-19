#!/bin/python

"""Convert an output file of remage-doc-dump to a rst file."""

import math
import re
import sys

if len(sys.argv) < 2:
    msg = "need to pass an original file"
    raise ValueError(msg)

path = sys.argv[1]

outlines = ["remage macro command reference", "=" * 31, ""]
infile = open(path, "rt")
inlines = [line.strip("\n") for line in infile]


def remove_whitespace_lines_end(lines: list):
    for i in range(len(lines) - 1, 0, -1):
        if lines[i].strip() not in ("", "::"):
            break
        del lines[i]


idx = 0
in_cmdblock = False
lastlevel = -1

for line in inlines:
    if re.match(r"Command directory path : /RMG/", line):
        remove_whitespace_lines_end(outlines)
        outlines.extend(["", line, "-" * len(line), ""])
        in_cmdblock = True
        lastlevel = -1
    elif re.match(r"Command /RMG/", line):
        remove_whitespace_lines_end(outlines)
        outlines.extend(["", line, "^" * len(line), ""])
        in_cmdblock = True
        lastlevel = -1
    elif in_cmdblock and (line == "Guidance :"):
        pass
    elif in_cmdblock and (inlines[idx - 1] == "Guidance :") and not line.startswith(" " * 2):
        outlines.extend([line, ""])
    elif in_cmdblock and line == " Commands : " and not inlines[idx + 1].startswith(" " * 4):
        pass
    elif in_cmdblock and line != "":
        stripped_line = line.strip()
        indent = math.ceil((len(line) - len(stripped_line)) / 2)
        if indent > lastlevel + 1:  # parts of the output have the wrong indentation.
            indent = lastlevel + 1
        m = re.match(r"(.*)( [:* ] ?)(.*)?$", line)
        if m:
            g = list(m.groups())
            sep = g[1].strip()
            fmt = "**" if sep == ":" else "*"
            if len(g) > 1:
                g[0] = f"{fmt}{g[0].strip()}{fmt}"
                g[1] = ": "
            if len(g) > 2 and g[2] != "":
                g[2] = f"``{g[2].strip()}``"
            stripped_line = "".join(g)
        outlines.append("    " * indent + "* " + stripped_line)
        lastlevel = indent
    idx += 1

outfile = open("rmg-commands.rst", "wt")
outfile.writelines([l + "\n" for l in outlines])
