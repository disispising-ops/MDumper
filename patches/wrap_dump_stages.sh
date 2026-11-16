#!/bin/bash
# Wrap each Generator::Generate<...>() in DumperMain.cpp with try/catch so
# a single stage throwing (e.g. CppGenerator hitting a missing map key)
# doesn't lose the ENTIRE dump — other stages (Mappings, IDAMappings,
# Dumpspace) still produce their output.
set -e
F="Dumper/DumperMain.cpp"
python3 - "$F" <<'PY'
import sys, re
p = sys.argv[1]
t = open(p).read()
# Find every `Generator::Generate<...>();` and the surrounding block,
# and wrap in a try/catch that logs + continues.
pattern = re.compile(
    r'(\{\s*std::chrono::high_resolution_clock::time_point Start = std::chrono::high_resolution_clock::now\(\);\s*)'
    r'(Generator::Generate<[A-Za-z_]+>\(\);)'
    r'(\s*auto Elapsed = std::chrono::high_resolution_clock::now\(\) - Start;\s*MD7::Globals::GLogger\.FmtWrite\(ELogLevel::Info, "Generated in \{\}\\n", Utils::ChronoDurationToString\(Elapsed\)\);\s*\})'
)

def wrap(m):
    pre, call, post = m.group(1), m.group(2), m.group(3)
    gen_name = re.search(r'Generate<([A-Za-z_]+)>', call).group(1)
    return (pre + "try { " + call + " } "
            "catch (const std::exception& e) { "
            "MD7::Globals::GLogger.FmtWrite(ELogLevel::Error, \"" + gen_name +
            " threw: {}, continuing\\n\", e.what()); } "
            "catch (...) { "
            "MD7::Globals::GLogger.FmtWrite(ELogLevel::Error, \"" + gen_name +
            " threw unknown, continuing\\n\"); }" + post)

new_t, n = pattern.subn(wrap, t)
if n == 0:
    print("no Generator::Generate<> found, nothing to wrap")
    sys.exit(0)
open(p, 'w').write(new_t)
print(f"wrapped {n} Generator::Generate<> calls in try/catch")
PY

# Also return success from RunDump even if the main dump throws, so any
# partial output still gets zipped by the APK.
F2="Dumper/main_android.cpp"
python3 - "$F2" <<'PY2'
import sys, re
p = sys.argv[1]
t = open(p).read()
# No change needed - RunDump already returns bool from FDumperMain::Run()
PY2
