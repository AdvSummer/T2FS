#include <t2fs.h>
#include <apidisk.h>
#include <bitmap2.h>

#include <stdbool.h>

static bool t2fs_init = false;

static struct t2fs_superbloco *superblock;

void initialize();
struct t2fs_superbloco* get_superblock();

void initialize() {
    superblock = get_superblock();

    t2fs_init = true;
}

struct t2fs_superbloco* get_superblock() {
    struct t2fs_superbloco* sb = malloc(sizeof(struct t2fs_superbloco));

    char sector[SECTOR_SIZE];
    if (read_sector(0, sector) != 0) {
        return 0;
    }

    int offset = 0;

    //id
    strncpy(sb->id, sector, 4);
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

    return sb;
}

int identify2(char *name, int size) {
    char names = "Leonardo Abreu Nahra: 242256\n" \
                 "Pedro Frederico Kampmann: 242244\n";
    strncpy(name, names, size);
    return 0;
}

FILE2 create2(char *filename) {
    if (!t2fs_init) {
        initialize();
    }
}

int delete2(char *filename) {
    if (!t2fs_init) {
        initialize();
    }
}

FILE2 open2(char *filename) {
    if (!t2fs_init) {
        initialize();
    }
}

int close2(FILE2 handle) {
    if (!t2fs_init) {
        initialize();
    }
}

int read2(FILE2 handle, char *buffer, int size) {
    if (!t2fs_init) {
        initialize();
    }
}

int write2(FILE2 handle, char *buffer, int size) {
    if (!t2fs_init) {
        initialize();
    }
}

int truncate2(FILE2 handle) {
    if (!t2fs_init) {
        initialize();
    }
}

int seek2(FILE2 handle, DWORD offset) {
    if (!t2fs_init) {
        initialize();
    }
}

int mkdir2(char *pathname) {
    if (!t2fs_init) {
        initialize();
    }
}

int rmdir2(char *pathname) {
    if (!t2fs_init) {
        initialize();
    }
}

DIR2 opendir2(char *pathname) {
    if (!t2fs_init) {
        initialize();
    }
}

int readdir2(DIR2 handle, DIRENT2 *dentry) {
    if (!t2fs_init) {
        initialize();
    }
}

int closedir2(DIR2 handle) {
    if (!t2fs_init) {
        initialize();
    }
}
