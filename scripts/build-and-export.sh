#!/bin/bash
# Builds firmware and copies .bin files next to the wizard for web flashing
# Run this once (developer/manufacturer step, not the end user)

set -e
cd "$(dirname "$0")/.."

echo "Building firmware..."
cd firmware
pio run --target build

echo "Copying binaries..."
BUILD_DIR=".pio/build/esp32-s3-devkitc-1"
mkdir -p ../flash-bin
cp "$BUILD_DIR/firmware.bin" ../flash-bin/
cp "$BUILD_DIR/bootloader.bin" ../flash-bin/
cp "$BUILD_DIR/partitions.bin" ../flash-bin/

# Also copy LittleFS image (includes wifi_config.json + web dashboard)
cp "$BUILD_DIR/littlefs.bin" ../flash-bin/

echo "Done! Binaries in flash-bin/"
echo "Now open wizard.html in a browser to flash via USB."
