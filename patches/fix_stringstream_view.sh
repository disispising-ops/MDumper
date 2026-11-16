#!/bin/bash
set -e
F="Dumper/Generator/Generators/MappingGenerator.cpp"
if grep -q "Data.view()" "$F"; then
python3 - "$F" <<'PY'
import sys
p = sys.argv[1]
t = open(p).read()
# Replace Data.view() with Data.str() — same semantics, libc++ compat
t = t.replace("std::string_view DataView     = Data.view();",
              "std::string DataView_ = Data.str();\n        std::string_view DataView = DataView_;")
t = t.replace("std::string_view DataView = Data.view();",
              "std::string DataView_ = Data.str();\n        std::string_view DataView = DataView_;")
open(p,"w").write(t)
print("patched", p)
PY
fi
