
#ifndef yfs_client_h
#define yfs_client_h

#include <string>

#include "lock_protocol.h"
#include "lock_client.h"

//#include "yfs_protocol.h"
#include "extent_client.h"
#include <vector>


class yfs_client {
  extent_client *ec;
  lock_client *lc;
 public:

  typedef unsigned long long inum;
  enum xxstatus { OK, RPCERR, NOENT, IOERR, EXIST };
  typedef int status;

  struct fileinfo {
    unsigned long long size;
    unsigned long atime;
    unsigned long mtime;
    unsigned long ctime;
  };
  struct dirinfo {
    unsigned long atime;
    unsigned long mtime;
    unsigned long ctime;
  };
   typedef fileinfo slinkinfo;
  struct dirent {
    std::string name;
    yfs_client::inum inum;
  };

 private:

    void _acquire(inum);
  void _release(inum);

  bool _has_duplicate(inum, const char *);
  bool _add_entry_and_save(inum, const char *, inum);

  bool _isfile(inum);
  bool _isdir(inum);

  int _getfile(inum, fileinfo &);
  int _getdir(inum, dirinfo &);
  int _getslink(inum, slinkinfo&);

  int _setattr(inum, size_t);
  int _lookup(inum, const char *, bool &, inum &);
  int _create(inum, const char *, mode_t, inum &);
  int _writedir(inum, std::list<dirent>&);
  int _readdir(inum, std::list<dirent> &);
  int _write(inum, size_t, off_t, const char *, size_t &);
  int _read(inum, size_t, off_t, std::string &);
  int _unlink(inum,const char *);
  int _mkdir(inum , const char *, mode_t , inum &);
  int _symlink(inum, const char *, const char *, inum&);
  int _readslink(inum, std::string&);
  int _rmdir(inum, const char *);

/*

  static std::string filename(inum);
  static inum n2i(std::string);


   int writedir(inum, std::list<dirent>&);
    bool has_duplicate(inum, const char *);
    bool add_entry_and_save(inum, const char *, inum);
*/
 public:
  yfs_client(std::string, std::string);

  bool isfile(inum);
  bool isdir(inum);

  int getfile(inum, fileinfo &);
  int getdir(inum, dirinfo &);
  int getslink(inum, slinkinfo&);

  int setattr(inum, size_t);
  int lookup(inum, const char *, bool &, inum &);
  int create(inum, const char *, mode_t, inum &);
  int readdir(inum, std::list<dirent> &);
  int write(inum, size_t, off_t, const char *, size_t &);
  int read(inum, size_t, off_t, std::string &);
  int unlink(inum,const char *);
  int mkdir(inum , const char *, mode_t , inum &);

   int symlink(inum, const char *, const char *, inum&);
   int readslink(inum, std::string&);
   int rmdir(inum, const char *);
   

  
  /** you may need to add symbolic link related methods here.*/
};

#endif 
