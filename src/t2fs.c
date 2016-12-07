#include <t2fs.h>
#include <apidisk.h>
#include <bitmap2.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define MAX_OPEN_FILES 20
#define RECORD_SIZE 64

typedef struct t2fs_superbloco superblock_t;
typedef struct t2fs_record record_t;
typedef struct t2fs_inode inode_t;

static bool t2fs_init = false;

static superblock_t *superblock = 0;
static record_t *root = 0;

static int inode_area = 0;
static int block_area = 0;

static struct files {
    record_t *dir;
    record_t *file;
    unsigned int p;
} files[MAX_OPEN_FILES] = {{0}};

static struct dirs {
    record_t *dir;
    int p;
} dirs[MAX_OPEN_FILES] = {{0}};

int initialize();
int get_superblock(superblock_t *sb);

int get_inode(int inode_number, inode_t *inode);
int set_inode(int inode_number, inode_t *inode);
int free_inode(int inode_number);

int get_record(int block_number, int record_number, record_t *file);
int set_record(int block_number, int record_number, record_t *file);

int get_ind(int block_number, int ind_number);
int set_ind(int block_number, int ind_number, int ind_block);

int load_file(char *filename, record_t *dir, record_t *file);
int load_dir(char *filename, record_t *file);
int load_block(char *filename, int block_number, record_t *file);
int load_single_ind(char *filename, int block_number, record_t *file);
int load_double_ind(char *filename, int block_number, record_t *file);

int save_file(record_t *file, record_t *dir);
int save_block(record_t *file, int block_number);
int save_single_ind(record_t *file, int block_number);
int save_double_ind(record_t *file, int block_number);

int get_n_block (inode_t *inode, int n, int block_number);
int read_from_sector( int sector_number, char *buffer, int n);
int read_from_block( int block_number, char *buffer, int n);

int read2 (FILE2 handle, char *buffer, int size);
int write2 (FILE2 handle, char *buffer, int size);
int truncate2 (FILE2 handle);
int seek2 (FILE2 handle, unsigned int offset);

int search_free_inode();

int initialize() {
    superblock = (superblock_t*)malloc(sizeof(superblock_t));
    if (get_superblock(superblock) != 0) {
        free(superblock);
        return -1;
    }

    inode_area = superblock->superblockSize
                 + superblock->freeInodeBitmapSize
                 + superblock->freeBlocksBitmapSize;
    block_area = inode_area + superblock->inodeAreaSize;

    root = (record_t*)malloc(sizeof(record_t));
    root->TypeVal = TYPEVAL_DIRETORIO;
    strncpy(root->name, "/\0", 2);
    root->blocksFileSize = 1;
    root->bytesFileSize = superblock->blockSize*SECTOR_SIZE;
    root->inodeNumber = 0;

    t2fs_init = true;

    return 0;
}

int get_superblock(superblock_t* sb) {
    unsigned char sector[SECTOR_SIZE];
    if (read_sector(0, sector) != 0) {
        return -1;
    }

    int offset = 0;

    //id
    memcpy(sb->id, sector, 4);
    offset += 4;

    //version
    sb->version = sector[offset] | sector[offset + 1] << 8;
    offset += 2;

    //superblock size
    sb->superblockSize = sector[offset] | sector[offset + 1] << 8;
    offset += 2;

    //block bitmap size
    sb->freeBlocksBitmapSize = sector[offset] | sector[offset + 1] << 8;
    offset += 2;

    //i-node bitmap size
    sb->freeInodeBitmapSize = sector[offset] | sector[offset + 1] << 8;
    offset += 2;

    //i-node area size
    sb->inodeAreaSize = sector[offset] | sector[offset + 1] << 8;
    offset += 2;

    //block size
    sb->blockSize = sector[offset] | sector[offset + 1] << 8;
    offset += 2;

    //disk size
    sb->diskSize = sector[offset]
                   | sector[offset + 1] << 8
                   | sector[offset + 2] << 16
                   | sector[offset + 3] << 24;

    return 0;
}

int get_inode(int inode_number, inode_t *inode) {
    unsigned char sector[SECTOR_SIZE];
    if (read_sector(inode_area + inode_number / 16, sector) != 0) {
        return -1;
    }

    int offset = (inode_number % 16) * sizeof(inode_t);
    inode->dataPtr[0] = sector[offset]
                        | sector[offset + 1] << 8
                        | sector[offset + 2] << 16
                        | sector[offset + 3] << 24;
    offset += 4;

    inode->dataPtr[1] = sector[offset]
                        | sector[offset + 1] << 8
                        | sector[offset + 2] << 16
                        | sector[offset + 3] << 24;
    offset += 4;

    inode->singleIndPtr = sector[offset]
                          | sector[offset + 1] << 8
                          | sector[offset + 2] << 16
                          | sector[offset + 3] << 24;
    offset += 4;

    inode->doubleIndPtr = sector[offset]
                          | sector[offset + 1] << 8
                          | sector[offset + 2] << 16
                          | sector[offset + 3] << 24;

    return 0;
}

int set_inode(int inode_number, inode_t *inode) {
    unsigned char sector[SECTOR_SIZE];
    int sector_number = inode_area + inode_number / 16;
    if (read_sector(sector_number, sector) != 0) {
        return -1;
    }

    int offset = (inode_number % 16) * sizeof(inode_t);
    sector[offset++] = inode->dataPtr[0] & 0xFF;
    sector[offset++] = (inode->dataPtr[0] >> 8) & 0xFF;
    sector[offset++] = (inode->dataPtr[0] >> 16) & 0xFF;
    sector[offset++] = (inode->dataPtr[0] >> 24) & 0xFF;

    sector[offset++] = inode->dataPtr[1] & 0xFF;
    sector[offset++] = (inode->dataPtr[1] >> 8) & 0xFF;
    sector[offset++] = (inode->dataPtr[1] >> 16) & 0xFF;
    sector[offset++] = (inode->dataPtr[1] >> 24) & 0xFF;

    sector[offset++] = inode->singleIndPtr & 0xFF;
    sector[offset++] = (inode->singleIndPtr >> 8) & 0xFF;
    sector[offset++] = (inode->singleIndPtr >> 16) & 0xFF;
    sector[offset++] = (inode->singleIndPtr >> 24) & 0xFF;

    sector[offset++] = inode->doubleIndPtr & 0xFF;
    sector[offset++] = (inode->doubleIndPtr >> 8) & 0xFF;
    sector[offset++] = (inode->doubleIndPtr >> 16) & 0xFF;
    sector[offset++] = (inode->doubleIndPtr >> 24) & 0xFF;

    if (write_sector(sector_number, sector) != 0) {
        return -1;
    }

    return 0;
}

int free_inode(int inode_number) {
    inode_t inode;
    if (get_inode(inode_number, &inode) != 0) {
        return -1;
    }
    setBitmap2(BITMAP_INODE, inode_number, 0);

    if (inode.dataPtr[0] == INVALID_PTR) {
        return 0;
    }
    setBitmap2(BITMAP_DADOS, inode.dataPtr[0], 0);

    if (inode.dataPtr[1] == INVALID_PTR) {
        return 0;
    }
    setBitmap2(BITMAP_DADOS, inode.dataPtr[1], 0);

    if (inode.singleIndPtr == INVALID_PTR) {
        return 0;
    }
    setBitmap2(BITMAP_DADOS, inode.singleIndPtr, 0);

    int i;
    int j;
    int s_ind;
    int d_ind;
    for (i = 0; i < 1024; ++i) {
        s_ind = get_ind(inode.singleIndPtr, i);
        if (s_ind == INVALID_PTR) {
            return 0;
        }
        setBitmap2(BITMAP_DADOS, s_ind, 0);
    }

    if (inode.doubleIndPtr == INVALID_PTR) {
        return 0;
    }
    setBitmap2(BITMAP_DADOS, inode.doubleIndPtr, 0);

    for (i = 0; i < 1024; ++i) {
        d_ind = get_ind(inode.doubleIndPtr, i);
        if (d_ind == INVALID_PTR) {
            return 0;
        }
        setBitmap2(BITMAP_DADOS, d_ind, 0);
        for (j = 0; j < 1024; ++j) {
            s_ind = get_ind(d_ind, j);
            if (s_ind == INVALID_PTR) {
                return 0;
            }
            setBitmap2(BITMAP_DADOS, s_ind, 0);
        }
    }

    return 0;
}

int get_record(int block_number, int record_number, record_t* file) {
    unsigned char sector[SECTOR_SIZE];
    unsigned int sector_number = block_area
                                 + block_number * superblock->blockSize
                                 + record_number / 4;
    if (read_sector(sector_number, sector) != 0) {
        return -1;
    }

    int offset = (record_number % 4) * RECORD_SIZE;
    file->TypeVal = sector[offset];
    offset += 1;

    if (file->TypeVal == TYPEVAL_INVALIDO) {
        return -1;
    }

    memcpy(file->name, sector + offset, 32);
    offset += 32;

    file->blocksFileSize = sector[offset]
                           | sector[offset + 1] << 8
                           | sector[offset + 2] << 16
                           | sector[offset + 3] << 24;
    offset += 4;

    file->bytesFileSize = sector[offset]
                          | sector[offset + 1] << 8
                          | sector[offset + 2] << 16
                          | sector[offset + 3] << 24;
    offset += 4;

    file->inodeNumber = sector[offset]
                        | sector[offset + 1] << 8
                        | sector[offset + 2] << 16
                        | sector[offset + 3] << 24;

    return 0;
}

int set_record(int block_number, int record_number, record_t *file) {
    unsigned char sector[SECTOR_SIZE];
    unsigned int sector_number = block_area
                                 + block_number * superblock->blockSize
                                 + record_number / 4;
    if (read_sector(sector_number, sector) != 0) {
        return -1;
    }

    int offset = (record_number % 4) * RECORD_SIZE;
    sector[offset++] = file->TypeVal;

    memcpy(sector + offset, file->name, 32);
    offset += 32;

    sector[offset++] = file->blocksFileSize & 0xFF;
    sector[offset++] = (file->blocksFileSize >> 8) & 0xFF;
    sector[offset++] = (file->blocksFileSize >> 16) & 0xFF;
    sector[offset++] = (file->blocksFileSize >> 24) & 0xFF;

    sector[offset++] = file->bytesFileSize & 0xFF;
    sector[offset++] = (file->bytesFileSize >> 8) & 0xFF;
    sector[offset++] = (file->bytesFileSize >> 16) & 0xFF;
    sector[offset++] = (file->bytesFileSize >> 24) & 0xFF;

    sector[offset++] = file->inodeNumber & 0xFF;
    sector[offset++] = (file->inodeNumber >> 8) & 0xFF;
    sector[offset++] = (file->inodeNumber >> 16) & 0xFF;
    sector[offset++] = (file->inodeNumber >> 24) & 0xFF;

    if (write_sector(sector_number, sector) != 0) {
        return -1;
    }

    return 0;
}

int get_ind(int block_number, int ind_number) {
    unsigned char sector[SECTOR_SIZE];
    unsigned int sector_number = block_area
                                 + block_number * superblock->blockSize
                                 + ind_number / 64;
    if (read_sector(sector_number, sector) != 0) {
        return -1;
    }

    int offset = (ind_number % 64) * 4;
    int ind = sector[offset]
              | sector[offset + 1] << 8
              | sector[offset + 2] << 16
              | sector[offset + 3] << 24;

    return ind;
}

int set_ind(int block_number, int ind_number, int ind_block) {
    unsigned char sector[SECTOR_SIZE];
    unsigned int sector_number = block_area
                                 + block_number * superblock->blockSize
                                 + ind_number / 64;
    if (read_sector(sector_number, sector) != 0) {
        return -1;
    }

    int offset = (ind_number % 64) * 4;
    sector[offset++] = ind_block & 0xFF;
    sector[offset++] = (ind_block >> 8) & 0xFF;
    sector[offset++] = (ind_block >> 16) & 0xFF;
    sector[offset++] = (ind_block >> 24) & 0xFF;

    if (write_sector(sector_number, sector) != 0) {
        return -1;
    }

    return 0;
}

int load_file(char *filename, record_t *dir, record_t *file) {
    if (filename[0] != '/') {
        printf("not an absolute path\n");
        return -1;
    }
    printf("load file %s\n", filename);

    *file = *root;
    char *buffer = (char*)malloc(sizeof(char) * strlen(filename) + 1);
    char *begin = filename + 1;
    char *end;
    do {
        end = strchr(begin, '/');
        if (end == 0) {
            *dir = *file;
            end = begin + strlen(begin);
        }

        memcpy(buffer, begin, end - begin);
        buffer[end - begin] = 0;

        printf("search file %s\n", buffer);
        if (load_dir(buffer, file) != 0) {
            printf("file %s not found\n", buffer);
            free(buffer);
            return -1;
        } else {
            begin = end + 1;
        }
    } while (end != filename + strlen(filename));

    printf("files %s and %s opened\n", dir->name, file->name);

    free(buffer);
    return 0;
}

int load_dir(char *filename, record_t *file) {
    if (file->TypeVal != TYPEVAL_DIRETORIO) {
        return -1;
    }

    inode_t inode;
    if (get_inode(file->inodeNumber, &inode) != 0) {
        printf("inode %d is invalid\n", file->inodeNumber);
        return -1;
    }

    if (load_block(filename, inode.dataPtr[0], file) == 0) {
        printf("file %s found on block %d\n", filename, inode.dataPtr[0]);
        return 0;
    }

    if (load_block(filename, inode.dataPtr[1], file) == 0) {
        return 0;
    }

    if (load_single_ind(filename, inode.singleIndPtr, file) == 0) {
        return 0;
    }

    if (load_double_ind(filename, inode.doubleIndPtr, file) == 0) {
        return 0;
    }

    return -1;
}

int load_block(char *filename, int block_number, record_t *file) {
    if (block_number == INVALID_PTR) {
        return -1;
    }

    int i;
    for (i = 0; i < 64; ++i) {
        if (get_record(block_number, i, file) == 0) {
            printf("compare %s %s\n", file->name, filename);
            if (strcmp(file->name, filename) == 0) {
                return 0;
            }
        }
    }

    return -1;
}

int load_single_ind(char *filename, int block_number, record_t *file) {
    if (block_number == INVALID_PTR) {
        return -1;
    }

    int i;
    int ind;
    for (i = 0; i < 1024; ++i) {
        ind = get_ind(block_number, i);
        if (ind == INVALID_PTR) {
            return -1;
        }

        if (load_block(filename, ind, file) == 0) {
            return 0;
        }
    }

    return -1;
}

int load_double_ind(char *filename, int block_number, record_t *file) {
    if (block_number == INVALID_PTR) {
        return -1;
    }

    int i;
    int ind;
    for (i = 0; i < 1024; ++i) {
        ind = get_ind(block_number, i);
        if (ind == INVALID_PTR) {
            return -1;
        }

        if (load_single_ind(filename, ind, file) == 0) {
            return 0;
        }
    }

    return -1;
}

int save_file(record_t *file, record_t *dir) {
    if (dir->TypeVal != TYPEVAL_DIRETORIO) {
        return -1;
    }
    printf("save %s in %s\n", file->name, dir->name);
    inode_t inode;
    if (get_inode(dir->inodeNumber, &inode) != 0) {
        return -1;
    }
    printf("save record in inode %d\n", dir->inodeNumber);
    if (inode.dataPtr[0] == INVALID_PTR) {
        inode.dataPtr[0] = searchBitmap2(BITMAP_DADOS, 0);
    }
    if (save_block(file, inode.dataPtr[0]) != 0) {

        if (inode.dataPtr[0] == INVALID_PTR) {
            inode.dataPtr[0] = searchBitmap2(BITMAP_DADOS, 0);
        }
        if (save_block(file, inode.dataPtr[1]) != 0) {

            if (inode.singleIndPtr == INVALID_PTR) {
                inode.singleIndPtr = searchBitmap2(BITMAP_DADOS, 0);
            }
            if (save_single_ind(file, inode.singleIndPtr) != 0) {

                if (inode.doubleIndPtr == INVALID_PTR) {
                    inode.doubleIndPtr = searchBitmap2(BITMAP_DADOS, 0);
                }
                if (save_double_ind(file, inode.doubleIndPtr) != 0) {
                    return -1;
                }
            }
        }
    }

    set_inode(dir->inodeNumber, &inode);

    return 0;
}

int save_block(record_t *file, int block_number) {
    if (block_number == INVALID_PTR) {
        return -1;
    }

    int i;
    record_t record;
    for (i = 0; i < 64; ++i) {
        if (get_record(block_number, i, &record) != 0 ||
            strcmp(file->name, record.name) == 0) {
            if (set_record(block_number, i, file) == 0) {
                return 0;
            }
        }
    }

    return -1;
}

int save_single_ind(record_t *file, int block_number) {
    if (block_number == INVALID_PTR) {
        return -1;
    }

    int i;
    int ind;
    for (i = 0; i < 1024; ++i) {
        ind = get_ind(block_number, i);
        if (ind == INVALID_PTR) {
            ind = searchBitmap2(BITMAP_DADOS, 0);
            set_ind(block_number, i, ind);
        }
        if (save_block(file, ind) == 0) {
            return 0;
        }
    }

    return -1;
}

int save_double_ind(record_t *file, int block_number) {
    if (block_number == INVALID_PTR) {
        return -1;
    }

    int i;
    int ind;
    for (i = 0; i < 1024; ++i) {
        ind = get_ind(block_number, i);
        if (ind == INVALID_PTR) {
            ind = searchBitmap2(BITMAP_DADOS, 0);
            set_ind(block_number, i, ind);
        }
        if (save_single_ind(file, ind) == 0) {
            return 0;
        }
    }

    return -1;
}

int identify2(char *name, int size) {
    const char *names = "Leonardo Abreu Nahra: 242256\n" \
                        "Pedro Frederico Kampmann: 242244\n";
    strncpy(name, names, size);
    return 0;
}

FILE2 create2(char *filename) {
    if (!t2fs_init) {
        initialize();
    }

    if (filename[0] != '/') {
        printf("not an absolute path\n");
        return -1;
    }

    int i;
    for (i = 0; i < MAX_OPEN_FILES; ++i) {
        if (files[i].file == 0) {
            break;
        }
    }
    if (i == MAX_OPEN_FILES) {
        return -1;
    }

    int inode_number = searchBitmap2(BITMAP_INODE, 0);
    if (inode_number < 0) {
        return -1;
    }
    printf("inode %d is free\n", inode_number);

    record_t *dir = (record_t*)malloc(RECORD_SIZE);
    record_t *file = (record_t*)malloc(RECORD_SIZE);
    if (load_file(filename, dir, file) == 0) {
        printf("file %s already exists\n", filename);
        free(dir);
        free(file);
        return -1;
    }

    char *begin = filename + 1;
    char *end;
    do {
        end = strchr(begin, '/');
        if (end == 0) {
            end = begin + strlen(begin);
            break;
        }
        begin = end + 1;
    } while (end != filename + strlen(filename));

    file->TypeVal = TYPEVAL_REGULAR;
    memcpy(file->name, begin, end - begin);
    file->name[end - begin] = 0;
    file->blocksFileSize = 0;
    file->bytesFileSize = 0;
    file->inodeNumber = inode_number;

    inode_t inode;
    inode.dataPtr[0] = INVALID_PTR;
    inode.dataPtr[1] = INVALID_PTR;
    inode.singleIndPtr = INVALID_PTR;
    inode.doubleIndPtr = INVALID_PTR;

    printf("saving inode %d\n", inode_number);
    if (set_inode(inode_number, &inode) != 0) {
        free(dir);
        free(file);
        return -1;
    }

    printf("saving file %s in %s\n", file->name, dir->name);
    if (save_file(file, dir) != 0) {
        free(dir);
        free(file);
        return -1;
    }

    if (setBitmap2(BITMAP_INODE, inode_number, 1) != 0) {
        free(dir);
        free(file);
        return -1;
    }

    files[i].dir = dir;
    files[i].file = file;
    files[i].p = 0;

    return i;
}

int delete2(char *filename) {
    if (!t2fs_init) {
        initialize();
    }

    record_t dir;
    record_t file;
    if (load_file(filename, &dir, &file) != 0) {
        printf("file %s doesn't exist\n", filename);
        return -1;
    }

    file.TypeVal = TYPEVAL_INVALIDO;
    free_inode(file.inodeNumber);

    if (save_file(&file, &dir) != 0) {
        return -1;
    }

    return 0;
}

FILE2 open2(char *filename) {
    if (!t2fs_init) {
        initialize();
    }

    int i;
    for (i = 0; i < MAX_OPEN_FILES; ++i) {
        if (files[i].file == 0) {
            break;
        }
    }
    if (i == MAX_OPEN_FILES) {
        return -1;
    }

    record_t *dir = (record_t*)malloc(RECORD_SIZE);
    record_t *file = (record_t*)malloc(RECORD_SIZE);
    if (load_file(filename, dir, file) == 0 &&
        file->TypeVal == TYPEVAL_REGULAR) {
        files[i].dir = dir;
        files[i].file = file;
        files[i].p = 0;
        return i;
    } else {
        free(file);
        free(dir);
        return -1;
    }
}

int close2(FILE2 handle) {
    if (!t2fs_init) {
        initialize();
    }

    record_t *file = files[handle].file;
    record_t *dir = files[handle].dir;

    if (file == 0) {
        printf("no file opened with handle %d\n", handle);
        return -1;
    }

    if (save_file(file, dir) != 0) {
        return -1;
    }

    free(file);
    free(dir);
    file = 0;
    dir = 0;

    return 0;
}

int get_n_block(inode_t *inode, int n, int block_number) {
    if ( n < 0) {
       return -1;
    }
    printf("get_n_block\n");
    if (n < 2) {
       if (inode->dataPtr[n] == INVALID_PTR) {
          return -1;
       }
       block_number = inode->dataPtr[n];
       return 0;
    }
    if (n < 1026) {
       if (inode->singleIndPtr == INVALID_PTR) {
          return -1;
       }
       block_number = get_ind(inode->singleIndPtr, n-2);
       return 0;
    }
    if (inode->doubleIndPtr == INVALID_PTR) {
       return -1;
    }
    
    int d_index = (n-2 / 1024) -1;
    int p_d = get_ind(inode->doubleIndPtr, d_index);

    if (p_d == INVALID_PTR) {
       return -1;
    }

    int dd_index = (n-1026) - (d_index*1024);
    block_number = get_ind(p_d, dd_index);
    return 0;
}

int read_from_sector( int sector_number, char *buffer, int n) { //read n bytes from sector
    int read = 0;
    unsigned char sector[SECTOR_SIZE];

    if (read_sector(sector_number, sector) != 0) {
        return -1;
    }
    int i;
    for (i=0; i< SECTOR_SIZE; ++i) {
    	if (read >= n) {
    		break;
    	}
    	buffer[read] = sector[i];
    	read++;
    }
    return read; //returns either n or SECTOR_SIZE bytes
}

int read_from_block( int block_number, char *buffer, int n) { //read n bytes from block
	int read = 0;
	int counter;
    unsigned int sector_number = block_area
                                + block_number * superblock->blockSize;
    int i = 0;
    while (read < n && i < superblock->blockSize) {
    	counter = read_from_sector(sector_number+i, buffer+read, n);
    	if(counter < 0) {
    		return -1;
    	}
    	read += counter;
    	i++;
    	n -= read;
    }
    return read; //returns either n bytes or blockSize*SECTOR_SIZE (full block) bytes
}

int read2(FILE2 handle, char *buffer, int size) {
    if (!t2fs_init) {
        initialize();
    }

    record_t *file = files[handle].file;
    unsigned int offset = files[handle].p;
    if (file == 0) {
        printf("no file opened with handle %d\n", handle);
        return -1;
    }

    if (offset + (unsigned int) size >= file->bytesFileSize) {

       printf("cannot read %d bytes from the file\n", size);
       return -1;
    }

    inode_t inode;
    if (get_inode(file->inodeNumber, &inode) != 0) {
       printf("error opening the file's inode\n");
       return -1;
    }

    int index = (offset / superblock->blockSize*SECTOR_SIZE);
    if (index > 0) {
    	index-=1;
    }
    int i = 0;
    int counter = 0;
    int read=0;
    int block_number = 0;

    while (read < size){
    	if (get_n_block(&inode, index+i, block_number) != 0) {
           return -1;
    	}
    	counter = read_from_block(block_number, buffer+read, size);
    	if (counter < 0) {
    		return -1;
    	}
    	read += counter;
    	i++;
	}
 
 	seek2(handle, offset+read);
    return read-size;
}

int write2(FILE2 handle, char *buffer, int size) {
    if (!t2fs_init) {
        initialize();
    }

    record_t *file = files[handle].file;
    if (file == 0) {
        printf("no file opened with handle %d\n", handle);
        return -1;
    }


//atualizar tamanho do arquivo em bytes, em blocos
    return 0;
}

int delete_from_block(int *block_number, int size, int begin, int end) {
    
return 0;
}

int truncate2(FILE2 handle) {
    if (!t2fs_init) {
       initialize();
    }

    return -1;
}

int seek2(FILE2 handle, unsigned int offset) {
    if (!t2fs_init) {
        initialize();
    }

    record_t *file = files[handle].file;
    if (file == 0) {
        printf("no file opened with handle %d\n", handle);
        return -1;
    }

    if (offset >= file->bytesFileSize) {
       return -1;
    }

    files[handle].p = offset;
    return 0;
}

int mkdir2(char *pathname) {
    if (!t2fs_init) {
        initialize();
    }

    if (pathname[0] != '/') {
        printf("not an absolute path\n");
        return -1;
    }

    int inode_number = searchBitmap2(BITMAP_INODE, 0);
    if (inode_number < 0) {
        return -1;
    }
    printf("inode %d is free\n", inode_number);

    record_t *dir = (record_t*)malloc(RECORD_SIZE);
    record_t *file = (record_t*)malloc(RECORD_SIZE);
    if (load_file(pathname, dir, file) == 0) {
        printf("dir %s already exists\n", pathname);
        free(dir);
        free(file);
        return -1;
    }

    char *begin = pathname + 1;
    char *end;
    do {
        end = strchr(begin, '/');
        if (end == 0) {
            end = begin + strlen(begin);
            break;
        }
        begin = end + 1;
    } while (end != pathname + strlen(pathname));

    file->TypeVal = TYPEVAL_DIRETORIO;
    memcpy(file->name, begin, end - begin);
    file->name[end - begin] = 0;
    file->blocksFileSize = 1;
    file->bytesFileSize = superblock->blockSize*SECTOR_SIZE;
    file->inodeNumber = inode_number;

    inode_t inode;
    inode.dataPtr[0] = INVALID_PTR;
    inode.dataPtr[1] = INVALID_PTR;
    inode.singleIndPtr = INVALID_PTR;
    inode.doubleIndPtr = INVALID_PTR;

    printf("saving inode %d\n", inode_number);
    if (set_inode(inode_number, &inode) != 0) {
        free(dir);
        free(file);
        return -1;
    }

    printf("saving file %s in %s\n", file->name, dir->name);
    if (save_file(file, dir) != 0) {
        free(dir);
        free(file);
        return -1;
    }

    if (setBitmap2(BITMAP_INODE, inode_number, 1) != 0) {
        free(dir);
        free(file);
        return -1;
    }

    return 0;
}

int rmdir2(char *pathname) {
    if (!t2fs_init) {
        initialize();
    }

    record_t dir;
    record_t file;
    if (load_file(pathname, &dir, &file) != 0) {
        printf("dir %s doesn't exist\n", pathname);
        return -1;
    }

    file.TypeVal = TYPEVAL_INVALIDO;
    free_inode(file.inodeNumber);

    if (save_file(&file, &dir) != 0) {
        return -1;
    }

    return 0;
}

DIR2 opendir2(char *pathname) {
    if (!t2fs_init) {
        initialize();
    }

    int i;
    for (i = 0; i < MAX_OPEN_FILES; ++i) {
        if (dirs[i].dir == 0) {
            break;
        }
    }
    if (i == MAX_OPEN_FILES) {
        return -1;
    }

    record_t dir;
    record_t *file = (record_t*)malloc(RECORD_SIZE);
    if (load_file(pathname, &dir, file) == 0 &&
        file->TypeVal == TYPEVAL_DIRETORIO) {
        dirs[i].dir = file;
        dirs[i].p = 0;
        return i;
    } else {
        free(file);
        return -1;
    }

    return 0;
}

int readdir2(DIR2 handle, DIRENT2 *dentry) {
    if (!t2fs_init) {
        initialize();
    }

    record_t *dir = dirs[handle].dir;
    if (dir == 0) {
        printf("no dir opened with handle %d\n", handle);
        return -1;
    }

    inode_t inode;
    if (get_inode(dir->inodeNumber, &inode) != 0) {
        return -1;
    }

    int p = dirs[handle].p;
    record_t file;
    if (get_record(inode.dataPtr[0], p, &file) != 0) {
        return -1;
    }

    dirs[handle].p++;
    memcpy(dentry->name, file.name, strlen(file.name));
    dentry->name[strlen(file.name)] = 0;
    dentry->fileType = file.TypeVal;
    dentry->fileSize = file.bytesFileSize;

    return 0;
}

int closedir2(DIR2 handle) {
    if (!t2fs_init) {
        initialize();
    }

    record_t *dir = dirs[handle].dir;
    if (dir == 0) {
        printf("no dir opened with handle %d\n", handle);
        return -1;
    }

    free(dir);
    dir = 0;

    return 0;
}
