#!/bin/bash
set -e
F="Dumper/main_android.cpp"
# replace std::format with fmt::format (MD7 already bundles fmt headers)
sed -i 's|std::format(|fmt::format(|g' "$F"
# make sure fmt header is included
if ! grep -q '#include <fmt/format.h>' "$F"; then
  sed -i '1i #include <fmt/format.h>' "$F"
fi
echo "patched std::format -> fmt::format in $F"
