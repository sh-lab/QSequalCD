#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build/wasm"
OUTPUT_DIR="${ROOT_DIR}/apps/web-demo/wasm"

if ! command -v emcmake >/dev/null 2>&1; then
  emsdk_env_candidates=()
  if [ -n "${EMSDK:-}" ]; then
    emsdk_env_candidates+=("${EMSDK}/emsdk_env.sh")
  fi
  emsdk_env_candidates+=(
    "${ROOT_DIR}/../emsdk/emsdk_env.sh"
    "${HOME}/emsdk/emsdk_env.sh"
    "${HOME}/Projects/emsdk/emsdk_env.sh"
  )

  for emsdk_env in "${emsdk_env_candidates[@]}"; do
    if [ -f "${emsdk_env}" ]; then
      EMSDK_QUIET=1 source "${emsdk_env}"
      break
    fi
  done
fi

if ! command -v emcmake >/dev/null 2>&1; then
  echo "emcmake was not found. Install and activate Emscripten before building Wasm." >&2
  exit 1
fi

cmake_args=(
  -S "${ROOT_DIR}/bindings/wasm"
  -B "${BUILD_DIR}"
  -DCMAKE_BUILD_TYPE=Release
)

emcmake cmake "${cmake_args[@]}"
cmake --build "${BUILD_DIR}" --target qscd_wasm

mkdir -p "${OUTPUT_DIR}"
cp "${BUILD_DIR}/qscd_wasm.js" "${OUTPUT_DIR}/qscd_wasm.js"
cp "${BUILD_DIR}/qscd_wasm.wasm" "${OUTPUT_DIR}/qscd_wasm.wasm"

echo "Wasm artifacts copied to ${OUTPUT_DIR}"
