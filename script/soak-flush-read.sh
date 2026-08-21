#!/bin/sh
# Reproduce the read-during-flush measurement.
#
# The defect this measures -- a read routed to the root file, past a directory
# that has not yet published the file covering the key, and told "no such key"
# for good -- has no unit test. Four attempts to build one are written up in
# issue #85; every tool in test/rad/ keeps the store small enough that the read
# finds its answer in memory and never has to route past an unfinished
# directory. So the evidence is this measurement, and this script is it.
#
#   sh script/soak-flush-read.sh <runs-per-worker> [workers]
#
# Two things it needs to be worth running:
#
#   Load. The window only opens when a flush is stretched by CPU contention.
#   The same build that fails 7.2% of the time on four pinned cores failed 0
#   times in 16 sequential runs on one. Fewer workers is not just slower, it is
#   blind.
#
#   A control that goes red. Pass --magnify to run the build with r.held
#   released and the read fix removed, which fails about 7% of the time. If that
#   does not go red, the setup has no detection power and a clean result from
#   the other arm means nothing.
set -eu

RUNS=${1:-500}
WORKERS=${2:-6}
MAGNIFY=false
for a in "$@"; do [ "$a" = "--magnify" ] && MAGNIFY=true; done

ROOT=$(cd "$(dirname "$0")/.." && pwd)
OUT=${SOAK_OUT:-$(mktemp -d)}
mkdir -p "$OUT"

if [ "$MAGNIFY" = true ]; then
  echo "MAGNIFIED: r.held released, read fix removed -- this arm is expected to fail."
  echo "Apply by hand before running; see issue #85."
fi

echo "$WORKERS workers x $RUNS runs, output in $OUT"

i=1
while [ "$i" -le "$WORKERS" ]; do
  (
    n=1
    while [ "$n" -le "$RUNS" ]; do
      rm -rf "$ROOT/tmp" >/dev/null 2>&1 || true
      timeout 300 taskset -c "$i" npx cross-env GUN_TEST_TMP=1 MULTICAST=false \
        ZEN_SILENCE_TEST_WARNINGS=1 node --import ./test/loaders/register-zen-minified.mjs \
        ./node_modules/mocha/bin/_mocha --exit test/rad/flush-read.js \
        > "$OUT/w$i-run$n.txt" 2>&1 || true
      if grep -q failing "$OUT/w$i-run$n.txt"; then
        echo "w$i run$n FAILED" >> "$OUT/tally"
      else
        echo "w$i run$n ok" >> "$OUT/tally"
        rm -f "$OUT/w$i-run$n.txt"
      fi
      n=$((n + 1))
    done
  ) &
  i=$((i + 1))
done
wait

total=$(grep -c . "$OUT/tally")
bad=$(grep -c FAILED "$OUT/tally" || true)
echo "$bad failures / $total runs"
[ "$bad" -eq 0 ] || echo "kept output of every failing run in $OUT"
