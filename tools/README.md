# Disk Image Analysis Tools

Four CLI tools for reading and decoding boot sectors, FAT tables, and directory entries from binary disk images.

## Tools

### 1. read_mbr
Reads and decodes a Master Boot Record (MBR) from a binary file.

**Usage:**
```bash
./read_mbr <binary_file> <offset>
```

**Example:**
```bash
./read_mbr disk.img 0
```

The MBR is typically located at offset 0 (the first 512 bytes of the disk).

**Output includes:**
- Boot code (first 16 bytes shown)
- Partition table entries (all 4 partitions)
  - Boot indicator (bootable/non-bootable)
  - CHS addressing (head, sector, cylinder)
  - Partition type (with descriptive names)
  - LBA start and size
  - Human-readable size in MB
- Boot signature validation (should be 0xAA55)

---

### 2. read_vbr
Reads and decodes a Volume Boot Record (VBR) from a binary file.

**Usage:**
```bash
./read_vbr <binary_file> <offset>
```

**Example:**
```bash
./read_vbr disk.img 1048576  # Read VBR at 1MB offset (LBA 2048)
```

The VBR is typically located at the start of a partition. To find the offset:
- Read the MBR to find the partition's starting LBA
- Multiply the LBA by 512 to get the byte offset

**Output includes:**
- Jump instruction
- OEM identifier
- BIOS Parameter Block (BPB):
  - Bytes per sector
  - Sectors per cluster (and cluster size)
  - Reserved sectors
  - Number of FATs
  - Root directory entries
  - Total sectors (16-bit and 32-bit)
  - Media descriptor
  - FAT size
  - CHS geometry
  - Hidden sectors
- Extended Boot Record (EBR):
  - File system type (FAT16/FAT32)
  - FAT32-specific fields (if applicable)
  - Drive number
  - Volume serial number
  - Volume label
- Boot code (first 16 bytes shown)
- Boot signature validation (should be 0xAA55)

---

### 3. read_fat
Reads and decodes a FAT16 File Allocation Table from a binary file.

**Usage:**
```bash
./read_fat <binary_file> <offset> [-v|--verbose]
```

**Example:**
```bash
./read_fat disk.img 1050624         # Read FAT at calculated offset
./read_fat disk.img 1050624 -v      # Verbose mode (shows free clusters too)
```

The FAT is located after the partition's reserved sectors.
To calculate the offset:
- Read the VBR to get reserved sectors count
- Calculate: offset = partition_start + reserved_sectors × 512

**Output includes:**
- Special entries (media descriptor, partition state)
- All FAT entries with interpretation:
  - Free clusters (0x0000)
  - Bad clusters (0xFFF7)
  - End of chain markers (0xFFF8-0xFFFF)
  - Cluster chain links (points to next cluster)
- Statistics:
  - Total, free, used, and bad clusters
  - Percentage of disk usage
  - Number of cluster chains (files)
- Cluster chain analysis:
  - Shows complete allocation chains for each file
  - Displays chain length in clusters and bytes
  - Detects circular chains and invalid entries
- Verbose mode shows all entries including free clusters

---

### 4. read_rootdir
Reads and decodes root directory entries from a binary file.

**Usage:**
```bash
./read_rootdir <binary_file> <offset>
```

**Example:**
```bash
./read_rootdir disk.img 8192  # Read root directory at offset 8192
```

The root directory is located after the partition's reserved sectors and FAT tables.
To calculate the offset:
1. Read the VBR to get reserved sectors count and FAT size
2. Calculate: offset = partition_start + (reserved_sectors + num_fats × sectors_per_fat) × 512

**Output includes:**
- Directory entries (up to 16 per sector, 32 bytes each)
- For each entry:
  - Filename in 8.3 format (and raw bytes)
  - File attributes (Read-only, Hidden, System, Volume Label, Directory, Archive)
  - Creation date and time (with tenths of seconds)
  - Last modification date and time
  - Last access date
  - First cluster number (combined high and low words)
  - File size (or \<DIR\> for directories)
- Handles special entries (., .., deleted files marked with 0xE5)
- Detects long filename entries
- Shows end of directory marker (0x00)

---

## Building

```bash
make
```

To clean up:
```bash
make clean
```

## Dependencies

- GCC compiler
- Standard C library
- layout.h (contains struct definitions)

## Offset Examples

- **MBR**: Offset 0
- **VBR** (if partition starts at LBA 2048): Offset 1048576 (= 2048 × 512)
- **FAT** (example for FAT16): Calculate based on VBR information
- **Root Directory** (example for FAT16): Calculate based on VBR information

### Calculating FAT Offset

For FAT16:
```
fat_offset = partition_start + reserved_sectors × 512
```

Example:
- Partition starts at LBA 2048 (offset 1048576)
- Reserved sectors: 4
- FAT offset = 1048576 + 4 × 512 = 1048576 + 2048 = 1050624

### Calculating Root Directory Offset

For FAT16:
```
root_dir_offset = partition_start + (reserved_sectors + num_fats × sectors_per_fat) × 512
```

Example:
- Partition starts at LBA 2048 (offset 1048576)
- Reserved sectors: 4
- Number of FATs: 2
- Sectors per FAT: 32
- Root dir offset = 1048576 + (4 + 2 × 32) × 512 = 1048576 + 68 × 512 = 1083392

You can use hexadecimal offsets too:
```bash
./read_vbr disk.img 0x100000  # 1MB in hex
```

## Notes

- All four tools read exactly 512 bytes from the specified offset
- Offsets can be specified in decimal or hexadecimal (prefix with 0x)
- The tools validate the boot signature (0xAA55) where applicable and report if invalid
- All multi-byte values are read in little-endian format
- Directory entries use FAT date/time encoding (years from 1980, 2-second resolution for times)
- Empty directory entries are marked with 0x00 as first byte
- Deleted files are marked with 0xE5 as first byte
- FAT16 uses 16-bit entries (2 bytes each), allowing 256 entries per 512-byte sector
- Use verbose mode (-v) with read_fat to see all entries including free clusters
