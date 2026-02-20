/* read_vbr.c - CLI tool to read and decode VBR from a binary file */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include "layout.h"

static void print_hex_ascii(const char *label, const uint8_t *data, size_t len) {
    printf("%s", label);
    for (size_t i = 0; i < len; i++) {
        if (isprint(data[i])) {
            putchar(data[i]);
        } else {
            putchar('.');
        }
    }
    printf("\" (hex: ");
    for (size_t i = 0; i < len; i++) {
        printf("%02X ", data[i]);
    }
    printf(")\n");
}

static uint16_t read_u16_le(const uint8_t *data) {
    return data[0] | (data[1] << 8);
}

static uint32_t read_u32_le(const uint8_t *data) {
    return data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
}

static void print_vbr(const struct vbr *vbr) {
    printf("=== Volume Boot Record (VBR) ===\n\n");
    
    // Jump instruction
    printf("Jump Instruction: ");
    for (int i = 0; i < 3; i++) {
        printf("%02X ", vbr->vbr_jump_instruction[i]);
    }
    if (vbr->vbr_jump_instruction[0] == 0xEB && vbr->vbr_jump_instruction[2] == 0x90) {
        printf("(JMP SHORT 0x%02X, NOP)\n", vbr->vbr_jump_instruction[1]);
    } else {
        printf("\n");
    }
    
    // OEM identifier
    print_hex_ascii("OEM Identifier:   \"", vbr->vbr_oem_identifier, 8);
    
    printf("\n--- BIOS Parameter Block (BPB) ---\n");
    
    // Parse BPB fields
    uint16_t bytes_per_sector = read_u16_le(&vbr->vbr_bpb[0]);
    uint8_t sectors_per_cluster = vbr->vbr_bpb[2];
    uint16_t reserved_sectors = read_u16_le(&vbr->vbr_bpb[3]);
    uint8_t num_fats = vbr->vbr_bpb[5];
    uint16_t root_dir_entries = read_u16_le(&vbr->vbr_bpb[6]);
    uint16_t total_sectors_16 = read_u16_le(&vbr->vbr_bpb[8]);
    uint8_t media_descriptor = vbr->vbr_bpb[10];
    uint16_t sectors_per_fat_16 = read_u16_le(&vbr->vbr_bpb[11]);
    uint16_t sectors_per_track = read_u16_le(&vbr->vbr_bpb[13]);
    uint16_t num_heads = read_u16_le(&vbr->vbr_bpb[15]);
    uint32_t hidden_sectors = read_u32_le(&vbr->vbr_bpb[17]);
    uint32_t total_sectors_32 = read_u32_le(&vbr->vbr_bpb[21]);
    
    printf("  Bytes per Sector:      %u (0x%04X)\n", bytes_per_sector, bytes_per_sector);
    printf("  Sectors per Cluster:   %u\n", sectors_per_cluster);
    if (sectors_per_cluster > 0) {
        printf("    Cluster Size:        %u bytes (%u KB)\n", 
               bytes_per_sector * sectors_per_cluster,
               (bytes_per_sector * sectors_per_cluster) / 1024);
    }
    printf("  Reserved Sectors:      %u\n", reserved_sectors);
    printf("  Number of FATs:        %u\n", num_fats);
    printf("  Root Dir Entries:      %u\n", root_dir_entries);
    printf("  Total Sectors (16):    %u", total_sectors_16);
    if (total_sectors_16 > 0) {
        printf(" (%.2f MB)\n", (total_sectors_16 * 512.0) / (1024.0 * 1024.0));
    } else {
        printf(" (use 32-bit field)\n");
    }
    
    const char *media_str = "Unknown";
    switch (media_descriptor) {
        case 0xF0: media_str = "Removable disk"; break;
        case 0xF8: media_str = "Fixed disk"; break;
        case 0xF9: media_str = "720 KB floppy"; break;
        case 0xFD: media_str = "360 KB floppy"; break;
        case 0xFF: media_str = "320 KB floppy"; break;
    }
    printf("  Media Descriptor:      0x%02X (%s)\n", media_descriptor, media_str);
    
    printf("  Sectors per FAT (16):  %u\n", sectors_per_fat_16);
    printf("  Sectors per Track:     %u\n", sectors_per_track);
    printf("  Number of Heads:       %u\n", num_heads);
    printf("  Hidden Sectors:        %u (0x%08X)\n", hidden_sectors, hidden_sectors);
    printf("  Total Sectors (32):    %u", total_sectors_32);
    if (total_sectors_32 > 0) {
        printf(" (%.2f MB)\n", (total_sectors_32 * 512.0) / (1024.0 * 1024.0));
    } else {
        printf("\n");
    }
    
    printf("\n--- Extended Boot Record (EBR) ---\n");
    
    // Parse EBR fields (FAT32 or FAT16)
    uint32_t sectors_per_fat_32 = read_u32_le(&vbr->vbr_ebr[0]);
    uint16_t fat_flags = read_u16_le(&vbr->vbr_ebr[4]);
    uint16_t version = read_u16_le(&vbr->vbr_ebr[6]);
    uint32_t root_cluster = read_u32_le(&vbr->vbr_ebr[8]);
    uint16_t fsinfo_sector = read_u16_le(&vbr->vbr_ebr[12]);
    uint16_t backup_boot_sector = read_u16_le(&vbr->vbr_ebr[14]);
    uint8_t drive_number = vbr->vbr_ebr[28];
    uint8_t ext_boot_sig = vbr->vbr_ebr[30];
    uint32_t volume_serial = read_u32_le(&vbr->vbr_ebr[31]);
    
    // Determine if FAT32 or FAT16
    int is_fat32 = (sectors_per_fat_16 == 0 && sectors_per_fat_32 > 0) || 
                   (root_dir_entries == 0);
    
    if (is_fat32) {
        printf("  File System Type:      FAT32\n");
        printf("  Sectors per FAT (32):  %u\n", sectors_per_fat_32);
        printf("  FAT Flags:             0x%04X\n", fat_flags);
        printf("  Version:               %u.%u\n", version >> 8, version & 0xFF);
        printf("  Root Cluster:          %u\n", root_cluster);
        printf("  FSInfo Sector:         %u\n", fsinfo_sector);
        printf("  Backup Boot Sector:    %u\n", backup_boot_sector);
    } else {
        printf("  File System Type:      FAT16\n");
    }
    
    printf("  Drive Number:          0x%02X\n", drive_number);
    printf("  Extended Boot Sig:     0x%02X", ext_boot_sig);
    if (ext_boot_sig == 0x29) {
        printf(" (valid - following fields are present)\n");
    } else {
        printf("\n");
    }
    
    if (ext_boot_sig == 0x29) {
        printf("  Volume Serial Number:  0x%08X\n", volume_serial);
        print_hex_ascii("  Volume Label:          \"", &vbr->vbr_ebr[35], 11);
        print_hex_ascii("  File System Type:      \"", &vbr->vbr_ebr[46], 8);
    }
    
    printf("\nBoot Code: %d bytes\n", (int)sizeof(vbr->vbr_boot_code));
    printf("  First 16 bytes: ");
    for (int i = 0; i < 16 && i < sizeof(vbr->vbr_boot_code); i++) {
        printf("%02X ", vbr->vbr_boot_code[i]);
    }
    printf("\n");
    
    // Check for boot message in the boot code
    for (size_t i = 0; i < sizeof(vbr->vbr_boot_code) - 10; i++) {
        if (vbr->vbr_boot_code[i] == 'T' && 
            strncmp((char*)&vbr->vbr_boot_code[i], "This is not", 11) == 0) {
            printf("  Boot message found at offset %zu\n", i);
            break;
        }
    }
    
    printf("\nBoot Signature:     0x%04X", vbr->vbr_boot_signature);
    if (vbr->vbr_boot_signature == 0xAA55) {
        printf(" (valid)\n");
    } else {
        printf(" (INVALID - expected 0xAA55)\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <binary_file> <offset>\n", argv[0]);
        fprintf(stderr, "  Reads 512 bytes starting at <offset> and decodes as VBR\n");
        fprintf(stderr, "  Example: %s disk.img 1048576  # Read VBR at 1MB offset\n", argv[0]);
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
    
    // Read 512 bytes into VBR structure
    struct vbr vbr;
    size_t bytes_read = fread(&vbr, 1, sizeof(struct vbr), fp);
    fclose(fp);
    
    if (bytes_read != sizeof(struct vbr)) {
        fprintf(stderr, "Error: Could not read %zu bytes (only read %zu bytes)\n", 
                sizeof(struct vbr), bytes_read);
        return 1;
    }
    
    printf("File: %s\n", filename);
    printf("Offset: %ld (0x%lX) bytes\n\n", offset, offset);
    
    print_vbr(&vbr);
    
    return 0;
}
