#!/usr/bin/env sh

INSTALL_DIR="/opt/ndi_recorder"
BIN_DIR="$INSTALL_DIR/bin"
ASSETS_DIR="$INSTALL_DIR/assets"
LIB_DIR="/usr/lib"

if [ ! -d "$INSTALL_DIR" ]; then
  mkdir "$INSTALL_DIR"
fi

if [ ! -d "$LIB_DIR" ]; then
  mkdir "$LIB_DIR"
fi

if [ ! -d "$BIN_DIR" ]; then
  mkdir "$BIN_DIR"
fi

if [ ! -d "$ASSETS_DIR" ]; then
  mkdir "$ASSETS_DIR"
fi


cp lib/* "$LIB_DIR"
cp bin/* "$BIN_DIR"

cp build/ndi_recorder "$BIN_DIR"

cp assets/* "$ASSETS_DIR"

chmod +x "$BIN_DIR/ndi_recorder"

#symlink to the /usr/bin directory
ln -s "$BIN_DIR/ndi_recorder" /usr/bin/