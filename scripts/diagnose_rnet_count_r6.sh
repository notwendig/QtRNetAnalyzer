#!/usr/bin/env bash
set -euo pipefail
repo=${1:-$(pwd)}
cd "$repo"
echo "== rnetframemodel.h enum =="
grep -n -A14 "enum Column" src/rnetframemodel.h || true
echo
echo "== Count DisplayRole / refresh =="
grep -n "ColCount\|RNET_COUNT_R6" src/rnetframemodel.cpp src/mainwindow.cpp src/rnetframedelegate.cpp || true
echo
echo "== git diff --stat =="
git diff --stat || true
