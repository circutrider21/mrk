#include <internal/builtin.h>
#include <internal/stivale2.h>
#include <klib/vector.h>
#include <mrk/fs.h>
#include <mrk/log.h>

fs::node* fs_root;
static klib::vector<fs::filesystem*>* filesystems;

bool fs::register_filesystem(fs::filesystem* fs)
{
    filesystems->push_back(fs);
    return true;
}

fs::filesystem* fs::get_filesystem(const char* fs_name)
{
    for (auto e : (*filesystems)) {
        if (!_strcmp(e->name, fs_name))
            return e;
    }

    log("fs: Filesystem %s does not exist or is not yet registered!\n", fs_name);
    return nullptr;
}

struct ustar_header {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char checksum[8];
    uint8_t type;
    char link_name[100];
    char signature[6];
    char version[2];
    char owner[32];
    char group[32];
    char device_maj[8];
    char device_min[8];
    char prefix[155];
};

enum {
    USTAR_FILE = '0',
    USTAR_HARD_LINK = '1',
    USTAR_SYM_LINK = '2',
    USTAR_CHAR_DEV = '3',
    USTAR_BLOCK_DEV = '4',
    USTAR_DIR = '5',
    USTAR_FIFO = '6'
};

static uintptr_t initramfs_addr;
static uintptr_t initramfs_size;
extern fs::filesystem tmpfs;

static uint64_t octal_to_int(const char* s)
{
    uint64_t ret = 0;
    while (*s) {
        ret *= 8;
        ret += *s - '0';
        s++;
    }
    return ret;
}

void fs::init()
{
    filesystems = new klib::vector<fs::filesystem*>();
    fs_root = new fs::node();
    *fs_root = { 0 };
    fs_root->name[0] = '/';

    fs::register_filesystem(&tmpfs);
    fs::mount("tmpfs", "/", "tmpfs");

    // Mount the initramfs
    stivale2_struct_tag_modules* modules_tag = (stivale2_struct_tag_modules*)arch::stivale2_get_tag(STIVALE2_STRUCT_TAG_MODULES_ID);
    if (modules_tag->module_count < 1) {
        PANIC("Initramfs required for boot!\n");
    }

    stivale2_module* module = &modules_tag->modules[0];

    initramfs_addr = module->begin;
    initramfs_size = module->end - module->begin;

    ustar_header* h = (ustar_header*)initramfs_addr;
    for (;;) {
        if (_memcmp(h->signature, "ustar", 5) != 0)
            break;

        uintptr_t size = octal_to_int(h->size);

        switch (h->type) {
        case USTAR_DIR: {
            fs::mkdir(nullptr, h->name, octal_to_int(h->mode), false);
            break;
        }
        case USTAR_FILE: {
            fs::inode* r = fs::open(nullptr, nullptr, h->name, O_RDWR | O_CREAT,
                octal_to_int(h->mode));
            void* buf = (void*)((uint64_t)h + 512);
            r->write(r, buf, 0, size);
            r->close(r);
            break;
        }
        case USTAR_SYM_LINK: {
            fs::symlink(nullptr, h->link_name, h->name);
            break;
        }
        }

        h = (ustar_header*)h + 512 + ALIGN_UP(size, 512);

        if ((uintptr_t)h >= initramfs_addr + initramfs_size)
            break;
    }

    log("fs: Imported Initramfs!\n");
}
