#!/bin/bash
#
# Build script for Tera Term on macOS (ARM64 / Apple Silicon)
#
# Usage:
#   ./build_macos.sh              # Build for native architecture
#   ./build_macos.sh arm64        # Build for ARM64
#   ./build_macos.sh x86_64       # Build for Intel x86_64
#   ./build_macos.sh universal    # Build Universal Binary (ARM64 + x86_64)
#
# Requirements:
#   - macOS 11.0+ (Big Sur or later)
#   - Xcode Command Line Tools (xcode-select --install)
#   - CMake 3.11+ (brew install cmake)
#   - Perl (included with macOS)
#

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ARCH="${1:-arm64}"
BUILD_DIR="${SCRIPT_DIR}/build_macos_${ARCH}"
BUILD_TYPE="${2:-Release}"

echo "=== Tera Term macOS Build ==="
echo "Architecture: ${ARCH}"
echo "Build type:   ${BUILD_TYPE}"
echo "Build dir:    ${BUILD_DIR}"
echo ""

# Check prerequisites
if ! command -v cmake &> /dev/null; then
    echo "ERROR: cmake not found. Install with: brew install cmake"
    exit 1
fi

if ! command -v clang &> /dev/null; then
    echo "ERROR: clang not found. Install Xcode Command Line Tools:"
    echo "  xcode-select --install"
    exit 1
fi

# Create build directory
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# Configure
echo "--- Configuring ---"
cmake "${SCRIPT_DIR}" \
    -G "Unix Makefiles" \
    -DARCHITECTURE="${ARCH}" \
    -DCMAKE_TOOLCHAIN_FILE="${SCRIPT_DIR}/macos.toolchain.cmake" \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DTTXSSH=OFF \
    -DENABLE_TTXSAMPLES=OFF

# Build
echo ""
echo "--- Building ---"
cmake --build . -- -j$(sysctl -n hw.ncpu)

echo ""
echo "=== Build complete ==="
echo "Output: ${BUILD_DIR}/TeraTerm.app"
