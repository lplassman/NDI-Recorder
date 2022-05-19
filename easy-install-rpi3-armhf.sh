#!/usr/bin/env sh

set -eu

echo "Starting install of NDI Recorder for Raspberry Pi 3 32-bit..."

echo "Installing prerequisites..."
sudo bash ./preinstall.sh

echo "Downloading NDI libraries..."
sudo bash ./download_NDI_SDK.sh

echo "Building executable for Raspberry Pi 3 32-bit..."
sudo bash ./build_rpi3_armhf.sh

echo "Installing in final directory..."
sudo bash ./install.sh

echo "Done"
