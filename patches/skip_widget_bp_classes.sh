#!/bin/bash
# When CppGenerator / IDAMappingGenerator iterates packages and tries to
# emit WidgetBlueprintGeneratedClass / BlueprintGeneratedClass instances,
# look them up in StructManager and if missing, SKIP instead of crashing.
# PUBG has thousands of BP widgets that crash the generator mid-way.
set -e
F="Dumper/Generator/Managers/StructManager.cpp"
# Already returns Invalid on missing key, so GetInfo itself is fine.
# The .at() throws are in CollisionManager or PackageManager callers who
# ASSUME the struct exists. The safest wide fix is: make GetInfo's warning
# log only at DEBUG level (it's firing 100s of times slowing dump),
# and make StructInfoHandle.IsValid() report false so callers skip.

python3 - "$F" <<'PY'
import sys
p = sys.argv[1]
t = open(p).read()
# Downgrade Warning to Debug so logs aren't flooded
old = 'MD7::Globals::GLogger.FmtWrite(ELogLevel::Warning, "StructManager::GetInfo: Struct not in map'
new = 'MD7::Globals::GLogger.FmtWrite(ELogLevel::Debug, "StructManager::GetInfo: Struct not in map'
t = t.replace(old, new)
open(p,'w').write(t)
print("patched log level")
PY

# Now patch CollisionManager.h to use .find() instead of .at()
F2="Dumper/Generator/Managers/CollisionManager.h"
python3 - "$F2" <<'PY2'
import sys, re
p = sys.argv[1]
t = open(p).read()
# Replace NameInfos.at(Struct.GetIndex()) with safe lookup
# Original:
#   CollisionManager::NameContainer& InfosForStruct = NameInfos.at(Struct.GetIndex());
# Fix: throw-free version
old = 'CollisionManager::NameContainer& InfosForStruct = NameInfos.at(Struct.GetIndex());'
new = ('auto NameInfos_It = NameInfos.find(Struct.GetIndex());\n'
       '\t\tif (NameInfos_It == NameInfos.end()) {\n'
       '\t\t\tstatic CollisionManager::NameContainer EmptyContainer{};\n'
       '\t\t\treturn EmptyContainer.empty() ? NameInfo{} : EmptyContainer.at(0);\n'
       '\t\t}\n'
       '\t\tCollisionManager::NameContainer& InfosForStruct = NameInfos_It->second;')
count = t.count(old)
if count > 0:
    t = t.replace(old, new)
    open(p,'w').write(t)
    print(f"patched {count} NameInfos.at() calls")
else:
    print("no NameInfos.at pattern found, skipping")
PY2
