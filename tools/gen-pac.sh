#!/usr/bin/env bash
# Generate a Rust PAC (peripheral access crate) for the VORAGO VA41620.
#
# There is no public VA41620 SVD on GitHub. Get VA416xx.svd from the VORAGO
# device family pack (vendor download), then:
#
#   ./tools/gen-pac.sh /path/to/VA416xx.svd
#
# Output: rust/va41620-pac/ (lib.rs split into src/ by `form`).
# This is optional: the F´ C++ build uses the vendor headers instead.
set -euo pipefail

usage() {
    cat <<'EOF'
usage: gen-pac.sh <VA416xx.svd> [output-dir]

  <VA416xx.svd>   SVD shipped in the VORAGO VA416xx DFP
  [output-dir]    default: rust/va41620-pac

Requires: svd2rust (cargo install svd2rust --locked) and optionally
          form (cargo install form --locked).
EOF
}

if [[ $# -lt 1 ]]; then
    usage
    exit 2
fi

svd=$(realpath "$1")
[[ -f $svd ]] || { echo "error: SVD not found: $svd" >&2; exit 1; }

root=$(cd "$(dirname "$0")/.." && pwd)
out=${2:-$root/rust/va41620-pac}
mkdir -p "$out/src"

if ! command -v svd2rust >/dev/null; then
    echo "error: svd2rust not installed" >&2
    echo "       cargo install svd2rust --locked" >&2
    exit 1
fi

echo "==> generating $out from $(basename "$svd")"
(
    cd "$out"
    svd2rust -i "$svd" --target cortex-m
    if command -v form >/dev/null; then
        form -i lib.rs -o src/ && rm -f lib.rs
    else
        echo "note: 'form' not found; leaving a single lib.rs"
        echo "      cargo install form --locked  (then re-run)"
    fi
    cargo fmt 2>/dev/null || true
)

if [[ ! -f $out/Cargo.toml ]]; then
    cat >"$out/Cargo.toml" <<'EOF'
[package]
name = "va41620-pac"
version = "0.1.0"
edition = "2021"
description = "Generated register map for the VORAGO VA41620"

[dependencies]
cortex-m = "0.7"
vcell = "0.1"
EOF
fi

echo "==> done"
echo "    review $out/src before trusting reset values or clock gates."
