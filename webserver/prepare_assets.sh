#!/usr/bin/env bash
# Resize and convert all images in assets/ to 800x600 JPEG, quality 65.
# Images are cropped (center) if the aspect ratio does not match.
# Output overwrites the original file (always saved as .jpg).

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ASSETS_DIR="$SCRIPT_DIR/assets"
TARGET_W=800
TARGET_H=600
QUALITY=65
MAX_BYTES=$((128 * 1024))

if ! command -v convert &>/dev/null; then
    echo "ERROR: ImageMagick 'convert' not found. Install with: sudo apt install imagemagick"
    exit 1
fi

shopt -s nullglob nocaseglob
files=("$ASSETS_DIR"/*.{jpg,jpeg,png,gif,bmp,webp,tiff,tif})
shopt -u nullglob nocaseglob

if [[ ${#files[@]} -eq 0 ]]; then
    echo "No image files found in $ASSETS_DIR"
    exit 0
fi

echo "Processing ${#files[@]} file(s) in $ASSETS_DIR"
echo

for src in "${files[@]}"; do
    filename="$(basename "$src")"
    name="${filename%.*}"
    dst="$ASSETS_DIR/${name}.jpg"

    size_before=$(wc -c < "$src")

    # Resize to fill 800x600 (^ = cover), center-crop to exact size, strip metadata
    convert "$src" \
        -resize "${TARGET_W}x${TARGET_H}^" \
        -gravity Center \
        -extent "${TARGET_W}x${TARGET_H}" \
        -quality "$QUALITY" \
        -strip \
        "$dst"

    # Remove original if it was a different format
    if [[ "$src" != "$dst" ]]; then
        rm "$src"
    fi

    size_after=$(wc -c < "$dst")

    status="OK"
    if [[ $size_after -gt $MAX_BYTES ]]; then
        status="WARNING: ${size_after} bytes exceeds 128 KB"
    fi

    printf "  %-30s  %6d KB  ->  %6d KB  %s\n" \
        "$filename" \
        $(( size_before / 1024 )) \
        $(( size_after  / 1024 )) \
        "$status"
done

echo
echo "Done."
