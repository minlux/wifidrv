/* read_mbr.c - CLI tool to read and decode MBR from a binary file */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "layout.h"

static void print_partition_entry(int index, const struct partition_entry *entry) {
    printf("  Partition %d:\n", index);
    printf("    Boot Indicator:     0x%02X (%s)\n", 
           entry->boot_indicator,
           entry->boot_indicator == 0x80 ? "bootable" : "non-bootable");
    printf("    Starting Head:      %u\n", entry->starting_head);
    printf("    Starting Sector:    %u (bits 0-5)\n", entry->starting_sector & 0x3F);
    printf("    Starting Cylinder:  %u (combined from bits)\n", 
           entry->starting_cylinder | ((entry->starting_sector & 0xC0) << 2));
    
    const char *partition_type_str = "Unknown";
    switch (entry->partition_type) {
        case 0x00: partition_type_str = "Empty"; break;
        case 0x01: partition_type_str = "FAT12"; break;
        case 0x04: partition_type_str = "FAT16 (<32MB)"; break;
        case 0x05: partition_type_str = "Extended"; break;
        case 0x06: partition_type_str = "FAT16 (>=32MB)"; break;
        case 0x07: partition_type_str = "NTFS/exFAT"; break;
        case 0x0B: partition_type_str = "FAT32 (CHS)"; break;
        case 0x0C: partition_type_str = "FAT32 (LBA)"; break;
        case 0x0E: partition_type_str = "FAT16 (LBA)"; break;
        case 0x0F: partition_type_str = "Extended (LBA)"; break;
        case 0x82: partition_type_str = "Linux Swap"; break;
        case 0x83: partition_type_str = "Linux"; break;
        case 0xEE: partition_type_str = "GPT Protective"; break;
    }
    printf("    Partition Type:     0x%02X (%s)\n", entry->partition_type, partition_type_str);
    
    printf("    Ending Head:        %u\n", entry->ending_head);
    printf("    Ending Sector:      %u (bits 0-5)\n", entry->ending_sector & 0x3F);
    printf("    Ending Cylinder:    %u (combined from bits)\n", 
           entry->ending_cylinder | ((entry->ending_sector & 0xC0) << 2));
    printf("    Starting LBA:       %u (0x%08X)\n", entry->starting_lba, entry->starting_lba);
    printf("    Size in Sectors:    %u (0x%08X)\n", entry->size_in_sectors, entry->size_in_sectors);
    
    if (entry->size_in_sectors > 0) {
        uint64_t size_bytes = (uint64_t)entry->size_in_sectors * 512;
        printf("    Size in Bytes:      %lu bytes (%.2f MB)\n", 
               size_bytes, size_bytes / (1024.0 * 1024.0));
    }
    printf("\n");
}

static void print_mbr(const struct mbr *mbr) {
    printf("=== Master Boot Record (MBR) ===\n\n");
    
    printf("Boot Code: %d bytes\n", (int)sizeof(mbr->mbr_boot_code));
    printf("  First 16 bytes: ");
    for (int i = 0; i < 16 && i < sizeof(mbr->mbr_boot_code); i++) {
        printf("%02X ", mbr->mbr_boot_code[i]);
    }
    printf("\n\n");
    
    printf("Partition Table:\n");
    for (int i = 0; i < 4; i++) {
        if (mbr->partition_table[i].partition_type != 0x00) {
            print_partition_entry(i, &mbr->partition_table[i]);
        } else {
            printf("  Partition %d: Empty\n\n", i);
        }
    }
    
    printf("Boot Signature:     0x%04X", mbr->boot_signature);
    if (mbr->boot_signature == 0xAA55) {
        printf(" (valid)\n");
    } else {
        printf(" (INVALID - expected 0xAA55)\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <binary_file> <offset>\n", argv[0]);
        fprintf(stderr, "  Reads 512 bytes starting at <offset> and decodes as MBR\n");
        fprintf(stderr, "  Example: %s disk.img 0\n", argv[0]);
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
    
    // Read 512 bytes into MBR structure
    struct mbr mbr;
    size_t bytes_read = fread(&mbr, 1, sizeof(struct mbr), fp);
    fclose(fp);
    
    if (bytes_read != sizeof(struct mbr)) {
        fprintf(stderr, "Error: Could not read %zu bytes (only read %zu bytes)\n", 
                sizeof(struct mbr), bytes_read);
        return 1;
    }
    
    printf("File: %s\n", filename);
    printf("Offset: %ld (0x%lX) bytes\n\n", offset, offset);
    
    print_mbr(&mbr);
    
    return 0;
}
