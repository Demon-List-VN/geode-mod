#!/usr/bin/env bash
set -euo pipefail

if ! command -v clang-tidy >/dev/null 2>&1; then
    echo "clang-tidy is not installed or not on PATH." >&2
    exit 127
fi

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${1:-}"

if [ -n "$build_dir" ] && [ "${build_dir#/}" = "$build_dir" ]; then
    build_dir="$PWD/$build_dir"
fi

cd "$repo_root"

if [ -z "$build_dir" ]; then
    for candidate in cmake-build-debug cmake-build-relwithdebinfo cmake-build-release build out/build; do
        if [ -f "$candidate/compile_commands.json" ]; then
            build_dir="$candidate"
            break
        fi
    done
fi

if [ -z "$build_dir" ] || [ ! -f "$build_dir/compile_commands.json" ]; then
    echo "No compile_commands.json found. Pass the CMake build directory as the first argument." >&2
    exit 1
fi

find src -type f \( -name '*.cpp' -o -name '*.cc' -o -name '*.cxx' \) -print0 \
    | xargs -0 clang-tidy -p "$build_dir" -header-filter="$repo_root/src/.*"
