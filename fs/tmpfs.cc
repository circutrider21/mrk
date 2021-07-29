#include <internal/builtin.h>
#include <mrk/alloc.h>
#include <mrk/fs.h>
#include <mrk/lock.h>

static mutex tmpfs_lock;

struct tmpfs_res : public fs::inode {
    size_t allocated_size;
    char* data;
};

extern fs::filesystem tmpfs;

static fs::node* tmpfs_mount(fs::inode* device)
{
    (void)device;

    fs::node* mount_gate = new fs::node;

    mount_gate->fs = &tmpfs;

    mount_gate->mount_data = mm::alloc(sizeof(uint64_t));

    *((uint64_t*)mount_gate->mount_data) = 1;

    fs::inode* res = (fs::inode*)mm::alloc(sizeof(tmpfs_res));
    res->actual_size = sizeof(tmpfs_res);
    res->st_mode = 0755 | S_IFDIR;
    res->st_ino = (*((uint64_t*)mount_gate->mount_data))++;
    res->st_blksize = 512;
    res->st_nlink = 1;

    mount_gate->res = res;

    return mount_gate;
}

static int64_t tmpfs_read(fs::inode* _this, void* buf, int64_t off, size_t count)
{
    tmpfs_res* self = (tmpfs_res*)_this;

    tmpfs_lock.lock();

    if (off + count > (size_t)self->st_size)
        count -= (off + count) - self->st_size;

    _memcpy(self->data + off, buf, count);

    tmpfs_lock.unlock();
    return count;
}

static int64_t tmpfs_write(fs::inode* _this, const void* buf, int64_t off, size_t count)
{
    tmpfs_res* self = (tmpfs_res*)_this;

    tmpfs_lock.lock();

    if (off + count > self->allocated_size) {
        while (off + count > self->allocated_size)
            self->allocated_size <<= 1;

        self->data = (char*)mm::ralloc(self->data, self->allocated_size);
    }

    _memcpy(buf, self->data + off, count);

    self->st_size += count;

    tmpfs_lock.unlock();
    return count;
}

static int tmpfs_close(fs::inode* _this)
{
    tmpfs_res* self = (tmpfs_res*)_this;

    tmpfs_lock.lock();

    self->refcount--;

    tmpfs_lock.unlock();
    return 0;
}

static bool tmpfs_grow(fs::inode* _this, size_t new_size)
{
    tmpfs_res* res = (tmpfs_res*)_this;

    tmpfs_lock.lock();

    while (new_size > res->allocated_size)
        res->allocated_size <<= 1; // size * 2

    res->data = (char*)mm::ralloc(res->data, res->allocated_size);

    res->st_size = new_size;

    tmpfs_lock.unlock();
    return true;
}

static fs::inode* tmpfs_open(fs::node* node, bool create, uint32_t mode)
{
    if (!create)
        return nullptr;

    tmpfs_res* res = (tmpfs_res*)mm::alloc(sizeof(tmpfs_res));

    res->allocated_size = 4096;
    res->data = (char*)mm::alloc(res->allocated_size);
    res->st_dev = node->dev_id;
    res->st_size = 0;
    res->st_blocks = 0;
    res->st_blksize = 512;
    res->st_ino = (*((uint64_t*)node->mount_data))++;
    res->st_mode = (mode & ~S_IFMT) | S_IFREG;
    res->st_nlink = 1;
    res->close = tmpfs_close;
    res->read = tmpfs_read;
    res->write = tmpfs_write;
    res->grow = tmpfs_grow;
    res->actual_size = sizeof(tmpfs_res);

    return (fs::inode*)res;
}

static fs::inode* tmpfs_symlink(fs::node* node)
{

    tmpfs_res* res = new tmpfs_res();

    res->st_dev = node->dev_id;
    res->st_size = _strlen(node->target);
    res->st_blocks = 0;
    res->st_blksize = 512;
    res->st_ino = (*((uint64_t*)node->mount_data))++;
    res->st_mode = S_IFLNK | 0777;
    res->st_nlink = 1;
    res->actual_size = sizeof(tmpfs_res);

    return (fs::inode*)res;
}

static fs::inode* tmpfs_mkdir(fs::node* node, uint32_t mode)
{

    fs::inode* res = new fs::inode();

    res->st_dev = node->dev_id;
    res->st_size = 0;
    res->st_blocks = 0;
    res->st_blksize = 512;
    res->st_ino = (*((uint64_t*)node->mount_data))++;
    res->st_mode = (mode & ~S_IFMT) | S_IFDIR;
    res->st_nlink = 1;
    res->actual_size = sizeof(fs::inode);

    return (fs::inode*)res;
}

static fs::node* tmpfs_populate(fs::node* node)
{
    (void)node;
    return nullptr;
}

fs::filesystem tmpfs = {
    .name = "tmpfs",
    .is_backed = false,
    .mount = tmpfs_mount,
    .populate = tmpfs_populate,
    .open = tmpfs_open,
    .symlink = tmpfs_symlink,
    .mkdir = tmpfs_mkdir
};
