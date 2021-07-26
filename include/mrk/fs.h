#pragma once

#include <klib/vector.h>

// Max length definitions
#define MAX_PATH_LEN 4096
#define MAX_NAME_LEN 256

namespace fs {
    // Diffrent types of vfs nodes
    enum class node_type : uint8_t {
        file,
        directory,
        link,
        pipe,
        blockdev,
        chardev,
        mountpoint
    };

    enum class open_mode : uint8_t {
        read,
        write,
        readwrite
    };

    struct filesystem {
        char name[16];
        bool no_backing; // is it backed by a device or memory ?

        // Required to be implemented by the filesystem
        virtual fs::inode* mount(fs::inode* device) const = 0;
        virtual int64_t mknode(fs::tnode* n) const = 0;
        virtual int64_t read(fs::inode* n, size_t offset, size_t len, void* buff) const = 0;
        virtual int64_t write(fs::inode* n, size_t offset, size_t len, const void* buff) const = 0;
        virtual int64_t sync(fs::inode* n) const = 0;
        virtual int64_t refresh(fs::inode* n) const = 0;
        virtual int64_t setlink(fs::tnode* n, fs::inode* target) const = 0;
        virtual int64_t ioctl(fs::inode* n, int64_t req_param, void* req_data) const = 0;
    };

    struct tnode {
        char name[MAX_NAME_LEN];
        fs::inode* inode;
        fs::inode* parent;
	fs::tnode* sibling;
    };

    struct fs::inode {
        vfs_node_type_t type;
        size_t size;
        uint32_t perms;
        uint32_t uid;
        uint32_t refcount;
        vfs_fsinfo_t* fs;
        void* ident;
        vfs_tnode_t* mountpoint;
        klib::vector<fs::tnode*> child;
    };

// structure describing an open node
typedef struct {
    vfs_tnode_t* tnode;
    fs::inode* inode;
    vfs_openmode_t mode;
    size_t seek_pos;
} vfs_node_desc_t;

// directory entry structure
typedef struct {
    vfs_node_type_t type;
    size_t record_len;
    char name[VFS_MAX_NAME_LEN];
} vfs_dirent_t;

void vfs_init();
void vfs_register_fs(vfs_fsinfo_t* fs);
vfs_fsinfo_t* vfs_get_fs(char* name);
void vfs_debug();

    fs::handle open(char* path, open_mode mode);
    int64_t create(char* path, node_type type);
    int64_t close(fs::handle handle);
    int64_t seek(fs::handle handle, size_t pos);
    int64_t read(fs::handle handle, size_t len, void* buff);
    int64_t write(fs::handle handle, size_t len, const void* buff);
    int64_t chmod(fs::handle handle, int32_t newperms);
    int64_t link(char* oldpath, char* newpath);
    int64_t unlink(char* path);
    int64_t getdent(fs::handle handle, fs::dirent* dirent);
    int64_t mount(char* device, char* path, char* fsname);
}

