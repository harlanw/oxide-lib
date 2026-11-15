#!/usr/bin/env bash
set -euo pipefail

root_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
build_dir="$root_dir/.build"

cmake -S "$root_dir" -B "$build_dir" "$@"

cat > "$root_dir/.clangd" <<'EOF'
CompileFlags:
  CompilationDatabase: .build
EOF

cmake --build "$build_dir" --parallel
