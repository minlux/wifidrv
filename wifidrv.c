// References:
// https://www.pjrc.com/tech/8051/ide/fat32.html

// Similar Projects:
// https://github.com/CodeName33/Wi-Fi-Flash-Drive.git
 

#include <stdint.h>
#include <string.h>
#include <stdio.h>


// Every sector is typically 512 bytes in size.
#define DISK_SECTOR_SIZE 512

struct partition_entry {
    uint8_t boot_indicator;      // 0x80 = bootable, 0x00 = non-bootable
    uint8_t starting_head;       // Starting head
    uint8_t starting_sector;     // Starting sector (bits 0-5), starting cylinder (bits 6-7)
    uint8_t starting_cylinder;   // Starting cylinder
    uint8_t partition_type;      // Partition type (e.g., 0x0B or 0x0C for FAT32)
    uint8_t ending_head;         // Ending head
    uint8_t ending_sector;       // Ending sector (bits 0-5), ending cylinder (bits 6-7)
    uint8_t ending_cylinder;     // Ending cylinder
    uint32_t starting_lba;       // Starting LBA of the partition
    uint32_t size_in_sectors;    // Size of the partition in sectors
}; // sizeof(struct partition_entry) == 16 bytes


struct directory_record {
    uint8_t name[11];            // File name (8 bytes) + extension (3 bytes)
    uint8_t attributes;          // File attributes
    uint8_t reserved;            // Reserved for Windows NT
    uint8_t creation_time_tenths; // Creation time in tenths of a second
    uint16_t creation_time;      // Creation time
    uint16_t creation_date;      // Creation date
    uint16_t last_access_date;   // Last access date
    uint16_t first_cluster_high; // High word of first cluster number
    uint16_t last_mod_time;      // Last modification time
    uint16_t last_mod_date;      // Last modification date
    uint16_t first_cluster_low;  // Low word of first cluster number
    uint32_t file_size;          // File size in bytes
}; // sizeof(struct directory_record) == 32 bytes








/* Master Boot Record (MBR) */
struct mbr {
    // The first sector of the drive is called the Master Boot Record (MBR). You can read it with LBA = 0
    // The first 446 bytes of the MBR are code that boots the computer.
    uint8_t mbr_boot_code[446];
    // The next 64 bytes are the partition table, which describes the partitions on the disk.
    struct partition_entry partition_table[4];
    // The last 2 bytes are the boot signature, which should be 0x55AA.
    uint16_t boot_signature;
}; // sizeof(struct mbr) == 512 bytes


/* Volume Boot Record (VBR) */
struct vbr {
    // Fieled *starting_lba* in partition_entry indicates the starting LBA of the partition.
    // The first 3 bytes are the jump instruction to boot code.
    uint8_t vbr_jump_instruction[3];
    // The next 8 bytes are the OEM identifier.
    uint8_t vbr_oem_identifier[8];
    // The next 25 bytes are the BIOS Parameter Block (BPB), which contains information about the volume.
    //   [0:1] = Bytes per Sector (should be 512),
    //     [2] = Sectors per Cluster (2^0 .. 2^7),
    //   [3:4] = Reserved Sector Count (usually 32 for FAT32),
    //     [5] = Number of FATs (usually 2),
    //   [6:7] = Max Root Dir Entries (0 for FAT32),
    //   [8:9] = Total Sectors (if zero, use 32-bit field at vbr_bpb[21:24]),
    //    [10] = Media Descriptor,
    // [11:12] = Sectors per FAT (0 for FAT32),
    // [13:14] = Sectors per Track,
    // [15:16] = Number of Heads,
    // [17:20] = Hidden Sectors,
    // [21:24] = Total Sectors (32-bit)
    uint8_t vbr_bpb[25];
    // The next 54 bytes are the Extended Boot Record (EBR), which contains additional information about the volume.
    //   [0:3] = Sectors per FAT (FAT32),
    //   [4:5] = FAT Flags,
    //   [6:7] = Version Number,
    //  [8:11] = Root Cluster Number,
    // [12:13] = FSInfo Sector Number,
    // [14:15] = Backup Boot Sector Number,
    // [16:27] = Reserved (12 bytes)
    //    [28] = Drive Number
    //    [29] = Reserved
    //    [30] = Extended Boot Signature
    // [31-34] = Volume Serial Number
    // [35-45] = Volume Label (11 bytes)
    // [46-53] = File System Type (8 bytes)
    uint8_t vbr_ebr[54];
    // The next 420 bytes are the boot code.
    uint8_t vbr_boot_code[420];
    // The last 2 bytes are the boot signature, which should be 0x55AA.
    uint16_t vbr_boot_signature;
}; // sizeof(struct vbr) == 512 bytes

/* Complete Disk Structure */
struct disk {
    /* LBA/SECTOR 0: Master Boot Record (MBR), Byte 0 .. 511 */
    struct mbr mbr;
    /* LBA/SECTOR 1: Volume Boot Record (VBR), Byte 512 .. 1023 */
    struct vbr vbr;
    /* LBA/SECTOR X .. Y: Reserved sectors
    uint8_t reserved_sectors[0 * DISK_SECTOR_SIZE];
    */
    /* LBA/SECTOR 2: FAT Table 1 */
    uint8_t fat_table_1[DISK_SECTOR_SIZE];
    /* LBA/SECTOR 3: FAT Table 2 */
    uint8_t fat_table_2[DISK_SECTOR_SIZE];
    /* LBA/SECTOR 4+: Clusters of 8KB each (16 sectors per cluster)
    uint8_t clusters[][16 * DISK_SECTOR_SIZE];
    */
};



// LBA/SECTOR 0 - Master Boot Record (MBR), 512 bytes
void * get_mbr(void * buffer) {
    struct mbr *mbr = (struct mbr *)buffer;
    // Clear boot code area
    memset(mbr->mbr_boot_code, 0, sizeof(mbr->mbr_boot_code)); // Clear boot code area
    // Set partition 0
    mbr->partition_table[0].boot_indicator = 0; // Non-bootable
    mbr->partition_table[0].starting_head = 0;
    mbr->partition_table[0].starting_sector = 0;
    mbr->partition_table[0].starting_cylinder = 0;
    mbr->partition_table[0].partition_type = 0x0C; // FAT32 with LBA addressing
    mbr->partition_table[0].ending_head = 0;
    mbr->partition_table[0].ending_sector = 0;
    mbr->partition_table[0].ending_cylinder = 0;
    mbr->partition_table[0].starting_lba = 1; // Starting LBA of the partition
    mbr->partition_table[0].size_in_sectors = 2048; // Size of the partition in sectors (1MB)
    // Clear other partitions
    memset(mbr->partition_table + 1, 0, 3 * sizeof(struct partition_entry));
    // Set boot signature
    mbr->boot_signature = 0xAA55;
    return buffer;
}

// LBA/SECTOR 1 - Volume Boot Record (VBR), 512 bytes
void * get_vbr(void * buffer) {
    struct vbr *vbr = (struct vbr *)buffer;
    // Set jump instruction
    vbr->vbr_jump_instruction[0] = 0xEB; // JMP short
    vbr->vbr_jump_instruction[1] = 0x58; // JMP offset
    vbr->vbr_jump_instruction[2] = 0x90; // NOP
    // Set OEM identifier
    memcpy(vbr->vbr_oem_identifier, "WIFI-DRV", 8);
    // Set BIOS Parameter Block (BPB)
    vbr->vbr_bpb[0] = 0x00; vbr->vbr_bpb[1] = 0x02; // Bytes per Sector = 512
    vbr->vbr_bpb[2] = 0x10; // Sectors per Cluster = 16 -> 8KB clusters
    vbr->vbr_bpb[3] = 0x01; vbr->vbr_bpb[4] = 0x00; // Reserved Sector Count = 1
    vbr->vbr_bpb[5] = 0x02; // Number of FATs = 2
    vbr->vbr_bpb[6] = 0x00; vbr->vbr_bpb[7] = 0x00; // Max Root Dir Entries = 0 (FAT32)
    vbr->vbr_bpb[8] = 0x00; vbr->vbr_bpb[9] = 0x00; // Total Sectors (16-bit) = 0 -> use 32-bit field instead
    vbr->vbr_bpb[10] = 0xF8; // Media Descriptor
    vbr->vbr_bpb[11] = 0x00; vbr->vbr_bpb[12] = 0x00; // Sectors per FAT (16-bit) = 0 -> use 32-bit field instead
    vbr->vbr_bpb[13] = 0x00; vbr->vbr_bpb[14] = 0x00; // Sectors per Track = 0
    vbr->vbr_bpb[15] = 0x00; vbr->vbr_bpb[16] = 0x00; // Number of Heads =  0
    vbr->vbr_bpb[17] = 0x00; vbr->vbr_bpb[18] = 0x00; vbr->vbr_bpb[19] = 0x00; vbr->vbr_bpb[20] = 0x00; // Hidden Sectors = 0
    vbr->vbr_bpb[21] = 0x00; vbr->vbr_bpb[22] = 0x08; vbr->vbr_bpb[23] = 0x00; vbr->vbr_bpb[24] = 0x00; // Total Sectors (32-bit) = 2048
    // Set Extended Boot Record (EBR)
    vbr->vbr_ebr[0] = 0x01; vbr->vbr_ebr[1] = 0x00; vbr->vbr_ebr[2] = 0x00; vbr->vbr_ebr[3] = 0x00; // Sectors per FAT (FAT32) = 1
    vbr->vbr_ebr[4] = 0x00; vbr->vbr_ebr[5] = 0x00; // FAT Flags
    vbr->vbr_ebr[6] = 0x00; vbr->vbr_ebr[7] = 0x00; // Version Number
    vbr->vbr_ebr[8] = 0x02; vbr->vbr_ebr[9] = 0x00; vbr->vbr_ebr[10] = 0x00; vbr->vbr_ebr[11] = 0x00; // Root Cluster Number = 2
    vbr->vbr_ebr[12] = 0x00; vbr->vbr_ebr[13] = 0x00; // FSInfo Sector Number = 0
    vbr->vbr_ebr[14] = 0x00; vbr->vbr_ebr[15] = 0x00; // Backup Boot Sector Number = 0
    memset(vbr->vbr_ebr + 16, 0, 12); // Reserved (12 bytes)
    vbr->vbr_ebr[28] = 0x80; // Drive Number (0x80 = hard disk)
    vbr->vbr_ebr[29] = 0x00; // Reserved
    vbr->vbr_ebr[30] = 0x29; // Extended Boot Signature
    vbr->vbr_ebr[31] = 0x12; vbr->vbr_ebr[32] = 0x34; vbr->vbr_ebr[33] = 0x56; vbr->vbr_ebr[34] = 0x78; // Volume Serial Number
    memcpy(vbr->vbr_ebr + 35, "WIFI-DRIVE ", 11); // Volume Label (11 bytes, space-padded)
    memcpy(vbr->vbr_ebr + 46, "FAT32   ", 8); // File System Type (8 bytes, space-padded)
    // Clear boot area
    memset(vbr->vbr_boot_code, 0, sizeof(vbr->vbr_boot_code)); // Clear boot area
    // Set boot signature
    vbr->vbr_boot_signature = 0xAA55;
    return buffer;
}


void * get_fat(void * buffer) {
    uint32_t *fat = (uint32_t *)buffer;
    // First two entries
    fat[0] = 0x0FFFFFF8; // Media descriptor and reserved cluster
    fat[1] = 0x0FFFFFFF; // Reserved cluster

    // Cluster 2: Root directory (one cluster)
    fat[2] = 0x0FFFFFFF; // End of chain for root directory

    // IMG1.JPG: Starts at cluster 3, 256kB = 32 clusters (8kB per cluster)
    // Cluster chain: 3 -> 4 -> 5 -> ... -> 34
    for (int i = 3; i < 34; i++) {
        fat[i] = i + 1; // Point to next cluster
    }
    fat[34] = 0x0FFFFFFF; // End of chain for IMG1.JPG

    // IMG2.JPG: Starts at cluster 35, 256kB = 32 clusters (8kB per cluster)
    // Cluster chain: 35 -> 36 -> 37 -> ... -> 66
    for (int i = 35; i < 66; i++) {
        fat[i] = i + 1; // Point to next cluster
    }
    fat[66] = 0x0FFFFFFF; // End of chain for IMG2.JPG

    // Clear remaining entries (if buffer is larger than needed)
    if (DISK_SECTOR_SIZE > 67 * 4) {
        memset(fat + 67, 0, DISK_SECTOR_SIZE - (67 * 4));
    }
    return buffer;
}


// Root directory is located at cluster 2, 
// which corresponds to LBA 4 (since reserved sectors = 1, FATs = 2, each FAT = 1 sector).
// Each cluster is 8KB (16 sectors), so root directory spans LBA 4 to LBA 19.
// This function returns only the first sector of the root directory.
void * get_root_directory_sector(void * buffer) {
    struct directory_record *dir = (struct directory_record *)buffer;
    // Clear root directory
    memset(dir, 0, DISK_SECTOR_SIZE);
    // File1: IMG1.JPG 256kB
    memcpy(dir[0].name, "IMG1    JPG", 11);
    dir[0].attributes = 0x20; // Archive
    dir[0].first_cluster_low = 3; // Starting cluster
    dir[0].file_size = 256 * 1024; // 256kB file size
    // File1: IMG1.JPG 256kB
    memcpy(dir[1].name, "IMG2    JPG", 11);
    dir[1].attributes = 0x20; // Archive
    dir[1].first_cluster_low = 35; // Starting cluster
    dir[1].file_size = 256 * 1024; // 256kB file size
    return buffer;
}



int main(int argc, char *argv[]) {
    // Open file for writing
    FILE *fp = fopen("wifidrv.img", "wb");
    if (fp == NULL) {
        fprintf(stderr, "Error: Could not open file for writing\n");
        return 1;
    }

    // Allocate buffer for each sector
    uint8_t buffer[DISK_SECTOR_SIZE];

    // Write MBR (LBA 0)
    // fwrite(get_mbr(buffer), 1, DISK_SECTOR_SIZE, fp);

    // Write VBR (LBA 1)
    fwrite(get_vbr(buffer), 1, DISK_SECTOR_SIZE, fp);
    // Write FAT Table 1 (LBA 2)
    fwrite(get_fat(buffer), 1, DISK_SECTOR_SIZE, fp);

    // Write FAT Table 2 (LBA 3)
    fwrite(get_fat(buffer), 1, DISK_SECTOR_SIZE, fp);

    // Write Root Directory at Cluster 2 (LBA 4-19, 16 sectors = 8KB)
    fwrite(get_root_directory_sector(buffer), 1, DISK_SECTOR_SIZE, fp);
    // Fill remaining 15 sectors of root directory cluster with zeros
    memset(buffer, 0, DISK_SECTOR_SIZE);
    for (int i = 0; i < 15; i++) {
        fwrite(buffer, 1, DISK_SECTOR_SIZE, fp);
    }

    // Write to cluster 3, which is IMG1.JPG
    // Copy content from local file hard-drive-4699797_1280.jpg into file
    FILE *img1 = fopen("hard-drive-4699797_1280.jpg", "rb");
    if (img1 != NULL) {
        size_t bytes_read;
        while ((bytes_read = fread(buffer, 1, DISK_SECTOR_SIZE, img1)) > 0) {
            if (bytes_read < DISK_SECTOR_SIZE) {
                memset(buffer + bytes_read, 0, DISK_SECTOR_SIZE - bytes_read);
            }
            fwrite(buffer, 1, DISK_SECTOR_SIZE, fp);
        }
        fclose(img1);
        // Calculate how many sectors were written and fill rest with zeros if needed
        fseek(img1, 0, SEEK_END);
        long img1_size = ftell(img1);
        int sectors_written = (img1_size + DISK_SECTOR_SIZE - 1) / DISK_SECTOR_SIZE;
        int sectors_needed = 32 * 16; // 32 clusters * 16 sectors per cluster = 512 sectors
        memset(buffer, 0, DISK_SECTOR_SIZE);
        for (int i = sectors_written; i < sectors_needed; i++) {
            fwrite(buffer, 1, DISK_SECTOR_SIZE, fp);
        }
    } else {
        // Fallback: write dummy data
        memset(buffer, 0, DISK_SECTOR_SIZE);
        memcpy(buffer, "This is dummy data for IMG1.JPG", 32); // Dummy data
        fwrite(buffer, 1, DISK_SECTOR_SIZE, fp);
        // Fill remaining 15 sectors with zeros
        memset(buffer, 0, DISK_SECTOR_SIZE);
        for (int i = 0; i < 511; i++) {
            fwrite(buffer, 1, DISK_SECTOR_SIZE, fp);
        }
    }

    // Write to cluster 35, which is IMG2.JPG
    memset(buffer, 0, DISK_SECTOR_SIZE);
    memcpy(buffer, "Das ist IMG2.JPG! Jedoch nur ein dummy!", 40); // Dummy data
    fwrite(buffer, 1, DISK_SECTOR_SIZE, fp);
    // Fill remaining 15 sectors with zeros
    memset(buffer, 0, DISK_SECTOR_SIZE);
    for (int i = 0; i < 511; i++) {
        fwrite(buffer, 1, DISK_SECTOR_SIZE, fp);
    }

    // Fill rest of 1MB disk image with zeros (2048 sectors total - 1044 already written = 1004 sectors)
    for (int i = 0; i < 1004; i++) {
        fwrite(buffer, 1, DISK_SECTOR_SIZE, fp);
    }

    fclose(fp);
    printf("Successfully created wifidrv.img (1MB)\n");

    return 0;
}



/*
Ein Sektor hat 512 Bytes.
Das Image soll 1MB groß sein, also 2048 Sektoren.
Pro Cluster sind es 8KB, also 16 Sektoren.
Die FAT passt komplett in einen Sektor (2048 Sektoren / 16 Sektoren pro Cluster = 128 Cluster, 128 * 4 Bytes = 512 Bytes).

Das Image hat jetzt folgende Struktur:
- LBA 0: MBR (512 bytes)
- LBA 1: VBR (512 bytes)
- LBA 2: FAT Table 1 (512 bytes)
- LBA 3: FAT Table 2 (512 bytes)
- LBA 4-19: Root Directory (16 sectors = 8KB)
- LBA 20-531: Cluster 3 (IMG1.JPG, 256kB)
- LBA 532-1043: Cluster 35 (IMG2.JPG, 256kB)
- LBA 1044-2047: Unused (zeros)
*/
