#!/usr/bin/env bash
set -euo pipefail

VERSION="${1:-0.1.0}"
RUNTIME_ID="${2:-}"

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
DIST_DIR="${ROOT_DIR}/dist"
PACKAGE_NAME="oaflang-${VERSION}"
STAGING_DIR="${DIST_DIR}/${PACKAGE_NAME}"

echo "Creating release package '${PACKAGE_NAME}'..."
rm -rf "${STAGING_DIR}"
mkdir -p "${STAGING_DIR}/bin"

PUBLISH_ARGS=(
  "${ROOT_DIR}/OafLang.csproj"
  "--configuration" "Release"
  "--output" "${STAGING_DIR}/bin"
)

if [[ -n "${RUNTIME_ID}" ]]; then
  PUBLISH_ARGS+=("--runtime" "${RUNTIME_ID}" "--self-contained" "false")
fi

dotnet publish "${PUBLISH_ARGS[@]}"

for path in docs examples SpecOverview.md SpecSyntax.md SpecRuntime.md SpecFileStructure.md SpecRoadmap.md; do
  if [[ -e "${ROOT_DIR}/${path}" ]]; then
    cp -R "${ROOT_DIR}/${path}" "${STAGING_DIR}/"
  fi
done

cat > "${STAGING_DIR}/README.txt" <<EOF
OafLang Release Package ${VERSION}

Contents:
- bin/: Published CLI and runtime assets
- docs/: Guides and references
- examples/: Sample programs and tutorials
- Spec*.md: Language specification documents

Quick start:
1. Run 'dotnet --info' to verify your local SDK/runtime.
2. Execute './bin/OafLang --self-test' to validate installation.
3. Compile source: './bin/OafLang "flux x = 1; return x;" --bytecode'
EOF

mkdir -p "${DIST_DIR}"
TAR_PATH="${DIST_DIR}/${PACKAGE_NAME}.tar.gz"
ZIP_PATH="${DIST_DIR}/${PACKAGE_NAME}.zip"

rm -f "${TAR_PATH}" "${ZIP_PATH}"
tar -czf "${TAR_PATH}" -C "${DIST_DIR}" "${PACKAGE_NAME}"

if command -v zip >/dev/null 2>&1; then
  (
    cd "${DIST_DIR}"
    zip -rq "${ZIP_PATH}" "${PACKAGE_NAME}"
  )
fi

echo "Package created:"
echo "  ${TAR_PATH}"
if [[ -f "${ZIP_PATH}" ]]; then
  echo "  ${ZIP_PATH}"
fi
