#!/usr/bin/env bash

set -euo pipefail
IFS=$'\n\t'

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)

if ! source /etc/os-release; then
	echo "Could not read /etc/os-release" >&2
	exit 1
fi

if [ "${VERSION_CODENAME:-}" != trixie ]; then
	echo "build-deb.sh only supports Debian trixie (native VM build). Found: ${VERSION_CODENAME:-unknown}" >&2
	exit 1
fi

ARTIFACTS_DIR="${HOME}/linamp-debs"

echo "Installing packaging tools..."
export DEBIAN_FRONTEND=noninteractive
sudo apt-get update
sudo apt-get install --no-install-recommends -y \
	debhelper \
	devscripts \
	dh-cmake \
	dh-python \
	dpkg-dev \
	fakeroot \
	rsync

BUILD_DIR=$(mktemp -d --tmpdir linamp-deb-build-XXXXXX)
cleanup() {
	rm -rf -- "$BUILD_DIR"
}
trap cleanup EXIT

echo "Copying sources to ${BUILD_DIR}..."
# Omit local dev CMake output (paths in CMakeCache.txt point at the checkout dir).
rsync -a \
	--exclude build/ \
	--exclude venv/ \
	--exclude .git/ \
	--exclude __pycache__/ \
	--exclude CMakeFiles/ \
	--exclude CMakeCache.txt \
	--exclude CMakeCache.txt.prev \
	--exclude cmake_install.cmake \
	--exclude Makefile \
	--exclude player_autogen/ \
	--exclude Testing/ \
	--exclude .cmake/ \
	--exclude .qt/ \
	--exclude .rcc/ \
	--exclude 'obj-*' \
	"${SCRIPT_DIR}/" "${BUILD_DIR}/"

cd "${BUILD_DIR}"
sed -i -E '1 s/\(([^)]+)\) [^;]+;/\(\1~trixie1\) trixie;/' debian/changelog

echo "Installing build dependencies from debian/control (first run may take a while)..."
sudo apt-get build-dep --no-install-recommends -y .

echo "Building Debian package..."
dpkg-buildpackage -us -uc

PARENT_DIR=$(dirname -- "${BUILD_DIR}")
mkdir -p "${ARTIFACTS_DIR}"
shopt -s nullglob
debs=("${PARENT_DIR}"/linamp_*.deb)
if [ "${#debs[@]}" -eq 0 ]; then
	echo "No .deb files found in ${PARENT_DIR}" >&2
	exit 1
fi

for deb in "${debs[@]}"; do
	cp -- "${deb}" "${ARTIFACTS_DIR}/"
done

echo "Built packages:"
ls -la "${ARTIFACTS_DIR}"/linamp_*.deb
echo "Install with: sudo apt install ${ARTIFACTS_DIR}/linamp_*.deb"
