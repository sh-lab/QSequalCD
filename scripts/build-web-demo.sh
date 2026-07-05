#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WEB_DIR="${ROOT_DIR}/apps/web-demo"
SITE_DIR="${ROOT_DIR}/site"

"${ROOT_DIR}/scripts/build-wasm.sh"

required_files=(
  "${WEB_DIR}/index.html"
  "${WEB_DIR}/app.js"
  "${WEB_DIR}/style.css"
  "${WEB_DIR}/wasm/qscd_wasm.js"
  "${WEB_DIR}/wasm/qscd_wasm.wasm"
)

for file in "${required_files[@]}"; do
  if [ ! -f "${file}" ]; then
    echo "Required web demo file is missing: ${file}" >&2
    exit 1
  fi
done

rm -rf "${SITE_DIR}"
mkdir -p "${SITE_DIR}/wasm"

cp "${WEB_DIR}/index.html" "${SITE_DIR}/index.html"
cp "${WEB_DIR}/app.js" "${SITE_DIR}/app.js"
cp "${WEB_DIR}/style.css" "${SITE_DIR}/style.css"
cp "${WEB_DIR}/wasm/qscd_wasm.js" "${SITE_DIR}/wasm/qscd_wasm.js"
cp "${WEB_DIR}/wasm/qscd_wasm.wasm" "${SITE_DIR}/wasm/qscd_wasm.wasm"
touch "${SITE_DIR}/.nojekyll"

site_required_files=(
  "${SITE_DIR}/index.html"
  "${SITE_DIR}/app.js"
  "${SITE_DIR}/style.css"
  "${SITE_DIR}/wasm/qscd_wasm.js"
  "${SITE_DIR}/wasm/qscd_wasm.wasm"
  "${SITE_DIR}/.nojekyll"
)

for file in "${site_required_files[@]}"; do
  if [ ! -f "${file}" ]; then
    echo "Required site file is missing: ${file}" >&2
    exit 1
  fi
done

echo "Web demo site is ready at ${SITE_DIR}."
echo "Run locally with: cd ${SITE_DIR} && python3 -m http.server"
