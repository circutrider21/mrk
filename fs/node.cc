#include <internal/builtin.h>
#include <mrk/alloc.h>
#include <mrk/fs.h>
#include <mrk/log.h>

#define NODE_NO_CREATE 0
#define NODE_CREATE_SHALLOW 1
#define NODE_CREATE_DEEP 2
#define NODE_NO_DEREF_LINKS 4
#define NODE_FAIL_IF_EXISTS 8

static mutex node_lock;

fs::node* path2node(fs::node* parent, const char* _path, int mode)
{
    bool last = false;

    // Make some checks on the path...
    if (_path == nullptr)
        return nullptr;
    if (*_path == 0)
        return nullptr;

    char l_path[_strlen(_path) + 1];
    _strcpy(l_path, _path);

    char* path = l_path;

    fs::node* cur_parent = *path == '/' || parent == nullptr ? fs_root : parent;

    while (cur_parent->redir != nullptr)
        cur_parent = cur_parent->redir;

    if (cur_parent->mount_gate != nullptr)
        cur_parent = cur_parent->mount_gate;

    fs::node* cur_node = cur_parent->child;

    while (*path == '/')
        path++;
    if (*path == 0)
        return fs_root;

next:;
    char* elem = path;
    while (*path != 0 && *path != '/')
        path++;
    if (*path == '/') {
        *path++ = 0;
        while (*path == '/')
            path++;
    }
    if (*path == 0)
        last = true;

    if (cur_node == nullptr)
        goto end;

    for (;;) {
        bool mount_peek = false;
        fs::node* mount_parent;

        if (cur_node->mount_gate != nullptr) {
            mount_peek = true;
            mount_parent = cur_node;
            cur_node = cur_node->mount_gate;
        }

        if (_memcmp(cur_node->name, elem, _strlen(elem)) != 0) {
            // If match found
            if (mount_peek)
                cur_node = mount_parent->next;
            else
                cur_node = cur_node->next;
            if (cur_node == nullptr)
                break;
            continue;
        }

        while (cur_node->redir != nullptr)
            cur_node = cur_node->redir;

        if (last) {
            if (mode & NODE_FAIL_IF_EXISTS) {
                return nullptr;
            }

            if (cur_node->res != nullptr && S_ISLNK(cur_node->res->st_mode)) {
                if (mode & NODE_NO_DEREF_LINKS) {
                    return nullptr;
                }

                return path2node(cur_node->parent, cur_node->target, mode);
            }

            return cur_node;
        }

        if (cur_node->res == nullptr || !S_ISDIR(cur_node->res->st_mode)) {
            return nullptr;
        }

        if (cur_node->child == nullptr) {
            cur_node->child = cur_node->fs->populate(cur_node);
            if (cur_node->child == nullptr) {
                return nullptr;
            }
        }

        cur_parent = cur_node;
        cur_node = cur_node->child;
        goto next;
    }

end:

    if (mode & (NODE_CREATE_SHALLOW | NODE_CREATE_DEEP)) {
        if (last) {
            return fs::create_node(cur_parent, elem);
        } else {
            if (!(mode & NODE_CREATE_DEEP)) {
                return nullptr;
            }
            cur_parent = fs::mkdir(cur_parent, elem, 0755, false);
            goto next;
        }
    }

    return nullptr;
}

fs::node* fs::create_node(fs::node* parent, const char* name, bool deep)
{
    if (deep) {
        fs::node* to_make = path2node(parent, name, NODE_NO_CREATE);

        if (to_make != nullptr)
            return nullptr;

        to_make = path2node(parent, name, NODE_CREATE_DEEP);
        return to_make;
    }

    // Use the vfs_root if parent wasn't specified
    if (parent == nullptr)
        parent = fs_root;

    // Use the mount-root instead of the parent (if possible)
    if (parent->mount_gate)
        parent = parent->mount_gate;

    fs::node* new_node = path2node(parent, name, NODE_NO_CREATE);

    // Node better not exist...
    if (new_node != nullptr && new_node != fs_root) {
        return nullptr;
    }

    new_node = new fs::node();

    _strcpy(name, new_node->name);
    parent->child = new_node;

    new_node->next = parent->child;
    new_node->fs = parent->fs;
    new_node->mount_data = parent->mount_data;
    new_node->dev_id = parent->dev_id;
    new_node->parent = parent;

    return new_node;
}

fs::node* fs::mkdir(fs::node* parent, const char* name, uint32_t mode, bool do_loop)
{
    if (parent == nullptr)
        parent = fs_root;

    fs::node* new_dir = path2node(parent, name, NODE_NO_CREATE);

    if (new_dir != nullptr)
        return nullptr;

    new_dir = path2node(parent, name, do_loop ? NODE_CREATE_DEEP : NODE_CREATE_SHALLOW);

    if (new_dir == nullptr)
        return nullptr;

    new_dir->res = new_dir->fs->mkdir(new_dir, mode);

    // Create the . and .. links
    fs::node* dt = fs::create_node(new_dir, ".");
    dt->redir = new_dir;
    fs::node* doubledt = fs::create_node(new_dir, "..");
    doubledt->redir = new_dir->parent;

    return new_dir;
}

fs::inode* fs::open(fs::node** dir, fs::node* parent, const char* path, int oflags, uint32_t mode)
{
    node_lock.lock();

    bool create = oflags & O_CREAT;
    fs::node* path_node = path2node(parent, path, create ? NODE_CREATE_SHALLOW : NODE_NO_CREATE);
    if (path_node == nullptr) {
        node_lock.unlock();
        return nullptr;
    }

    if (path_node->res == nullptr)
        path_node->res = path_node->fs->open(path_node, create, mode);

    if (path_node->res == nullptr) {
        node_lock.unlock();
        return nullptr;
    }

    fs::inode* res = path_node->res;

    if (S_ISDIR(res->st_mode) && dir != nullptr) {
        *dir = path_node;
    }

    res->refcount++;

    node_lock.unlock();
    return res;
}

bool fs::symlink(fs::node* parent, const char* target, const char* path)
{
    node_lock.lock();

    fs::node* path_node = path2node(parent, path,
        NODE_FAIL_IF_EXISTS | NODE_CREATE_SHALLOW);

    if (path_node == nullptr) {
        node_lock.unlock();
        return false;
    }

    _strcpy(path_node->target, target);

    path_node->res = path_node->fs->symlink(path_node);

    node_lock.unlock();

    return true;
}

bool fs::mount(const char* source, const char* target, const char* fs_type)
{
    fs::filesystem* fs = fs::get_filesystem(fs_type);
    if (fs == nullptr)
        return false;

    bool mounting_root = false;
    for (size_t i = 0; target[i] == '/'; i++) {
        if (target[i + 1] == 0) {
            mounting_root = true;
            break;
        }
    }

    fs::node* tgt_node;
    if (!mounting_root) {
        tgt_node = path2node(nullptr, target, NODE_NO_CREATE);
        if (tgt_node == nullptr)
            return false;

        if (!S_ISDIR(tgt_node->res->st_mode)) {
            return false;
        }
    }

    uint64_t backing_dev_id = 0;
    fs::inode* src_handle = nullptr;
    if (fs->is_backed) {
        fs::node* backing_dev_node = path2node(nullptr, source, NODE_NO_CREATE);
        if (backing_dev_node == nullptr)
            return false;
        if (!S_ISCHR(backing_dev_node->res->st_mode)
            && !S_ISBLK(backing_dev_node->res->st_mode))
            return false;
        fs::inode* src_handle = fs::open(nullptr, nullptr, source, O_RDWR, 0);
        if (src_handle == nullptr)
            return false;
        backing_dev_id = backing_dev_node->res->st_rdev;
    }


    fs::node* mount_gate = fs->mount(src_handle);
    if (mount_gate == nullptr) {
        src_handle->close(src_handle);
        return false;
    }

    if (fs->is_backed) {
        mount_gate->dev_id = backing_dev_id;
        mount_gate->res->st_dev = backing_dev_id;
    }

    if (mounting_root) {
        fs_root = mount_gate;
        mount_gate->parent = nullptr;
        _strcpy(mount_gate->name, "/");

        fs::node* dot = fs::create_node(mount_gate, ".");
        dot->redir = mount_gate;
        fs::node* dotdot = fs::create_node(mount_gate, "..");
        dotdot->redir = mount_gate;
    } else {
        tgt_node->mount_gate = mount_gate;
        mount_gate->parent = tgt_node->parent;
        _strcpy(mount_gate->name, tgt_node->name);

        fs::node* d = fs::create_node(mount_gate, ".");
        d->redir = mount_gate;
        fs::node* dd = fs::create_node(mount_gate, "..");
        dd->redir = mount_gate->parent;
    }

    // log("fs: `%s` is now mounted as '%s'. (type: `%s`)\n", source, target, fs_type);

    return true;
}

