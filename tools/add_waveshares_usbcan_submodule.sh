#!/usr/bin/env bash
set -euo pipefail

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$PROJECT_DIR"

SUB_PATH="third_party/waveshares_USBCAN_B"
SUB_URL="git@github.com:notwendig/waveshares_USBCAN_B.git"
SUB_BRANCH="main"

if [ ! -d .git ]; then
    echo "error: $PROJECT_DIR is not a git repository" >&2
    exit 1
fi

has_gitlink() {
    git ls-files --stage -- "$SUB_PATH" | awk '$1 == "160000" { found=1 } END { exit found ? 0 : 1 }'
}

section_exists() {
    git config --file .gitmodules --get "submodule.${SUB_PATH}.url" >/dev/null 2>&1
}

remove_stale_module_state() {
    echo "[*] removing stale submodule metadata for $SUB_PATH"
    git submodule deinit -f -- "$SUB_PATH" >/dev/null 2>&1 || true
    git rm -f --cached -- "$SUB_PATH" >/dev/null 2>&1 || true
    rm -rf ".git/modules/$SUB_PATH"

    if section_exists; then
        git config -f .gitmodules --remove-section "submodule.${SUB_PATH}" || true
    fi

    if [ -f .gitmodules ] && [ ! -s .gitmodules ]; then
        rm -f .gitmodules
    fi
}

if has_gitlink; then
    echo "[*] submodule gitlink already present"
    git submodule sync --recursive -- "$SUB_PATH"
    git submodule update --init --recursive -- "$SUB_PATH"
    echo "[*] done"
    exit 0
fi

if section_exists; then
    echo "[*] .gitmodules entry exists, but gitlink is missing; repairing"
    remove_stale_module_state
fi

if [ -e "$SUB_PATH" ]; then
    if [ -d "$SUB_PATH" ] && [ -z "$(find "$SUB_PATH" -mindepth 1 -maxdepth 1 -print -quit 2>/dev/null)" ]; then
        rmdir "$SUB_PATH"
    else
        echo "error: $SUB_PATH exists but is not a registered submodule." >&2
        echo "       Move it away first, then rerun this script:" >&2
        echo "       mv '$SUB_PATH' '${SUB_PATH}.bak'" >&2
        exit 1
    fi
fi

mkdir -p "$(dirname "$SUB_PATH")"

echo "[*] adding submodule: $SUB_URL -> $SUB_PATH"
git submodule add -f -b "$SUB_BRANCH" "$SUB_URL" "$SUB_PATH"

echo "[*] updating submodule"
git submodule update --init --recursive -- "$SUB_PATH"

echo "[*] done"
