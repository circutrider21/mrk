#pragma once

#include <cstddef>
#include <cstdint>
#include <mrk/lock.h>

#define O_ACCMODE 0x0007
#define O_EXEC 1
#define O_RDONLY 2
#define O_RDWR 3
#define O_SEARCH 4
#define O_WRONLY 5

#define O_APPEND 0x0008
#define O_CREAT 0x0010
#define O_DIRECTORY 0x0020
#define O_EXCL 0x0040
#define O_NOCTTY 0x0080
#define O_NOFOLLOW 0x0100
#define O_TRUNC 0x0200
#define O_NONBLOCK 0x0400
#define O_DSYNC 0x0800
#define O_RSYNC 0x1000
#define O_SYNC 0x2000
#define O_CLOEXEC 0x4000

#define S_IFMT 0x0F000
#define S_IFBLK 0x06000
#define S_IFCHR 0x02000
#define S_IFIFO 0x01000
#define S_IFREG 0x08000
#define S_IFDIR 0x04000
#define S_IFLNK 0x0A000
#define S_IFSOCK 0x0C000
#define S_IFPIPE 0x03000

#define S_ISBLK(m) (((m)&S_IFMT) == S_IFBLK)
#define S_ISCHR(m) (((m)&S_IFMT) == S_IFCHR)
#define S_ISFIFO(m) (((m)&S_IFMT) == S_IFIFO)
#define S_ISREG(m) (((m)&S_IFMT) == S_IFREG)
#define S_ISDIR(m) (((m)&S_IFMT) == S_IFDIR)
#define S_ISLNK(m) (((m)&S_IFMT) == S_IFLNK)
#define S_ISSOCK(m) (((m)&S_IFMT) == S_IFSOCK)

namespace fs {
struct filesystem;

struct inode {
    size_t actual_size;

    int refcount; // Keeps track of how many refrences
    mutex lock;

    // Stat information
    uint64_t st_dev;
    uint64_t st_ino;
    int32_t st_mode;
    int32_t st_nlink;
    int32_t st_uid;
    int32_t st_gid;
    uint64_t st_rdev;
    int64_t st_size;

    int64_t st_atim_sec;
    long st_atim_nsec;
    int64_t st_mtim_sec;
    long st_mtim_nsec;
    int64_t st_ctim_sec;
    long st_ctim_nsec;

    int64_t st_blksize;
    int64_t st_blocks;

    int (*close)(struct fs::inode*);
    int64_t (*read)(struct fs::inode* self, void* buf, int64_t loc, size_t count);
    int64_t (*write)(struct fs::inode* self, const void* buf, int64_t loc, size_t count);
    bool (*grow)(struct fs::inode* self, size_t new_size);
    int (*ioctl)(struct fs::inode* self, int request, void* argp);
};

struct node {
    struct fs::node* redir;
    char name[128];
    char target[128];
    fs::inode* res;
    void* mount_data;
    uint64_t dev_id;
    struct fs::filesystem* fs;
    struct fs::node* mount_gate;
    struct fs::node* parent;
    struct fs::node* child;
    struct fs::node* next;
};

struct filesystem {
    const char* name;
    bool is_backed;

    struct fs::node* (*mount)(struct fs::inode* device);
    struct fs::node* (*populate)(struct fs::node* node);
    struct fs::inode* (*open)(struct fs::node* node, bool new_node, uint32_t mode);
    struct fs::inode* (*symlink)(struct fs::node* node);
    struct fs::inode* (*mkdir)(struct fs::node* node, uint32_t mode);
};

void init();
fs::node* create_node(fs::node* parent, const char* name, bool deep = false);
fs::node* mkdir(fs::node* parent, const char* name, uint32_t mode, bool do_loop);
fs::inode* open(fs::node** dir, fs::node* parent, const char* path, int oflags, uint32_t mode);
bool symlink(fs::node* parent, const char* target, const char* path);
bool mount(const char* source, const char* target, const char* fs_type);

filesystem* get_filesystem(const char* fs_name);
bool register_filesystem(fs::filesystem* fs);
}

extern fs::node* fs_root;
