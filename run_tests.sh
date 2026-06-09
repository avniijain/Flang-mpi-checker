#!/bin/bash
set -u

PROJ="$(cd "$(dirname "$0")" && pwd)"
BUILD=$PROJ/build
CHECKER=$BUILD/tools/flang-mpi-checker/flang-mpi-checker
MODDIR=$BUILD/mpi_mods

if [ -n "${FLANG_COMPILER:-}" ]; then
  FLANG="$FLANG_COMPILER"
elif command -v flang-new-18 >/dev/null 2>&1; then
  FLANG=flang-new-18
elif command -v flang-new >/dev/null 2>&1; then
  FLANG=flang-new
elif command -v flang >/dev/null 2>&1; then
  FLANG=flang
else
  echo "ERROR: Flang compiler not found."
  echo "Set FLANG_COMPILER to the LLVM 18 Flang executable."
  exit 1
fi

if [ ! -f "$CHECKER" ]; then
  echo "ERROR: Binary not found. Build first:"
  echo "  cd $BUILD && ninja"
  exit 1
fi

echo "=== Compiling MPI stub ==="
mkdir -p "$MODDIR"
"$FLANG" -c "$PROJ/test/mpi_stub.f90" -o /dev/null -J "$MODDIR"
echo "Stub OK"
echo ""
echo "=== Running 22 tests ==="

PASS=0; FAIL=0

for f in "$PROJ"/test/buffer_mismatch/*.f90 \
          "$PROJ"/test/noncontiguous/*.f90 \
          "$PROJ"/test/derived_type/*.f90 \
          "$PROJ"/test/optional_args/*.f90 \
          "$PROJ"/test/collective_ordering/*.f90 \
          "$PROJ"/test/fortran_specific/*.f90; do
  name=$(basename $f)
  result=$(timeout 15 "$CHECKER" -I "$MODDIR" "$f" 2>&1)
  total=$(echo "$result" | grep "Total" | awk '{print $3}')
  rules=$(echo "$result" | grep "Rule:" | xargs)
  sem=$(echo "$result" | grep -c "semantic errors" || true)
  if [ "$sem" -gt 0 ]; then
    echo "  FAIL  $name (semantic errors)"
    FAIL=$((FAIL+1))
  else
    echo "  OK    $name: diags=$total $rules"
    PASS=$((PASS+1))
  fi
done

echo ""
echo "=== Results: $PASS/22 OK, $FAIL failed ==="
