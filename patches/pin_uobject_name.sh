#!/bin/bash
# Insert OverrideInGenOffsets into PUBG profile after OverrideDecryptCallbacks
set -e
F="Dumper/Profile/CustomProfiles/Shared/PUBG.h"
if ! grep -q "OverrideInGenOffsets" "$F"; then
python3 - "$F" <<'PY'
import sys, re
p = sys.argv[1]
t = open(p).read()
# Find end of OverrideDecryptCallbacks function and insert our override after it
anchor = "	void OverrideDecryptCallbacks(FDecryptCallbacks& Callbacks) const override"
insertion = """
	void OverrideInGenOffsets(FInGenOffsets& Offsets) const override
	{
#ifdef __LP64__
		Offsets.UObject.Name = 0x18;
#else
		Offsets.UObject.Name = 0x10;
#endif
	}

"""
if anchor not in t:
    print("anchor not found - skipping"); sys.exit(0)
# Find the closing brace of the OverrideDecryptCallbacks function
i = t.find(anchor)
# find matching }  — simple brace balancer starting from first {
start = t.find("{", i)
depth = 0
j = start
while j < len(t):
    if t[j] == "{": depth += 1
    elif t[j] == "}":
        depth -= 1
        if depth == 0: break
    j += 1
# insert after the closing }
new = t[:j+1] + "\n" + insertion + t[j+1:]
open(p,"w").write(new)
print("inserted OverrideInGenOffsets into", p)
PY
else
echo "OverrideInGenOffsets already present"
fi
