/* read_fat.c - CLI tool to read and decode FAT16 File Allocation Table from a binary file */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "layout.h"

// FAT16 entry special values
#define FAT16_FREE          0x0000
#define FAT16_RESERVED      0x0001
#define FAT16_BAD_CLUSTER   0xFFF7
#define FAT16_EOC_MIN       0xFFF8  // End of Chain minimum
#define FAT16_EOC_MAX       0xFFFF  // End of Chain maximum

// Number of FAT16 entries in one sector (512 bytes / 2 bytes per entry)
#define FAT16_ENTRIES_PER_SECTOR (DISK_SECTOR_SIZE / 2)

typedef struct {
    uint32_t free_clusters;
    uint32_t used_clusters;
    uint32_t bad_clusters;
    uint32_t eoc_markers;
    uint32_t total_chains;
} fat_statistics_t;

static const char* get_fat_entry_description(uint16_t entry) {
    if (entry == FAT16_FREE) {
        return "Free cluster";
    } else if (entry == FAT16_RESERVED) {
        return "Reserved";
    } else if (entry == FAT16_BAD_CLUSTER) {
        return "Bad cluster";
    } else if (entry >= FAT16_EOC_MIN && entry <= FAT16_EOC_MAX) {
        return "End of chain";
    } else if (entry >= 0x0002 && entry <= 0xFFEF) {
        return "Next cluster";
    } else {
        return "Unknown/Invalid";
    }
}

static void print_fat_entry(uint32_t cluster_num, uint16_t entry, int verbose) {
    if (!verbose && entry == FAT16_FREE) {
        // Skip free clusters in non-verbose mode
        return;
    }
    
    printf("  Cluster %4u (0x%04X): 0x%04X", cluster_num, cluster_num, entry);
    
    const char *desc = get_fat_entry_description(entry);
    
    if (entry == FAT16_FREE) {
        printf(" - %s\n", desc);
    } else if (entry == FAT16_RESERVED) {
        printf(" - %s\n", desc);
    } else if (entry == FAT16_BAD_CLUSTER) {
        printf(" - %s (marked as defective)\n", desc);
    } else if (entry >= FAT16_EOC_MIN && entry <= FAT16_EOC_MAX) {
        printf(" - %s (last cluster in file)\n", desc);
    } else if (entry >= 0x0002 && entry <= 0xFFEF) {
        printf(" - %s -> %u (0x%04X)\n", desc, entry, entry);
    } else {
        printf(" - %s\n", desc);
    }
}

static void analyze_chains(const uint16_t *fat_table, uint32_t num_entries) {
    printf("\n=== Cluster Chain Analysis ===\n\n");
    
    // Track which clusters are chain starts
    uint8_t *is_continuation = calloc(num_entries, sizeof(uint8_t));
    if (!is_continuation) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return;
    }
    
    // Mark all clusters that are pointed to by another cluster
    for (uint32_t i = 2; i < num_entries; i++) {
        uint16_t entry = fat_table[i];
        if (entry >= 0x0002 && entry <= 0xFFEF && entry < num_entries) {
            is_continuation[entry] = 1;
        }
    }
    
    // Find and print all chains
    int chain_count = 0;
    for (uint32_t i = 2; i < num_entries; i++) {
        uint16_t entry = fat_table[i];
        
        // Skip free, bad, and continuation clusters
        if (entry == FAT16_FREE || entry == FAT16_BAD_CLUSTER || is_continuation[i]) {
            continue;
        }
        
        // This is a chain start
        chain_count++;
        printf("Chain %d starts at cluster %u:\n  ", chain_count, i);
        
        uint32_t current = i;
        int length = 1;
        int max_length = 1000; // Prevent infinite loops
        
        printf("%u", current);
        
        while (length < max_length) {
            uint16_t next = fat_table[current];
            
            if (next >= FAT16_EOC_MIN && next <= FAT16_EOC_MAX) {
                // End of chain
                printf(" -> EOC\n");
                break;
            } else if (next >= 0x0002 && next <= 0xFFEF && next < num_entries) {
                printf(" -> %u", next);
                current = next;
                length++;
                
                // Line break every 10 clusters for readability
                if (length % 10 == 0) {
                    printf("\n  ");
                }
            } else {
                printf(" -> INVALID(0x%04X)\n", next);
                break;
            }
        }
        
        if (length >= max_length) {
            printf(" -> ERROR: Chain too long or circular\n");
        }
        
        printf("  Length: %d cluster%s", length, length == 1 ? "" : "s");
        printf(" (%u bytes)\n\n", length * 512);  // Assuming 1 cluster = 1 sector for simplicity
    }
    
    if (chain_count == 0) {
        printf("No cluster chains found (no allocated files).\n");
    } else {
        printf("Total chains found: %d\n", chain_count);
    }
    
    free(is_continuation);
}

static void print_statistics(const fat_statistics_t *stats, uint32_t num_entries) {
    printf("\n=== FAT Statistics ===\n\n");
    printf("Total clusters:     %u\n", num_entries);
    printf("Free clusters:      %u (%.1f%%)\n", 
           stats->free_clusters, 
           (stats->free_clusters * 100.0) / num_entries);
    printf("Used clusters:      %u (%.1f%%)\n", 
           stats->used_clusters, 
           (stats->used_clusters * 100.0) / num_entries);
    printf("Bad clusters:       %u\n", stats->bad_clusters);
    printf("EOC markers:        %u\n", stats->eoc_markers);
    printf("Estimated chains:   %u\n", stats->total_chains);
}

static void print_fat_table(const uint16_t *fat_table, int verbose) {
    printf("=== FAT16 File Allocation Table ===\n\n");
    printf("Reading %d FAT entries (512 bytes / 2 bytes per entry)\n\n", 
           FAT16_ENTRIES_PER_SECTOR);
    
    fat_statistics_t stats = {0};
    
    // First two entries are special (media descriptor and partition state)
    printf("Special entries:\n");
    printf("  Entry 0 (Media):  0x%04X", fat_table[0]);
    if ((fat_table[0] & 0xFF) == 0xF8) {
        printf(" - Fixed disk media descriptor\n");
    } else {
        printf(" - Media descriptor: 0x%02X\n", fat_table[0] & 0xFF);
    }
    
    printf("  Entry 1 (State):  0x%04X", fat_table[1]);
    if (fat_table[1] == 0xFFFF) {
        printf(" - Clean unmount\n");
    } else if (fat_table[1] == 0xFF7F) {
        printf(" - Dirty (not cleanly unmounted)\n");
    } else {
        printf("\n");
    }
    
    printf("\nData cluster entries (starting from cluster 2):\n");
    
    // Process remaining entries
    for (int i = 2; i < FAT16_ENTRIES_PER_SECTOR; i++) {
        uint16_t entry = fat_table[i];
        
        // Update statistics
        if (entry == FAT16_FREE) {
            stats.free_clusters++;
        } else if (entry == FAT16_BAD_CLUSTER) {
            stats.bad_clusters++;
        } else if (entry >= FAT16_EOC_MIN && entry <= FAT16_EOC_MAX) {
            stats.eoc_markers++;
            stats.used_clusters++;
        } else if (entry >= 0x0002 && entry <= 0xFFEF) {
            stats.used_clusters++;
        }
        
        print_fat_entry(i, entry, verbose);
    }
    
    print_statistics(&stats, FAT16_ENTRIES_PER_SECTOR);
    
    // Analyze cluster chains
    if (stats.used_clusters > 0) {
        analyze_chains(fat_table, FAT16_ENTRIES_PER_SECTOR);
    }
}

int main(int argc, char *argv[]) {
    int verbose = 0;
    const char *filename = NULL;
    long offset = 0;
    
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (filename == NULL) {
            filename = argv[i];
        } else if (offset == 0) {
            offset = strtol(argv[i], NULL, 0);
        }
    }
    
    if (filename == NULL || (argc < 3)) {
        fprintf(stderr, "Usage: %s <binary_file> <offset> [-v|--verbose]\n", argv[0]);
        fprintf(stderr, "  Reads 512 bytes starting at <offset> and decodes as FAT16 table\n");
        fprintf(stderr, "  Example: %s disk.img 1050624\n", argv[0]);
        fprintf(stderr, "  Use -v or --verbose to show all entries including free clusters\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "To find the FAT offset:\n");
        fprintf(stderr, "  1. Read the VBR to get the number of reserved sectors\n");
        fprintf(stderr, "  2. Calculate: offset = partition_start + reserved_sectors * 512\n");
        fprintf(stderr, "  Example: partition at LBA 2048, reserved=4 -> offset = 1048576 + 2048 = 1050624\n");
        return 1;
    }
    
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
    
    // Read 512 bytes as FAT16 entries (256 entries of 2 bytes each)
    uint16_t fat_table[FAT16_ENTRIES_PER_SECTOR];
    size_t bytes_read = fread(fat_table, 1, sizeof(fat_table), fp);
    fclose(fp);
    
    if (bytes_read != sizeof(fat_table)) {
        fprintf(stderr, "Error: Could not read %zu bytes (only read %zu bytes)\n", 
                sizeof(fat_table), bytes_read);
        return 1;
    }
    
    printf("File: %s\n", filename);
    printf("Offset: %ld (0x%lX) bytes\n", offset, offset);
    printf("Verbose mode: %s\n\n", verbose ? "enabled" : "disabled");
    
    print_fat_table(fat_table, verbose);
    
    return 0;
}
