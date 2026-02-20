#!/bin/bash
# Script to create a binary image file from multiple sources
# Output file: out.img (16 MB, zero-filled with specific data copied at offsets)

set -e  # Exit on error

OUTPUT_FILE="out.img"
IMAGE_SIZE=$((16 * 1024 * 1024))  # 16 MB in bytes

echo "Creating $OUTPUT_FILE with size $IMAGE_SIZE bytes (16 MB)..."

# Create a 16 MB file filled with zeros
dd if=/dev/zero of="$OUTPUT_FILE" bs=1M count=16 status=progress

echo ""
echo "Copying data chunks to specific offsets..."

# Function to copy bytes from source+offset to destination+offset
# Usage: copy_bytes <source_file> <source_offset> <dest_offset> <num_bytes>
copy_bytes() {
    local src_file="$1"
    local src_offset="$2"
    local dst_offset="$3"
    local num_bytes="$4"
    
    if [ ! -f "$src_file" ]; then
        echo "Error: Source file '$src_file' not found!"
        exit 1
    fi
    
    echo "  Copying $num_bytes bytes from $src_file+$src_offset to $OUTPUT_FILE+$dst_offset"
    dd if="$src_file" of="$OUTPUT_FILE" \
       bs=1 skip="$src_offset" seek="$dst_offset" count="$num_bytes" \
       conv=notrunc status=none
}

# Function to copy entire file to destination at offset
# Usage: copy_file <source_file> <dest_offset>
copy_file() {
    local src_file="$1"
    local dst_offset="$2"
    
    if [ ! -f "$src_file" ]; then
        echo "Error: Source file '$src_file' not found!"
        exit 1
    fi
    
    local file_size=$(stat -c%s "$src_file")
    echo "  Copying entire file $src_file ($file_size bytes) to $OUTPUT_FILE+$dst_offset"
    dd if="$src_file" of="$OUTPUT_FILE" \
       bs=1 seek="$dst_offset" \
       conv=notrunc status=none
}

# Copy 512-byte chunks at specific offsets
copy_bytes "stick16.img" 0 0 512
copy_bytes "stick16.img" 1048576 1048576 512
copy_bytes "stick16.img" 1050624 1050624 512
copy_bytes "stick16.img" 1067008 1067008 512
copy_bytes "stick16.img" 1083392 1083392 512

# Copy entire files
copy_file "IMG1.JPG" 1101824
copy_file "IMG2.JPG" 1232896

echo ""
echo "Done! Created $OUTPUT_FILE"
echo ""

# Show file size
ls -lh "$OUTPUT_FILE"

# Optional: Show hex dump of first 512 bytes (MBR)
echo ""
echo "First 32 bytes of $OUTPUT_FILE (MBR start):"
hexdump -C "$OUTPUT_FILE" | head -n 2

echo ""
echo "Summary of operations:"
echo "  - MBR at offset 0 (512 bytes from stick16.bin)"
echo "  - VBR at offset 1048576 (512 bytes from stick16.img)"
echo "  - Sector at offset 1050624 (512 bytes from stick16.img)"
echo "  - Sector at offset 1067008 (512 bytes from stick16.img)"
echo "  - Root dir at offset 1083392 (512 bytes from stick16.img)"
echo "  - IMG1.JPG at offset 1101824"
echo "  - IMG2.JPG at offset 1232896"
