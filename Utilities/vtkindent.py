#!/usr/bin/env python
"""
This script take a "whitesmith" indented source file as input,
and re-indent the braces according to the "allman" style.
"""

import sys
import re

if len(sys.argv) != 2:
    sys.stderr.write("Please provide a single file to re-indent.\n")
    sys.stderr.write("The output is written to stdout.\n")
    sys.exit(1)

keychar = re.compile("[/\"\']")
c_comment = re.compile("\\/\\*(\\*(?!\\/)|[^*])*\\*\\/")
c_comment_start = re.compile("\\/\\*(\\*(?!\\/)|[^*])*$")
c_comment_end = re.compile("^(\\*(?!\\/)|[^*])*\\*\\/")
cpp_comment = re.compile("\\/\\/.*")
string_literal = re.compile("\"([^\"]|\\\\\")*\"")
string_literal_start = re.compile("\"([^\"]|\\\\\")*\\$")
string_literal_end = re.compile("^([^\"]|\\\")*\"")
string_literal = re.compile("\"([^\"]|\\\\\")*\"")
char_literal = re.compile("\'([^\']|\\\\\')+\'")
char_literal_start = re.compile("\'([^\']|\\\\\')+\\$")
char_literal_end = re.compile("^([^\']|\\\\\')+\'")

f = open(sys.argv[1])
lines = f.readlines()
n = len(lines)
newlines = []

cont = None

for i in range(n):
    line = lines[i]
    if cont is not None:
        match = cont.match(line)
        if match:
            line = line[match.end():]
            cont = None
        else:
            if cont is c_comment_end:
                line = ""
            else:
                line = "\\"
    pos = 0
    while True:
        match = keychar.search(line, pos)
        if match is None:
            break
        pos = match.start()
        end = match.end()
        match = c_comment.match(line, pos)
        if match:
            line = line[0:pos] + " " + line[match.end():]
            pos += 1
            continue
        match = c_comment_start.match(line, pos)
        if match:
            line = line[0:pos]
            cont = c_comment_end
            break
        match = cpp_comment.match(line, pos)
        if match:
            line = line[0:pos]
            break
        match = string_literal.match(line, pos)
        if match:
            line = line[0:pos] + "\"\"" + line[match.end():]
            pos += 2
            continue
        match = string_literal_start.match(line, pos)
        if match:
            line = line[0:pos] + "\"\"\\"
            cont = string_literal_end
            break
        match = char_literal.match(line, pos)
        if match:
            line = line[0:pos] + "\' \'" + line[match.end():]
            pos += 3
            continue
        match = char_literal_start.match(line, pos)
        if match:
            line = line[0:pos] + "\' \'\\"
            cont = char_literal_end
            break
        pos += 1

    newlines.append(line)

"""
Use a stack to keep track of braces and, whenever a closing brace is found,
properly indent it and its opening brace.
"""

# stack holds tuples (delim, row, col, newcol)
stack = []

delims = re.compile("[{}()\\[\\]]")
spaces = re.compile("  *")
indent = re.compile("  ")

for i in range(n):
    line = newlines[i]
    pos = 0
    # newpos will be set to the correct indentation
    newpos = 0
    while True:
        match = delims.search(line, pos)
        if match is None:
            break
        pos = match.start()
        delim = line[pos]
        if delim in ('{', '(', '['):
            stack.append((delim, i, pos, newpos))
        else:
            try:
                ldelim, j, k, l = stack.pop()
            except IndexError:
                ldelim = ""
            if ldelim != {'}':'{', ')':'(', ']':'['}[delim]:
                sys.stderr.write(sys.argv[1] + ":" + str(i) + ": ")
                sys.stderr.write("mismatched \'" + delim + "\'\n")
            if (ldelim == '{' and delim == '}' and
                spaces.sub("", lines[i][0:pos]) == "" and
                spaces.sub("", lines[j][0:k])  == ""):
                lines[i] = indent.sub("", lines[i], count=1)
                lines[j] = indent.sub("", lines[j], count=1)
        pos += 1

for line in lines:
    print line.rstrip()
