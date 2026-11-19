#!/usr/bin/env bash
#
# mayhem/build.sh — build SFML's fuzz harnesses and its own test suite.
#
#   build/mayhem-fuzz : SFML (all modules, static) + mayhem/harnesses/* with $SANITIZER_FLAGS,
#                       SanCov and DWARF 3; binaries copied to /mayhem/<harness>{,-standalone}
#   build/mayhem-test : SFML's Catch2 test suite with the project's normal (Release) flags; test.sh runs it
#
# Third-party libraries come from apt (SFML_USE_SYSTEM_DEPS=ON). The two that SFML always
# FetchContent's (SheenBidi, Catch2) are cached in the image under /opt/toolchains/deps and used via
# FETCHCONTENT_SOURCE_DIR_* with FETCHCONTENT_FULLY_DISCONNECTED, so this runs offline and re-runs.
# All build directories live under build/, which upstream's .gitignore already ignores.
set -euo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
: "${SRC:=/mayhem}"
: "${STANDALONE_FUZZ_MAIN:=/opt/mayhem/StandaloneFuzzTargetMain.c}"
export CC CXX

DEPS=/opt/toolchains/deps
SHEENBIDI_SRC=$DEPS/sheenbidi-3.0.0/src
CATCH2_SRC=$DEPS/catch2-3.9.1/src

cd "$SRC"
mkdir -p build

# SFML patches SheenBidi's CMakeLists (object library, no install) as its FetchContent PATCH_COMMAND,
# which a source-dir override skips: apply the same patch script to a copy in the build tree.
# Only replace the copy when it changed, so a re-run keeps its timestamps.
sb=build/mayhem-sheenbidi
rm -rf "$sb.tmp"
cp -r "$SHEENBIDI_SRC" "$sb.tmp"
chmod -R u+w "$sb.tmp"
cmake -DSHEENBIDI_DIR="$PWD/$sb.tmp" -P tools/sheenbidi/PatchSheenBidi.cmake
if [ -d "$sb" ] && diff -rq "$sb" "$sb.tmp" >/dev/null; then rm -rf "$sb.tmp"; else rm -rf "$sb"; mv "$sb.tmp" "$sb"; fi

offline=(-DFETCHCONTENT_FULLY_DISCONNECTED=ON
         -DFETCHCONTENT_SOURCE_DIR_SHEENBIDI="$PWD/$sb"
         -DFETCHCONTENT_SOURCE_DIR_CATCH2="$CATCH2_SRC")

# 1+2) Sanitized SFML + harnesses. SanCov on the library too (fuzzer-no-link) whenever a sanitizer
#      runtime is present to provide its callbacks; NDEBUG is deliberately not defined (keep asserts).
cov=""
[ -z "$SANITIZER_FLAGS" ] || cov="-fsanitize=fuzzer-no-link"
fuzzflags="-O1 $SANITIZER_FLAGS $cov $DEBUG_FLAGS"
cmake -S mayhem/harnesses -B build/mayhem-fuzz \
    -DCMAKE_BUILD_TYPE=Fuzz \
    -DCMAKE_C_FLAGS="$fuzzflags" -DCMAKE_CXX_FLAGS="$fuzzflags" \
    -DSFML_USE_SYSTEM_DEPS=ON -DSFML_BUILD_EXAMPLES=OFF -DSFML_BUILD_TEST_SUITE=OFF \
    -DLIB_FUZZING_ENGINE="$LIB_FUZZING_ENGINE" -DSTANDALONE_MAIN="$STANDALONE_FUZZ_MAIN" \
    -DLSAN_OFF_SOURCE="$SRC/mayhem/lsan_off.cc" \
    "${offline[@]}"
harnesses=(packet_fuzzer ipaddress_fuzzer http_fuzzer ftp_fuzzer string_fuzzer soundfile_fuzzer image_fuzzer font_fuzzer)
targets=()
for h in "${harnesses[@]}"; do targets+=("$h" "$h-standalone"); done
cmake --build build/mayhem-fuzz -j"$MAYHEM_JOBS" --target "${targets[@]}"
for t in "${targets[@]}"; do cp -f "build/mayhem-fuzz/$t" "/mayhem/$t"; done

# 3) SFML's own test suite, normal Release flags. Display and audio-device tests stay on (upstream's
#    defaults): test.sh hosts the display tests on Xvfb + fluxbox (Mesa llvmpipe), as upstream's Linux CI
#    does, and miniaudio falls back to its null backend without a sound card. IPv4 loopback tests stay on
#    (127.0.0.1 only); the IPv4-internet and IPv6 tests are off (upstream defaults; no route in a sandbox).
cmake -S . -B build/mayhem-test \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_FLAGS="$COVERAGE_FLAGS" -DCMAKE_CXX_FLAGS="$COVERAGE_FLAGS" \
    -DSFML_USE_SYSTEM_DEPS=ON -DSFML_BUILD_EXAMPLES=OFF -DSFML_BUILD_TEST_SUITE=ON \
    -DSFML_RUN_DISPLAY_TESTS=ON -DSFML_RUN_AUDIO_DEVICE_TESTS=ON \
    -DSFML_RUN_IPV4_LINK_TESTS=ON -DSFML_RUN_IPV4_INTERNET_TESTS=OFF \
    -DSFML_RUN_IPV6_LINK_TESTS=OFF -DSFML_RUN_IPV6_INTERNET_TESTS=OFF \
    "${offline[@]}"
cmake --build build/mayhem-test -j"$MAYHEM_JOBS"
