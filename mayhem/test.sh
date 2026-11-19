#!/usr/bin/env bash
#
# mayhem/test.sh — RUN SFML's own Catch2 test suite (built by mayhem/build.sh in build/mayhem-test
# with the project's normal flags). Each module's test binary runs from its test/<Module> directory
# (where its resource files live, as SFML's ctest registration does) with Catch2's JUnit reporter;
# the counts come from those reports, so a binary that produces no report counts as failed.
# The display tests run on a private Xvfb server with fluxbox (Mesa llvmpipe), as upstream's Linux CI
# runs them; the audio-device tests use miniaudio's null backend when there is no sound card.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${SRC:=/mayhem}"
cd "$SRC"

emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}

# Start Xvfb + fluxbox on a free display; stop both on exit.
xpids=()
cleanup() { for p in "${xpids[@]}"; do kill "$p" 2>/dev/null; done; wait 2>/dev/null; }
trap cleanup EXIT
dnum=99
while [ -e "/tmp/.X${dnum}-lock" ] || [ -e "/tmp/.X11-unix/X${dnum}" ]; do dnum=$((dnum + 1)); done
export DISPLAY=":$dnum"
Xvfb "$DISPLAY" -screen 0 1920x1080x24 -nolisten tcp >/dev/null 2>&1 &
xpids+=($!)
for _ in $(seq 1 100); do [ -e "/tmp/.X11-unix/X${dnum}" ] && break; sleep 0.1; done
if [ ! -e "/tmp/.X11-unix/X${dnum}" ]; then echo "Xvfb did not start on $DISPLAY" >&2; fi
fluxbox >/dev/null 2>&1 &
xpids+=($!)
sleep 2

bin="$SRC/build/mayhem-test/bin"
out="$(mktemp -d "${TMPDIR:-/tmp}/sfml-test.XXXXXX")"
passed=0; failed=0; skipped=0
for module in System Network Audio Window Graphics; do
  exe="$bin/test-sfml-${module,,}"
  if [ ! -x "$exe" ]; then
    echo "missing test runner $exe (mayhem/build.sh should have built it)" >&2
    failed=$((failed + 1)); continue
  fi
  report="$out/$module.xml"
  ( cd "$SRC/test/$module" && "$exe" --reporter "junit::out=$report" --reporter console ) || true
  attr() { grep -o "<testsuite [^>]*" "$report" | grep -o " $1=\"[0-9]*\"" | head -1 | grep -o '[0-9]*'; }
  if [ ! -s "$report" ] || ! grep -q "<testsuite " "$report"; then
    echo "test-sfml-${module,,}: no JUnit report" >&2
    failed=$((failed + 1)); continue
  fi
  t=$(attr tests); f=$(attr failures); e=$(attr errors); s=$(attr skipped)
  t=${t:-0}; f=${f:-0}; e=${e:-0}; s=${s:-0}
  if [ "$t" -eq 0 ]; then
    echo "test-sfml-${module,,}: ran no tests" >&2
    failed=$((failed + 1)); continue
  fi
  echo "test-sfml-${module,,}: tests=$t failures=$f errors=$e skipped=$s"
  failed=$((failed + f + e)); skipped=$((skipped + s)); passed=$((passed + t - f - e - s))
done
rm -rf "$out"
emit_ctrf "catch2" "$passed" "$failed" "$skipped"
