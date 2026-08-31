#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
GACUI_DIR="$SCRIPT_DIR/../GacUI"
SOURCE_IMPORT_DIR="$GACUI_DIR/Import"
SOURCE_RELEASE_DIR="$GACUI_DIR/Release"
DEST_DIR="$SCRIPT_DIR/Import"
DEST_TEST_DIR="$SCRIPT_DIR/Import-Test"

if [[ ! -d "$SOURCE_IMPORT_DIR" ]]; then
    echo "GacUI dependency imports not found: $SOURCE_IMPORT_DIR" >&2
    exit 1
fi

if [[ ! -d "$SOURCE_RELEASE_DIR" ]]; then
    echo "GacUI release sources not found: $SOURCE_RELEASE_DIR" >&2
    exit 1
fi

if ! command -v perl >/dev/null 2>&1; then
    echo "Perl is required to normalize imported source line endings." >&2
    exit 1
fi

for dest_dir in "$DEST_DIR" "$DEST_TEST_DIR"; do
    if [[ -e "$dest_dir" ]]; then
        chmod -R u+w "$dest_dir"
    fi
done

rm -rf "$DEST_DIR" "$DEST_TEST_DIR"
mkdir -p "$DEST_DIR/Skins/DarkSkin"
mkdir -p "$DEST_TEST_DIR"

cp -R "$SOURCE_IMPORT_DIR/." "$DEST_DIR/"

# GacUI normalizes dependency imports through Git, but generated working-tree
# copies can still use CRLF. Keep wGac's dependency snapshot deterministically
# LF so rerunning this importer does not rewrite otherwise unchanged files.
find "$DEST_DIR" -type f -exec perl -pi -e 's/\r\n/\n/g' {} +

cp "$SOURCE_RELEASE_DIR"/GacUI*.h "$DEST_DIR/"
cp "$SOURCE_RELEASE_DIR"/GacUI*.cpp "$DEST_DIR/"
cp "$SOURCE_RELEASE_DIR"/DarkSkin*.h "$DEST_DIR/Skins/DarkSkin/"
cp "$SOURCE_RELEASE_DIR"/DarkSkin*.cpp "$DEST_DIR/Skins/DarkSkin/"
cp "$SOURCE_RELEASE_DIR/Test.RemotingHelpers.h" "$DEST_DIR/"
cp "$SOURCE_RELEASE_DIR/Test.RemotingHelpers.cpp" "$DEST_DIR/"

# Release/CodePack outputs are stored as CRLF in wGac. Normalize them as well,
# independent of the source checkout's current Git EOL conversion.
perl -pi -e 's/\r?\n/\r\n/g' \
    "$DEST_DIR"/GacUI*.h \
    "$DEST_DIR"/GacUI*.cpp \
    "$DEST_DIR"/Skins/DarkSkin/DarkSkin*.h \
    "$DEST_DIR"/Skins/DarkSkin/DarkSkin*.cpp \
    "$DEST_DIR"/Test.RemotingHelpers.h \
    "$DEST_DIR"/Test.RemotingHelpers.cpp

# These two release artifacts are stored as LF in wGac.
perl -pi -e 's/\r\n/\n/g' \
    "$DEST_DIR"/GacUICompiler.h \
    "$DEST_DIR"/GacUICompiler.cpp

mv "$DEST_DIR/Test.RemotingHelpers.h" "$DEST_TEST_DIR/"
mv "$DEST_DIR/Test.RemotingHelpers.cpp" "$DEST_TEST_DIR/"

chmod -R a-w "$DEST_DIR"
chmod -R a-w "$DEST_TEST_DIR"

echo "Imported GacUI dependencies and release sources from $GACUI_DIR to $DEST_DIR and $DEST_TEST_DIR"
