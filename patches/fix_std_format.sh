#!/bin/bash
set -e
F="Dumper/main_android.cpp"
# Remove the bad include we added earlier (if present)
sed -i '/^#include <fmt\/format\.h>$/d' "$F"
# Replace std::format with fmt::format (fmt already pulled via Logger.h / Utils.h includes in MD7)
sed -i 's|std::format(|fmt::format(|g' "$F"
echo "patched std::format -> fmt::format in $F"
