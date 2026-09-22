#!/usr/bin/env bash
# Parses every example config with config_build, mirroring what the CMake CI
# job does with the installed binary.
set -euo pipefail

config_build="$(pwd)/$1"

status=0
for cfg in examples/config_example*.cfg; do
  case "${cfg}" in
    # Fragments pulled in by config_example16.cfg; not standalone configs.
    *_base.cfg | *_overlay.cfg) continue ;;
  esac
  if "${config_build}" "${cfg}" > /dev/null; then
    echo "ok   ${cfg}"
  else
    echo "FAIL ${cfg}"
    status=1
  fi
done

exit "${status}"
