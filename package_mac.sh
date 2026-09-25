#!/bin/bash
set -e

# ============================================================
# MemoraDB macOS Packaging Script
# Creates a native MemoraDB.app bundle and packs it into a
# drag-and-drop Apple Disk Image (.dmg) and .zip bundle
# ============================================================

VERSION="1.0.0"
echo "=== Packaging MemoraDB for macOS (v${VERSION}) ==="

# Build if release directory doesn't exist
if [ ! -f "build/release/memora" ]; then
    echo "Building MemoraDB Release..."
    cmake -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
    cmake --build build --config Release
fi

rm -rf dist
APP_NAME="MemoraDB.app"
APP_DIR="dist/MemoraDB.app"
CONTENTS_DIR="${APP_DIR}/Contents"
MACOS_DIR="${CONTENTS_DIR}/MacOS"
RESOURCES_DIR="${CONTENTS_DIR}/Resources"

mkdir -p "${MACOS_DIR}"
mkdir -p "${RESOURCES_DIR}"

echo "Creating native ${APP_NAME} bundle..."

# Copy binary & dynamic libraries into Contents/MacOS/
cp build/release/memora "${MACOS_DIR}/memora"
chmod +x "${MACOS_DIR}/memora"
cp -P build/release/libonnxruntime*.dylib "${MACOS_DIR}/" 2>/dev/null || true

# Copy models into Contents/MacOS/models/
mkdir -p "${MACOS_DIR}/models"
cp -R build/release/models/* "${MACOS_DIR}/models/"

# Convert MemoraDB.png into native macOS AppIcon.icns
if [ -f "MemoraDB.png" ]; then
    echo "Generating native macOS AppIcon.icns..."
    ICONSET="dist/MemoraDB.iconset"
    mkdir -p "${ICONSET}"
    sips -z 16 16     MemoraDB.png --out "${ICONSET}/icon_16x16.png" > /dev/null 2>&1 || true
    sips -z 32 32     MemoraDB.png --out "${ICONSET}/icon_16x16@2x.png" > /dev/null 2>&1 || true
    sips -z 32 32     MemoraDB.png --out "${ICONSET}/icon_32x32.png" > /dev/null 2>&1 || true
    sips -z 64 64     MemoraDB.png --out "${ICONSET}/icon_32x32@2x.png" > /dev/null 2>&1 || true
    sips -z 128 128   MemoraDB.png --out "${ICONSET}/icon_128x128.png" > /dev/null 2>&1 || true
    sips -z 256 256   MemoraDB.png --out "${ICONSET}/icon_128x128@2x.png" > /dev/null 2>&1 || true
    sips -z 256 256   MemoraDB.png --out "${ICONSET}/icon_256x256.png" > /dev/null 2>&1 || true
    sips -z 512 512   MemoraDB.png --out "${ICONSET}/icon_256x256@2x.png" > /dev/null 2>&1 || true
    sips -z 512 512   MemoraDB.png --out "${ICONSET}/icon_512x512.png" > /dev/null 2>&1 || true
    sips -z 1024 1024 MemoraDB.png --out "${ICONSET}/icon_512x512@2x.png" > /dev/null 2>&1 || true
    iconutil -c icns "${ICONSET}" -o "${RESOURCES_DIR}/AppIcon.icns" > /dev/null 2>&1 || true
    rm -rf "${ICONSET}"
fi

# Create launcher script that opens Terminal with memora
cat << 'EOF' > "${MACOS_DIR}/memora-launcher"
#!/bin/bash
DIR="$(cd "$(dirname "$0")" && pwd)"
osascript -e "tell application \"Terminal\" to do script \"\\\"$DIR/memora\\\"\"" -e "tell application \"Terminal\" to activate"
EOF
chmod +x "${MACOS_DIR}/memora-launcher"

# Create Info.plist for macOS bundle identification
cat << EOF > "${CONTENTS_DIR}/Info.plist"
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleDevelopmentRegion</key>
    <string>en</string>
    <key>CFBundleDisplayName</key>
    <string>MemoraDB</string>
    <key>CFBundleExecutable</key>
    <string>memora-launcher</string>
    <key>CFBundleIconFile</key>
    <string>AppIcon</string>
    <key>CFBundleIdentifier</key>
    <string>com.vihaanjain.memoradb</string>
    <key>CFBundleInfoDictionaryVersion</key>
    <string>6.0</string>
    <key>CFBundleName</key>
    <string>MemoraDB</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleShortVersionString</key>
    <string>${VERSION}</string>
    <key>CFBundleVersion</key>
    <string>${VERSION}</string>
    <key>LSMinimumSystemVersion</key>
    <string>11.0</string>
    <key>NSHighResolutionCapable</key>
    <true/>
</dict>
</plist>
EOF

echo "Staging drag-and-drop DMG folder..."
STAGING_DIR="dist/dmg_staging"
mkdir -p "${STAGING_DIR}"
cp -R "${APP_DIR}" "${STAGING_DIR}/"
ln -s /Applications "${STAGING_DIR}/Applications"

# Add README for users
cat << EOF > "${STAGING_DIR}/README.txt"
MemoraDB v${VERSION} for macOS
==============================
A C++ append-only temporal database with semantic vector search.

Installation:
- Drag "MemoraDB.app" into "Applications".
- Double-click MemoraDB in Applications (or Spotlight) to launch the database interactive shell!

Command Line Terminal Usage:
/Applications/MemoraDB.app/Contents/MacOS/memora
EOF

echo "Creating Apple Disk Image (.dmg)..."
DMG_NAME="MemoraDB-Setup-macOS.dmg"
rm -f "${DMG_NAME}"
hdiutil create -volname "MemoraDB" -srcfolder "${STAGING_DIR}" -ov -format UDZO "${DMG_NAME}"

echo "Creating portable ZIP archive of MemoraDB.app..."
ZIP_NAME="MemoraDB-macOS.zip"
rm -f "${ZIP_NAME}"
(cd dist && zip -r "../${ZIP_NAME}" "MemoraDB.app")

echo "=== macOS Packaging Complete! ==="
echo "Artifacts generated:"
echo "  - ${DMG_NAME} (Drag-and-Drop Disk Image containing MemoraDB.app -> /Applications)"
echo "  - ${ZIP_NAME} (Portable standalone MemoraDB.app bundle)"
