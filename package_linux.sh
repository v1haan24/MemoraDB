#!/bin/bash
set -e

# ============================================================
# MemoraDB Linux Packaging Script
# Creates a standalone AppImage and portable tar.gz bundle
# ============================================================

VERSION="1.0.0"
echo "=== Packaging MemoraDB for Linux (v${VERSION}) ==="

# Build if release directory doesn't exist
if [ ! -f "build/release/memora" ]; then
    echo "Building MemoraDB Release..."
    cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release
fi

APP_DIR="dist/MemoraDB.AppDir"
rm -rf dist
mkdir -p "${APP_DIR}/usr/bin"
mkdir -p "${APP_DIR}/usr/lib"
mkdir -p "${APP_DIR}/usr/bin/models"

echo "Staging files into AppDir..."
cp build/release/memora "${APP_DIR}/usr/bin/"
chmod +x "${APP_DIR}/usr/bin/memora"

# Copy ONNX runtime shared objects (dereference symlinks and ensure real files)
cp -L build/release/libonnxruntime*.so* "${APP_DIR}/usr/lib/" 2>/dev/null || true
cp -L build/release/libonnxruntime*.so* "${APP_DIR}/usr/bin/" 2>/dev/null || true
find build -name "libonnxruntime*.so*" -exec cp -L {} "${APP_DIR}/usr/lib/" \; 2>/dev/null || true
find build -name "libonnxruntime*.so*" -exec cp -L {} "${APP_DIR}/usr/bin/" \; 2>/dev/null || true

# Explicitly ensure libonnxruntime.so.1 exists in both locations
for d in "${APP_DIR}/usr/lib" "${APP_DIR}/usr/bin"; do
    if [ ! -f "$d/libonnxruntime.so.1" ]; then
        for f in "$d"/libonnxruntime.so*; do
            if [ -f "$f" ]; then
                cp -a "$f" "$d/libonnxruntime.so.1"
                break
            fi
        done
    fi
done

# Copy models
cp -R build/release/models/* "${APP_DIR}/usr/bin/models/"

# Desktop integration files
if [ -f "MemoraDB.png" ]; then
    cp MemoraDB.png "${APP_DIR}/memora.png"
    cp MemoraDB.png "${APP_DIR}/.DirIcon"
fi

cat << 'EOF' > "${APP_DIR}/memora.desktop"
[Desktop Entry]
Name=MemoraDB
Exec=memora
Icon=memora
Type=Application
Categories=Development;Database;
Terminal=true
Comment=Append-only temporal database with semantic vector search
EOF

# AppRun entry point
cat << 'EOF' > "${APP_DIR}/AppRun"
#!/bin/bash
HERE="$(dirname "$(readlink -f "${0}")")"
export LD_LIBRARY_PATH="${HERE}/usr/lib:${HERE}/usr/bin:${LD_LIBRARY_PATH}"

# Remain in user's working directory so data/ can be created
if [ -n "$OWD" ]; then
    cd "$OWD"
fi

exec "${HERE}/usr/bin/memora" "$@"
EOF
chmod +x "${APP_DIR}/AppRun"

# Build AppImage
echo "Downloading appimagetool..."
APPIMAGETOOL="dist/appimagetool"
if [ ! -f "${APPIMAGETOOL}" ]; then
    curl -fSL -o "${APPIMAGETOOL}" "https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage"
    chmod +x "${APPIMAGETOOL}"
fi

echo "Extracting appimagetool for FUSE-free execution..."
(cd dist && ./appimagetool --appimage-extract > /dev/null 2>&1)
APPIMAGETOOL_BIN="dist/squashfs-root/AppRun"

echo "Building AppImage package..."
APPIMAGE_NAME="MemoraDB-Setup-Linux-x86_64.AppImage"
rm -f "${APPIMAGE_NAME}"
ARCH=x86_64 "${APPIMAGETOOL_BIN}" "${APP_DIR}" "${APPIMAGE_NAME}"

# Also build portable tar.gz bundle for servers/headless environments
echo "Building portable TAR.GZ archive..."
PORTABLE_DIR="dist/MemoraDB-Linux-x86_64"
mkdir -p "${PORTABLE_DIR}"
cp -R "${APP_DIR}/usr/bin/"* "${PORTABLE_DIR}/"

cat << 'EOF' > "${PORTABLE_DIR}/run.sh"
#!/bin/bash
DIR="$(cd "$(dirname "$0")" && pwd)"
export LD_LIBRARY_PATH="${DIR}:${DIR}/../lib:${LD_LIBRARY_PATH}"
exec "${DIR}/memora" "$@"
EOF
chmod +x "${PORTABLE_DIR}/run.sh"

TAR_NAME="MemoraDB-Linux-x86_64.tar.gz"
rm -f "${TAR_NAME}"
tar -czvf "${TAR_NAME}" -C dist "MemoraDB-Linux-x86_64"

echo "=== Linux Packaging Complete! ==="
echo "Artifacts generated:"
echo "  - ${APPIMAGE_NAME}"
echo "  - ${TAR_NAME}"
