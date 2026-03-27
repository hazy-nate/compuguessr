#!/usr/bin/env python3

import sys
import re

def process_file(file_path):
    with open(file_path, 'r', encoding='utf-8') as f:
        data = f.read()

    # Clean <pre> indentation
    # Captures leading whitespace of the first line to use as a baseline.
    def strip_indent(match):
        indent = match.group(1)
        content = match.group(2)
        # Remove that specific indent from the start of every line in the block
        cleaned = re.sub(rf"^{re.escape(indent)}", "", content, flags=re.M)
        return f"<pre>{cleaned}</pre>"

    # Matching across newlines with re.S.
    data = re.sub(r"<pre>([ \t]*)(.*?)</pre>", strip_indent, data, flags=re.S)

    # Convert double-backslashes to HTML line breaks.
    data = data.replace(r"\\", "<br>")

    with open(file_path, 'w', encoding='utf-8') as f:
        f.write(data)

#===============================================================================
# RUNNING
#===============================================================================

if __name__ == "__main__":
    for filepath in sys.argv[1:]:
        process_file(filepath)
