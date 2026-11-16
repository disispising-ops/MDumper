#!/bin/bash
set -e
F="Dumper/Generator/Managers/StructManager.cpp"
# Downgrade Warning log to Debug so logs aren't flooded by hundreds of BP widget misses
sed -i 's|ELogLevel::Warning, "StructManager::GetInfo: Struct not in map|ELogLevel::Debug, "StructManager::GetInfo: Struct not in map|' "$F"
echo "quieted StructManager warnings"
