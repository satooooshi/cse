// yfs client.  implements FS operations using extent and lock server
#include "yfs_client.h"
#include "extent_client.h"
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

yfs_client::yfs_client()
{
    ec = new extent_client();

}

yfs_client::yfs_client(std::string extent_dst, std::string lock_dst)
{
    ec = new extent_client();
    if (ec->put(1, "") != extent_protocol::OK)
        printf("error init root dir\n"); // XYB: init root dir
}

yfs_client::inum
yfs_client::n2i(std::string n)
{
    std::istringstream ist(n);
    unsigned long long finum;
    ist >> finum;
    return finum;
}

std::string
yfs_client::filename(inum inum)
{
    std::ostringstream ost;
    ost << inum;
    return ost.str();
}

bool
yfs_client::isfile(inum inum)
{
    extent_protocol::attr a;

    if (ec->getattr(inum, a) != extent_protocol::OK) {
        printf("error getting attr\n");
        return false;
    }

    if (a.type == extent_protocol::T_FILE) {
        printf("isfile: %lld is a file\n", inum);
        return true;
    } 
    printf("isfile: %lld is a dir\n", inum);
    return false;
}
/** Your code here for Lab...
 * You may need to add routines such as
 * readlink, issymlink here to implement symbolic link.
 * 
 * */

bool
yfs_client::isdir(inum inum)
{
    // Oops! is this still correct when you implement symlink?
    //return ! isfile(inum);
    extent_protocol::attr a;

     if (ec->getattr(inum, a) != extent_protocol::OK) {
        printf("isdir: error getting attr\n");
        return false;
    }

    if (a.type == extent_protocol::T_DIR) {
        printf("isdir: %lld is a dir\n", inum);
        return true;
    }

    return false;
}

int
yfs_client::getfile(inum inum, fileinfo &fin)
{
    int r = OK;

    printf("getfile %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }

    fin.atime = a.atime;
    fin.mtime = a.mtime;
    fin.ctime = a.ctime;
    fin.size = a.size;
    printf("getfile %016llx -> sz %llu\n", inum, fin.size);

release:
    return r;
}

int
yfs_client::getdir(inum inum, dirinfo &din)
{
    int r = OK;

    printf("getdir %016llx\n", inum);
    extent_protocol::attr a;
    if (ec->getattr(inum, a) != extent_protocol::OK) {
        r = IOERR;
        goto release;
    }
    din.atime = a.atime;
    din.mtime = a.mtime;
    din.ctime = a.ctime;

release:
    return r;
}


#define EXT_RPC(xx) do { \
    if ((xx) != extent_protocol::OK) { \
        printf("EXT_RPC Error: %s:%d \n", __FILE__, __LINE__); \
        r = IOERR; \
        goto release; \
    } \
} while (0)


bool yfs_client::has_duplicate(inum parent, const char *name) {
    bool exist;
    inum old_inum;

    if (lookup(parent, name, exist, old_inum) != extent_protocol::OK) {
        printf("has_duplicate: fail to perform lookup\n");

        return true; // use true to forbid further action
    }
    return exist;
}

bool yfs_client::add_entry_and_save(inum parent, const char *name, inum inum) {
    std::list<dirent> entries;

    if (readdir(parent, entries) != OK) {
        printf("add_entry_and_save: fail to read directory entires\n");
        return false;
    }

    dirent entry;
    entry.name = name;
    entry.inum = inum;
    entries.push_back(entry);

    if (writedir(parent, entries) != OK) {
        printf("add_entry_and_save: fail to write directory entires\n");
        return false;
    }

    return true;
}



int yfs_client::writedir(inum dir, std::list<dirent>& entries) {
    printf("writedir: try to write directory entry to %llu\n", dir);

    // prepare content
    std::ostringstream ost;

    for (std::list<dirent>::iterator it = entries.begin(); it != entries.end();
         ++it) {
        ost << it->name;
        ost.put('\0');
        ost << it->inum;
    }

    // write to file
    if (ec->put(dir, ost.str()) != extent_protocol::OK) {
        printf("writedir: fail to write directory\n");
        return IOERR;
    }

    return OK;
}

// Only support set size of attr
int
yfs_client::setattr(inum ino, size_t size)
{
    //int r = OK;

    /*
     * your code goes here.
     * note: get the content of inode ino, and modify its content
     * according to the size (<, =, or >) content length.
     */

    //return r;

    // keep off invalid input
    if (ino <= 0) {
        printf("setattr: invalid inode number %llu\n", ino);
        return IOERR;
    }

    if (size < 0) {
        printf("setattr: size cannot be negative %lu\n", size);
        return IOERR;
    }

    // read old content
    std::string content;

    if (ec->get(ino, content) != extent_protocol::OK) {
        printf("setattr: fail to read content\n");
        return IOERR;
    }

    // just return if no resize should be applied
    if (size == content.size()) {
        return OK;
    }

    // resize and write back
    content.resize(size);

    if (ec->put(ino, content) != extent_protocol::OK) {
        printf("setattr: failt to write content\n");
        return IOERR;
    }

    return OK;
}

int
yfs_client::create(inum parent, const char *name, mode_t mode, inum &ino_out)
{
    //int r = OK;

    /*
     * your code goes here.
     * note: lookup is what you need to check if file exist;
     * after create file or dir, you must remember to modify the parent infomation.
     */

    //return r;
     // on exist, return EXIST
    if (has_duplicate(parent, name)) {
        return EXIST;
    }

    // create file first
    if (ec->create(extent_protocol::T_FILE, ino_out) != extent_protocol::OK) {
        printf("create: fail to create file\n");
        return IOERR;
    }

    // write back
    if (add_entry_and_save(parent, name, ino_out) == false) {
        return IOERR;
    }

    return OK;
}

int
yfs_client::mkdir(inum parent, const char *name, mode_t mode, inum &ino_out)
{
    //int r = OK;

    /*
     * your code goes here.
     * note: lookup is what you need to check if directory exist;
     * after create file or dir, you must remember to modify the parent infomation.
     */

    //return r;

    // on exist, return EXIST
    if (has_duplicate(parent, name)) {
        return EXIST;
    }

    // create file first
    if (ec->create(extent_protocol::T_DIR, ino_out) != extent_protocol::OK) {
        printf("create: fail to create directory\n");
        return IOERR;
    }

    // write back
    if (add_entry_and_save(parent, name, ino_out) == false) {
        return IOERR;
    }

    return OK;
}

int
yfs_client::lookup(inum parent, const char *name, bool &found, inum &ino_out)
{
    // r = OK;

    /*
     * your code goes here.
     * note: lookup file from parent dir according to name;
     * you should design the format of directory content.
     */

    //return r;

     // read directory entries
    std::list<dirent> entries;

    if (readdir(parent, entries) != OK) {
        printf("lookup: fail to read directory entires\n");
        return IOERR;
    }

    // check if name exists
    found = false;

    for (std::list<dirent>::iterator it = entries.begin(); it != entries.end();
         ++it) {
        if (it->name == name) {
            found   = true;
            ino_out = it->inum;
        }
    }

    return OK;
}

int
yfs_client::readdir(inum dir, std::list<dirent> &list)
{
    //int r = OK;

    /*
     * your code goes here.
     * note: you should parse the dirctory content using your defined format,
     * and push the dirents to the list.
     */

    //return r;
    // get content
    std::string content;

    if (ec->get(dir, content) != extent_protocol::OK) {
        return IOERR;
    }

    // read entries
    list.clear();
    std::istringstream ist(content);
    dirent entry;

    while (std::getline(ist, entry.name, '\0')) {
        ist >> entry.inum;
        list.push_back(entry);
    }

    return OK;
}

int
yfs_client::read(inum ino, size_t size, off_t off, std::string &data)
{
    //int r = OK;

    /*
     * your code goes here.
     * note: read using ec->get().
     */

    //return r;

    // keep off invalid input
    if (ino <= 0) {
        printf("read: invalid inode number %llu\n", ino);
        return IOERR;
    }

    if (size < 0) {
        printf("read: size cannot be negative %lu\n", size);
        return IOERR;
    }

    if (off < 0) {
        printf("read: offset cannot be negative %li\n", off);
        return IOERR;
    }

    // return IOERR if offset is beyond file size
    extent_protocol::attr a;

    if (ec->getattr(ino, a) != extent_protocol::OK) {
        printf("read: error getting attr\n");
        return IOERR;
    }

    if (off >= a.size) {
        printf("read: offset %li beyond file size %u\n", off, a.size);
        return IOERR;
    }

    // read the file and get desired data
    std::string content;

    if (ec->get(ino, content) != extent_protocol::OK) {
        printf("read: fail to read file\n");
        return IOERR;
    }

    data = content.substr(off, size);

    return OK;
}

int
yfs_client::write(inum ino, size_t size, off_t off, const char *data,
        size_t &bytes_written)
{
    //int r = OK;

    /*
     * your code goes here.
     * note: write using ec->put().
     * when off > length of original file, fill the holes with '\0'.
     */

    //return r;

     // keep off invalid input
    if (ino <= 0) {
        printf("write: invalid inode number %llu\n", ino);
        return IOERR;
    }

    if (size < 0) {
        printf("write: size cannot be negative %lu\n", size);
        return IOERR;
    }

    if (off < 0) {
        printf("write: offset cannot be negative %li\n", off);
        return IOERR;
    }

    // read the file
    std::string content;

    if (ec->get(ino, content) != extent_protocol::OK) {
        printf("write: fail to read file\n");
        return IOERR;
    }

    // replace old data with new data, then write back
    if ((unsigned)off >= content.size()) {
        content.resize(off, '\0');
        content.append(data, size);
    } else {
        content.replace(off,
                        off + size <= content.size() ? size : content.size() - off,
                        data,
                        size);
    }

    if (ec->put(ino, content) != extent_protocol::OK) {
        printf("write: fail to write file\n");
        return IOERR;
    }

    return OK;
}


int yfs_client::symlink(inum parent, const char *link, const char *name, inum& ino_out) {
    // keep off invalid input
    if (parent <= 0) {
        printf("symlink: invalid inode number %llu\n", parent);
        return IOERR;
    }

    // create file first
    if (ec->create(extent_protocol::T_SLINK, ino_out) != extent_protocol::OK) {
        printf("symlink: fail to create directory\n");
        return IOERR;
    }

    // write path to file
    if (ec->put(ino_out, link) != extent_protocol::OK) {
        printf("symlink: fail to write link\n");
        return IOERR;
    }

    // add entry to directory
    if (add_entry_and_save(parent, name, ino_out) == false) {
        printf("symlink: fail to add entry\n");
        return IOERR;
    }

    return OK;
}

int yfs_client::unlink(inum parent,const char *name)
{
    //int r = OK;

    /*
     * your code goes here.
     * note: you should remove the file using ec->remove,
     * and update the parent directory content.
     */

    //return r;

    printf("unlink: try to unlink %s from parent %llu\n", name, parent);

    // invalid inode number
    if (parent <= 0) {
        printf("unlink: invalid inode number %llu\n", parent);
        return IOERR;
    }

    // read entries
    std::list<dirent> entries;

    if (readdir(parent, entries) != OK) {
        printf("unlink: fail to read directory\n");
        return IOERR;
    }

    // locate target inode number
    std::list<dirent>::iterator it;

    for (it = entries.begin(); it != entries.end(); ++it) {
        if (it->name == name) {
            break;
        }
    }

    if (it == entries.end()) {
        printf("unlink: no such file or directory %s\n", name);
        return IOERR;
    }

    if (!isfile(it->inum)) {
        printf("unlink: %s is not a file\n", name);
        return IOERR;
    }

    // remove file and entry in directory
    if (ec->remove(it->inum) != extent_protocol::OK) {
        printf("unlink: fail to remove file %s\n", name);
        return IOERR;
    }

    entries.erase(it);

    if (writedir(parent, entries) != OK) {
        printf("unlink: fail to write directory entires\n");
        return IOERR;
    }

    return OK;
}

int yfs_client::readslink(inum ino, std::string& path) {
    // keep off invalid input
    if (ino <= 0) {
        printf("readslink: invalid inode number %llu\n", ino);
        return IOERR;
    }

    // read path
    if (ec->get(ino, path) != extent_protocol::OK) {
        printf("readslink: fail to read path\n");
        return IOERR;
    }

    return OK;
}