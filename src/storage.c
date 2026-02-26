#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <Arduino.h>

#define DISK_SECTOR_SIZE (512)

extern const unsigned char DISK_MBR[];
extern const unsigned int DISK_MBR_len;
extern const unsigned char DISK_VBR[];
extern const unsigned int DISK_VBR_len;
extern const unsigned char DISK_FAT[];
extern const unsigned int DISK_FAT_len;
extern const unsigned char DISK_ROOTDIR[];
extern const unsigned int DISK_ROOTDIR_len;

extern const unsigned char FILE_IMG1_JPG[];
extern const unsigned int FILE_IMG1_JPG_len;
extern const unsigned char FILE_IMG2_JPG[];
extern const unsigned int FILE_IMG2_JPG_len;

// CREDS.JSN
// @LBA 2152 (0x0010d000)
// Total size is 2kB (4 sectors, resp. 1 clusters), fill up the rest with zeros
// Cluster 3
unsigned char FILE_CREDS_JSN[4 * DISK_SECTOR_SIZE]; // 2kB
unsigned char FILE_IMG_JPG[256 * DISK_SECTOR_SIZE]; // 128kB
unsigned int  FILE_IMG_JPG_len; // actual fetched size, read by storage.c

// extern volatile uint32_t http_fetch_trigger;


static void * get_file_lba(uint32_t offset, void * buffer, const unsigned char data[], uint32_t len)
{
    const int32_t rest = len - offset;
    // Entiere sector of data left
    if (rest >= DISK_SECTOR_SIZE)
    {
        memcpy(buffer, &data[offset], DISK_SECTOR_SIZE);
        return buffer;
    }
    // Some data left
    if (rest > 0)
    {
        memcpy(buffer, &data[offset], rest);
        memset(buffer + rest, 0, DISK_SECTOR_SIZE - rest);
        return buffer;
    }
    // Clear entire sectory
    memset(buffer, 0, DISK_SECTOR_SIZE);
    return buffer;
}


// Read 512 bytes from storage at given LBA into buffer
// Set buffer to zeros, if LBA address is "unknown"
static void * get_lba(uint32_t lba, void * buffer)
{
    if (lba == 0)
    {
        memcpy(buffer, DISK_MBR, DISK_SECTOR_SIZE);
        return buffer;
    }
    if (lba == 2048)
    {
        memcpy(buffer, DISK_VBR, DISK_SECTOR_SIZE);
        return buffer;
    }
    if ((lba == 2052) || (lba == 2084))
    {
        memcpy(buffer, DISK_FAT, DISK_SECTOR_SIZE);
        return buffer;
    }
    if (lba == 2116)
    {
        memcpy(buffer, DISK_ROOTDIR, DISK_SECTOR_SIZE);
        return buffer;
    }
    if ((lba >= 2152) && (lba < 2156))
    {
        return get_file_lba(DISK_SECTOR_SIZE * (lba - 2152), buffer, FILE_CREDS_JSN, sizeof(FILE_CREDS_JSN));
    }
    if ((lba >= 2156) && (lba < 2412))
    {
        // return get_file_lba(DISK_SECTOR_SIZE * (lba - 2412), buffer, FILE_IMG1_JPG, FILE_IMG1_JPG_len);
        return get_file_lba(DISK_SECTOR_SIZE * (lba - 2156), buffer, FILE_IMG_JPG, sizeof(FILE_IMG_JPG));
    }
    if ((lba >= 2412) && (lba < 2668))
    {
        // return get_file_lba(DISK_SECTOR_SIZE * (lba - 2412), buffer, FILE_IMG2_JPG, FILE_IMG2_JPG_len);
        return get_file_lba(DISK_SECTOR_SIZE * (lba - 2412), buffer, FILE_IMG_JPG, sizeof(FILE_IMG_JPG));
    }

    // Otherwise
    memset(buffer, 0, DISK_SECTOR_SIZE);
    return buffer;
}


// bufsize is expected to be a multiple of DISK_SECTOR_SIZE
uint32_t get_lba_slice(uint32_t lba, void * buffer, uint32_t bufsize)
{
    int32_t num = (int32_t)bufsize;
    while (num > 0)
    {
        get_lba(lba++, buffer);
        buffer += DISK_SECTOR_SIZE;
        num -= DISK_SECTOR_SIZE;
    }
    return bufsize;
}



// Write 512 bytes of data to storage at given LBA
static const void * set_lba(uint32_t lba, const void * data)
{
    if ((lba >= 2152) && (lba <= 2407))
    {
        const uint32_t offset = DISK_SECTOR_SIZE * (lba - 2152);
        memcpy(&FILE_CREDS_JSN[offset], data, DISK_SECTOR_SIZE);
    }
    return data;
}

// bufsize is expected to be a multiple of DISK_SECTOR_SIZE
uint32_t set_lba_slice(uint32_t lba, const void * data, uint32_t len)
{
    int32_t num = (int32_t)len;
    while (num > 0)
    {
        set_lba(lba++, data);
    }
    return len;
}


void prepare_files(void)
{
    memcpy(FILE_IMG_JPG, FILE_IMG1_JPG, FILE_IMG1_JPG_len);
    FILE_IMG_JPG_len = FILE_IMG1_JPG_len;
}