/* read_rootdir.c - CLI tool to read and decode root directory entries from a binary file */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "layout.h"

// FAT attribute flags
#define ATTR_READ_ONLY  0x01
#define ATTR_HIDDEN     0x02
#define ATTR_SYSTEM     0x04
#define ATTR_VOLUME_ID  0x08
#define ATTR_DIRECTORY  0x10
#define ATTR_ARCHIVE    0x20
#define ATTR_LONG_NAME  (ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID)

// Number of directory entries in one sector
#define ENTRIES_PER_SECTOR (DISK_SECTOR_SIZE / sizeof(struct directory_record))

static void decode_fat_time(uint16_t fat_time, int *hour, int *minute, int *second) {
    *hour = (fat_time >> 11) & 0x1F;
    *minute = (fat_time >> 5) & 0x3F;
    *second = (fat_time & 0x1F) * 2;
}

static void decode_fat_date(uint16_t fat_date, int *year, int *month, int *day) {
    *year = 1980 + ((fat_date >> 9) & 0x7F);
    *month = (fat_date >> 5) & 0x0F;
    *day = fat_date & 0x1F;
}

static void print_fat_datetime(uint16_t fat_date, uint16_t fat_time) {
    int year, month, day, hour, minute, second;
    decode_fat_date(fat_date, &year, &month, &day);
    decode_fat_time(fat_time, &hour, &minute, &second);
    printf("%04d-%02d-%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
}

static void print_attributes(uint8_t attr) {
    printf("0x%02X (", attr);
    
    if (attr == 0) {
        printf("Normal");
    } else if ((attr & ATTR_LONG_NAME) == ATTR_LONG_NAME) {
        printf("Long Filename");
    } else {
        int first = 1;
        if (attr & ATTR_READ_ONLY) {
            printf("%sRead-only", first ? "" : ", ");
            first = 0;
        }
        if (attr & ATTR_HIDDEN) {
            printf("%sHidden", first ? "" : ", ");
            first = 0;
        }
        if (attr & ATTR_SYSTEM) {
            printf("%sSystem", first ? "" : ", ");
            first = 0;
        }
        if (attr & ATTR_VOLUME_ID) {
            printf("%sVolume Label", first ? "" : ", ");
            first = 0;
        }
        if (attr & ATTR_DIRECTORY) {
            printf("%sDirectory", first ? "" : ", ");
            first = 0;
        }
        if (attr & ATTR_ARCHIVE) {
            printf("%sArchive", first ? "" : ", ");
            first = 0;
        }
    }
    printf(")");
}

static void print_filename(const uint8_t *name) {
    // Print 8.3 filename format
    char filename[13];
    int i, j = 0;
    
    // Copy base name (8 bytes)
    for (i = 0; i < 8 && name[i] != ' '; i++) {
        filename[j++] = name[i];
    }
    
    // Check if there's an extension
    int has_ext = 0;
    for (i = 8; i < 11; i++) {
        if (name[i] != ' ') {
            has_ext = 1;
            break;
        }
    }
    
    // Add extension if present
    if (has_ext) {
        filename[j++] = '.';
        for (i = 8; i < 11 && name[i] != ' '; i++) {
            filename[j++] = name[i];
        }
    }
    
    filename[j] = '\0';
    printf("\"%s\"", filename);
}

static void print_directory_entry(int index, const struct directory_record *entry) {
    // Check for empty entry
    if (entry->name[0] == 0x00) {
        return; // End of directory
    }
    
    // Check for deleted entry
    if (entry->name[0] == 0xE5) {
        printf("Entry %d: <DELETED>\n\n", index);
        return;
    }
    
    // Check for special entries (. and ..)
    if (entry->name[0] == '.') {
        printf("Entry %d: Special directory entry\n", index);
        printf("  Name:          ");
        if (entry->name[1] == '.' && entry->name[2] == ' ') {
            printf("\"..\" (parent directory)\n");
        } else if (entry->name[1] == ' ') {
            printf("\".\" (current directory)\n");
        } else {
            print_filename(entry->name);
            printf("\n");
        }
        printf("  Attributes:    ");
        print_attributes(entry->attributes);
        printf("\n\n");
        return;
    }
    
    printf("Entry %d:\n", index);
    
    // Filename
    printf("  Name:          ");
    print_filename(entry->name);
    
    // Show raw bytes for debugging
    printf(" (raw: ");
    for (int i = 0; i < 11; i++) {
        if (isprint(entry->name[i])) {
            putchar(entry->name[i]);
        } else {
            printf("\\x%02X", entry->name[i]);
        }
    }
    printf(")\n");
    
    // Attributes
    printf("  Attributes:    ");
    print_attributes(entry->attributes);
    printf("\n");
    
    // Skip detailed info for long filename entries
    if ((entry->attributes & ATTR_LONG_NAME) == ATTR_LONG_NAME) {
        printf("  (Long filename entry - skip detailed decode)\n\n");
        return;
    }
    
    // Creation time
    if (entry->creation_date != 0 || entry->creation_time != 0) {
        printf("  Created:       ");
        print_fat_datetime(entry->creation_date, entry->creation_time);
        if (entry->creation_time_tenths > 0) {
            printf(".%02d", entry->creation_time_tenths);
        }
        printf("\n");
    }
    
    // Last modification time
    if (entry->last_mod_date != 0 || entry->last_mod_time != 0) {
        printf("  Modified:      ");
        print_fat_datetime(entry->last_mod_date, entry->last_mod_time);
        printf("\n");
    }
    
    // Last access date
    if (entry->last_access_date != 0) {
        printf("  Last Access:   ");
        int year, month, day;
        decode_fat_date(entry->last_access_date, &year, &month, &day);
        printf("%04d-%02d-%02d\n", year, month, day);
    }
    
    // Cluster number
    uint32_t cluster = ((uint32_t)entry->first_cluster_high << 16) | entry->first_cluster_low;
    printf("  First Cluster: %u (0x%08X)\n", cluster, cluster);
    
    // File size
    if (entry->attributes & ATTR_DIRECTORY) {
        printf("  Size:          <DIR>\n");
    } else {
        printf("  Size:          %u bytes", entry->file_size);
        if (entry->file_size >= 1024) {
            if (entry->file_size >= 1024 * 1024) {
                printf(" (%.2f MB)", entry->file_size / (1024.0 * 1024.0));
            } else {
                printf(" (%.2f KB)", entry->file_size / 1024.0);
            }
        }
        printf("\n");
    }
    
    printf("\n");
}

static void print_root_directory(const struct directory_record *entries) {
    printf("=== Root Directory Entries ===\n\n");
    printf("Reading %d directory entries (512 bytes / 32 bytes per entry)\n\n", 
           ENTRIES_PER_SECTOR);
    
    int count = 0;
    for (int i = 0; i < ENTRIES_PER_SECTOR; i++) {
        if (entries[i].name[0] == 0x00) {
            // End of directory
            printf("End of directory (entry %d has 0x00 as first byte)\n", i);
            break;
        }
        
        if (entries[i].name[0] != 0xE5) {
            count++;
        }
        
        print_directory_entry(i, &entries[i]);
    }
    
    printf("Total entries found: %d (excluding deleted)\n", count);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <binary_file> <offset>\n", argv[0]);
        fprintf(stderr, "  Reads 512 bytes starting at <offset> and decodes as root directory\n");
        fprintf(stderr, "  Example: %s disk.img 8192  # Read root dir at offset 8192\n", argv[0]);
        fprintf(stderr, "\n");
        fprintf(stderr, "To find the root directory offset:\n");
        fprintf(stderr, "  1. Read the VBR to get the number of reserved sectors and FAT size\n");
        fprintf(stderr, "  2. Calculate: offset = partition_start + (reserved + num_fats * fat_size) * 512\n");
        return 1;
    }
    
    const char *filename = argv[1];
    long offset = strtol(argv[2], NULL, 0);
    
    if (offset < 0) {
        fprintf(stderr, "Error: Offset must be non-negative\n");
        return 1;
    }
    
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("Error opening file");
        return 1;
    }
    
    // Seek to the specified offset
    if (fseek(fp, offset, SEEK_SET) != 0) {
        perror("Error seeking to offset");
        fclose(fp);
        return 1;
    }
    
    // Read 512 bytes as directory entries
    struct directory_record entries[ENTRIES_PER_SECTOR];
    size_t bytes_read = fread(entries, 1, sizeof(entries), fp);
    fclose(fp);
    
    if (bytes_read != sizeof(entries)) {
        fprintf(stderr, "Error: Could not read %zu bytes (only read %zu bytes)\n", 
                sizeof(entries), bytes_read);
        return 1;
    }
    
    printf("File: %s\n", filename);
    printf("Offset: %ld (0x%lX) bytes\n\n", offset, offset);
    
    print_root_directory(entries);
    
    return 0;
}
