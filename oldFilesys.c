/*
  Big Brother File System
  Copyright (C) 2012 Joseph J. Pfeiffer, Jr., Ph.D. <pfeiffer@cs.nmsu.edu>
  This program can be distributed under the terms of the GNU GPLv3.
  See the file COPYING.
  This code is derived from function prototypes found /usr/include/fuse/fuse.h
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  His code is licensed under the LGPLv2.
  A copy of that code is included in the file fuse.h
  The point of this FUSE filesystem is to provide an introduction to
  FUSE.  It was my first FUSE filesystem as I got to know the
  software; hopefully, the comments in this code will help people who
  follow later to get a gentler introduction.
  This might be called a no-op filesystem:  it doesn't impose
  filesystem semantics on top of any other existing structure.  It
  simply reports the requests that come in, and passes them to an
  underlying filesystem.  The information is saved in a logfile named
  bbfs.log, in the directory from which you run bbfs.
*/
#define FUSE_USE_VERSION 30
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <linux/limits.h>
#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include <sys/xattr.h>
#include <pthread.h>

#define BB_DATA ((struct bb_state *)fuse_get_context()->private_data)
#define local_dir "/tmp/fuse_test"
#define server_dir "/tmp/fuse_test"
#define MAX_XFER_BUF_SIZE 16384

struct bb_state
{
   char *rootdir;
   char *serverdir;
   char *hostname;
   ssh_session bb_ssh_session;
   sftp_session bb_sftp_session;
   pthread_mutex_t lock;
};



int setup_ssh(ssh_session *my_ssh_session, char *hostname);
int setup_sftp(sftp_session *my_sftp_session, ssh_session *my_ssh_session);


static int get_error(int retstat)
{
   if (retstat < 0)
      return -errno;
   return retstat;
}

static void serverFullName(char fpath[PATH_MAX], const char *path)
{
   strcpy(fpath, BB_DATA->serverdir);
   strncat(fpath, path, PATH_MAX);
}

int getMtimeServer(char fpath[PATH_MAX], uint32_t *mt)
{
   //printf("Getting mtime\n");

   int rc;
   ssh_session my_session;
   sftp_session my_sftp;

   rc = setup_ssh(&my_session, BB_DATA->hostname);

   if (rc < 0)
   {
      return -1;
   }
   rc = setup_sftp(&my_sftp, &my_session);

   if (rc < 0)
   {
      return -1;
   }

   sftp_attributes stats = sftp_lstat(my_sftp, fpath);
   if (stats == NULL)
   {
      int ret = sftp_get_error(my_sftp);
      if (ret == SSH_FX_NO_SUCH_FILE || ret == SSH_FX_NO_SUCH_PATH)
      {
         sftp_free(my_sftp);
         ssh_free(my_session);
         return -ENOENT;
      }
      else if (ret == SSH_FX_PERMISSION_DENIED)
      {
         sftp_free(my_sftp);
         ssh_free(my_session);
         return -EACCES;
      }
      else
      {
         fprintf(stdout, "Error getting mtime SFTP session: code %d.\n",
                 ret);
         fprintf(stdout, "Error getting mtime SSH session: code %s.\n",
                 ssh_get_error(my_session));
         sftp_free(my_sftp);
         ssh_free(my_session);
         return -1;
      }
   }
   else
   {
      *mt = stats->mtime;
      sftp_attributes_free(stats);
   }

   sftp_free(my_sftp);
   ssh_free(my_session);

   return 0;
}

int copyInFile(char sfpath[PATH_MAX], char fpath[PATH_MAX])
{
   //printf("Copying in: %s\n", fpath);

   int rc;
   ssh_session my_session;
   sftp_session my_sftp;

   rc = setup_ssh(&my_session, BB_DATA->hostname);

   if (rc < 0)
   {
      return -1;
   }
   rc = setup_sftp(&my_sftp, &my_session);

   if (rc < 0)
   {
      return -1;
   }

   int access_type;
   sftp_file file;
   char buffer[MAX_XFER_BUF_SIZE];
   int nbytes, nwritten;
   int fd;

   access_type = O_RDONLY;
   file = sftp_open(my_sftp, sfpath,
                    access_type, 0);
   if (file == NULL)
   {
      fprintf(stderr, "Can't open file for reading: %s\n",
              ssh_get_error(my_session));
      return -SSH_ERROR;
   }

   fd = open(fpath, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU | S_IRWXG | S_IRWXO);
   if (fd < 0)
   {
      fprintf(stderr, "Can't open file for writing: %s\n",
              strerror(errno));
      return -errno;
   }

   for (;;)
   {
      nbytes = sftp_read(file, buffer, sizeof(buffer));
      if (nbytes == 0)
      {
         break; // EOF
      }
      else if (nbytes < 0)
      {
         fprintf(stderr, "Error while reading file: %s\n",
                 ssh_get_error(my_session));
         sftp_close(file);
         close(fd);
         return -SSH_ERROR;
      }

      nwritten = write(fd, buffer, nbytes);
      if (nwritten != nbytes)
      {
         fprintf(stderr, "Error writing: %s\n",
                 strerror(errno));
         sftp_close(file);
         close(fd);
         return -SSH_ERROR;
      }
   }
   rc = close(fd);
   rc = sftp_close(file);
   if (rc != SSH_OK)
   {
      fprintf(stderr, "Can't close the read file: %s\n",
              ssh_get_error(my_session));
      return rc;
   }

   sftp_free(my_sftp);
   ssh_free(my_session);

   return SSH_OK;
}

int copyOutFile(char sfpath[PATH_MAX], char fpath[PATH_MAX])
{
   //printf("Copying out: %s\n", fpath);
   int access_type = O_WRONLY | O_CREAT | O_TRUNC;
   sftp_file file;
   char buffer[MAX_XFER_BUF_SIZE];
   int rc;
   int nwritten;
   int nread;
   int fd;

   ssh_session my_session;
   sftp_session my_sftp;

   rc = setup_ssh(&my_session, BB_DATA->hostname);

   if (rc < 0)
   {
      return -1;
   }
   rc = setup_sftp(&my_sftp, &my_session);

   if (rc < 0)
   {
      return -1;
   }

   file = sftp_open(my_sftp, sfpath,
                    access_type, S_IRWXU | S_IRWXG | S_IRWXO);
   if (file == NULL)
   {
      fprintf(stderr, "Can't open file for writing: %s\n",
              ssh_get_error(my_sftp));
      return SSH_ERROR;
   }

   fd = open(fpath, O_RDONLY);

   int off;
   off = lseek(fd, 0, SEEK_SET);

   for (;;)
   {
      nread = read(fd, buffer, sizeof(buffer));
      if (nread < 0)
      {
         perror("Error Readin for copy");
         break;
      }
      if (nread == 0)
      {
         //printf("End of File");
         break;
      }
      // off = lseek(fd, nread, SEEK_CUR);
      nwritten = sftp_write(file, buffer, nread);
      if (nwritten != nread)
      {
         fprintf(stderr, "Can't write data to file: %s\n",
                 ssh_get_error(my_session));
         sftp_close(file);
         close(fd);
         sftp_free(my_sftp);
         ssh_free(my_session);
         return SSH_ERROR;
      }
   }

   rc = sftp_close(file);
   if (rc != SSH_OK)
   {
      fprintf(stderr, "Can't close the written file: %s\n",
              ssh_get_error(my_session));
      close(fd);
      sftp_free(my_sftp);
      ssh_free(my_session);
      return rc;
   }

   sftp_free(my_sftp);
   ssh_free(my_session);
   close(fd);
   return SSH_OK;
}

//  All the paths I see are relative to the root of the mounted
//  filesystem.  In order to get to the underlying filesystem, I need to
//  have the mountpoint.  I'll save it away early on in main(), and then
//  whenever I need a path for something I'll call this to construct
//  it.
static void bb_fullpath(char fpath[PATH_MAX], const char *path)
{
   strcpy(fpath, BB_DATA->rootdir);
   strncat(fpath, path, PATH_MAX);
}

///////////////////////////////////////////////////////////
//
// Prototypes for all these functions, and the C-style comments,
// come from /usr/include/fuse.h
//
/** Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.  The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
int bb_getattr(const char *path, struct stat *statbuf)
{
   //printf("getAttr:%s\n", path);
   int retstat;
   char fpath[PATH_MAX];
   bb_fullpath(fpath, path);

   // get local attributes
   retstat = get_error(lstat(fpath, statbuf));

   // get server attributes
   char sfpath[PATH_MAX];
   serverFullName(sfpath, path);
   int rc;

   pthread_mutex_lock(&(BB_DATA->lock));

   sftp_attributes server_stats;
   server_stats = sftp_lstat(BB_DATA->bb_sftp_session, sfpath);

   pthread_mutex_unlock(&(BB_DATA->lock));

   if (server_stats == NULL)
   {
      int code = sftp_get_error(BB_DATA->bb_sftp_session);
      if (code == SSH_FX_NO_SUCH_FILE)
      {
         return retstat;
      }
      else if (code == SSH_FX_PERMISSION_DENIED || code == SSH_FX_WRITE_PROTECT)
      {
         return -EACCES;
      }
      else
      {
         return -EFAULT;
      }
   }
   // case where file is on server but not on local or local is stale
   else if (retstat < 0 || server_stats->mtime > statbuf->st_mtime)
   {
      statbuf->st_mtime = server_stats->mtime;
      statbuf->st_atime = server_stats->atime;
      statbuf->st_size = server_stats->size;
      statbuf->st_uid = getuid();
      statbuf->st_gid = getgid();
      statbuf->st_mode = server_stats->permissions;
      if (strcmp(path, "/") == 0)
      {
         statbuf->st_nlink = 2;
      }
      else
      {
         statbuf->st_nlink = 1;
      }
      retstat = 0;
   }

   if (server_stats != NULL)
      sftp_attributes_free(server_stats);

   return retstat;
}

/** Read the target of a symbolic link
 *
 * The buffer should be filled with a null terminated string.  The
 * buffer size argument includes the space for the terminating
 * null character.  If the linkname is too long to fit in the
 * buffer, it should be truncated.  The return value should be 0
 * for success.
 */
// Note the system readlink() will truncate and lose the terminating
// null.  So, the size passed to to the system readlink() must be one
// less than the size passed to bb_readlink()
// bb_readlink() code by Bernardo F Costa (thanks!)
int bb_readlink(const char *path, char *link, size_t size)
{
   //printf("readLink:%s\n", path);
   int retstat;
   char fpath[PATH_MAX];
   bb_fullpath(fpath, path);

   retstat = get_error(readlink(fpath, link, size - 1));
   if (retstat >= 0)
   {
      link[retstat] = '\0';
      retstat = 0;
   }

   return retstat;
}

/** Create a file node
 *
 * There is no create() operation, mknod() will be called for
 * creation of all non-directory, non-symlink nodes.
 */
// shouldn't that comment be "if" there is no.... ?
int bb_mknod(const char *path, mode_t mode, dev_t dev)
{
   //printf("mknod:%s\n", path);
   int retstat;
   char fpath[PATH_MAX];

   bb_fullpath(fpath, path);

   // On Linux this could just be 'mknod(path, mode, dev)' but this
   // tries to be be more portable by honoring the quote in the Linux
   // mknod man page stating the only portable use of mknod() is to
   // make a fifo, but saying it should never actually be used for
   // that.
   if (S_ISREG(mode))
   {
      retstat = get_error(open(fpath, O_CREAT | O_EXCL | O_WRONLY, mode));
      if (retstat >= 0)
         retstat = get_error(close(retstat));
   }
   else if (S_ISFIFO(mode))
   {
      retstat = get_error(mkfifo(fpath, mode));
   }
   else
   {
      retstat = get_error(mknod(fpath, mode, dev));
   }

   return retstat;
}

/** Create a directory */
int bb_mkdir(const char *path, mode_t mode)
{
   //printf("mkdir:%s\n", path);
   char fpath[PATH_MAX];

   bb_fullpath(fpath, path);

   int retstat = get_error(mkdir(fpath, mode));
   return retstat;
}

/** Remove a file */
int bb_unlink(const char *path)
{
   //printf("unlink:%s\n", path);
   char fpath[PATH_MAX];
   bb_fullpath(fpath, path);

   int retstat = get_error(unlink(fpath));
   return retstat;
}

/** Remove a directory */
int bb_rmdir(const char *path)
{
   //printf("rmdir:%s\n", path);
   char fpath[PATH_MAX];

   bb_fullpath(fpath, path);

   int retstat = get_error(rmdir(fpath));
   return retstat;
}

/** Create a symbolic link */
// The parameters here are a little bit confusing, but do correspond
// to the symlink() system call.  The 'path' is where the link points,
// while the 'link' is the link itself.  So we need to leave the path
// unaltered, but insert the link into the mounted directory.
int bb_symlink(const char *path, const char *link)
{
   //printf("symlink:%s\n", path);
   char flink[PATH_MAX];

   bb_fullpath(flink, link);

   int retstat = get_error(symlink(path, flink));
   return retstat;
}

/** Rename a file */
// both path and newpath are fs-relative
int bb_rename(const char *path, const char *newpath)
{
   //printf("rename:%s\n", path);
   char fpath[PATH_MAX];
   char fnewpath[PATH_MAX];

   bb_fullpath(fpath, path);
   bb_fullpath(fnewpath, newpath);

   int retstat = get_error(rename(fpath, fnewpath));
   return retstat;
}

/** Create a hard link to a file */
int bb_link(const char *path, const char *newpath)
{
   //printf("link:%s\n", path);
   char fpath[PATH_MAX], fnewpath[PATH_MAX];

   bb_fullpath(fpath, path);
   bb_fullpath(fnewpath, newpath);

   int retstat = get_error(link(fpath, fnewpath));
   return retstat;
}

/** Change the permission bits of a file */
int bb_chmod(const char *path, mode_t mode)
{
   //printf("chmod:%s\n", path);
   char fpath[PATH_MAX];

   bb_fullpath(fpath, path);

   int retstat = get_error(chmod(fpath, mode));
   return retstat;
}

/** Change the owner and group of a file */
int bb_chown(const char *path, uid_t uid, gid_t gid)

{
   //printf("chown:%s\n", path);
   char fpath[PATH_MAX];

   bb_fullpath(fpath, path);

   int retstat = get_error(chown(fpath, uid, gid));
   return retstat;
}

/** Change the size of a file */
int bb_truncate(const char *path, off_t newsize)
{
   //printf("truncate:%s\n", path);
   char fpath[PATH_MAX];

   bb_fullpath(fpath, path);

   int retstat = get_error(truncate(fpath, newsize));
   return retstat;
}

/** Change the access and/or modification times of a file */
/* note -- I'll want to change this as soon as 2.6 is in debian testing */
int bb_utime(const char *path, struct utimbuf *ubuf)
{
   //printf("utime:%s\n", path);
   char fpath[PATH_MAX];

   bb_fullpath(fpath, path);

   int retsat = get_error(utime(fpath, ubuf));
   return retsat;
}

/** File open operation
 *
 * No creation, or truncation flags (O_CREAT, O_EXCL, O_TRUNC)
 * will be passed to open().  Open should check if the operation
 * is permitted for the given flags.  Optionally open may also
 * return an arbitrary filehandle in the fuse_file_info structure,
 * which will be passed to all file operations.
 *
 * Changed in version 2.2
 */
int bb_open(const char *path, struct fuse_file_info *fi)
{
   //printf("open:%s\n", path);
   int retstat = 0;
   int fd;
   char fpath[PATH_MAX];
   char sfpath[PATH_MAX];

   bb_fullpath(fpath, path);
   serverFullName(sfpath, path);

   // TODO: check server mtime and local mtime
   int rc;
   uint32_t server_mtime;
   uint32_t local_mtime;
   rc = getMtimeServer(sfpath, &server_mtime);

   //printf("Got server return code: %d", rc);

   struct stat attr;
   retstat = lstat(fpath, &attr);
   int needCopy = 0;

   // TODO: if file dne on local but on server or server has larger mtime copy in server file

   if (rc < 0)
   {
      if (rc == -EACCES)
      {
         return rc;
      }
      else if (rc != -ENOENT)
      {
         return -1;
      }
   }

   if (retstat < 0)
   {
      if (errno == EACCES)
      {
         return -EACCES;
      }
      else if (rc >= 0)
      {
         needCopy = 1;
      }
      else if (errno != ENOENT)
      {
         return -errno;
      }
   }

   if (retstat >= 0 && rc >= 0)
   {
      local_mtime = attr.st_mtime;
      if (server_mtime > local_mtime)
      {
         needCopy = 1;
      }
   }

   // need to copy in
   if (needCopy)
   {
      retstat = copyInFile(sfpath, fpath);
      if (retstat < 0)
      {
         return -1;
      }
   }

   // continue to opening as normal

   // if the open call succeeds, my retstat is the file descriptor,
   // else it's -errno.  I'm making sure that in that case the saved
   // file descriptor is exactly -1.
   fd = get_error(open(fpath, fi->flags, S_IRWXU | S_IRWXG | S_IRWXO));
   if (fd < 0)
   {
      retstat = -errno;
   }
   fi->fh = fd;

   return retstat;
}

/** Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.  An exception to this is when the
 * 'direct_io' mount option is specified, in which case the return
 * value of the read system call will reflect the return value of
 * this operation.
 *
 * Changed in version 2.2
 */
// I don't fully understand the documentation above -- it doesn't
// match the documentation for the read() system call which says it
// can return with anything up to the amount of data requested. nor
// with the fusexmp code which returns the amount of data also
// returned by read.
int bb_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
   //printf("read:%s\n", path);
   int retstat = 0;

   // check if changes made and then copy to server

   return get_error(pread(fi->fh, buf, size, offset));
}

/** Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error.  An exception to this is when the 'direct_io'
 * mount option is specified (see read operation).
 *
 * Changed in version 2.2
 */
// As  with read(), the documentation above is inconsistent with the
// documentation for the write() system call.
int bb_write(const char *path, const char *buf, size_t size, off_t offset,
             struct fuse_file_info *fi)
{
   //printf("write:%s\n", path);
   int retstat = 0;

   return get_error(pwrite(fi->fh, buf, size, offset));
}

/** Get file system statistics
 *
 * The 'f_frsize', 'f_favail', 'f_fsid' and 'f_flag' fields are ignored
 *
 * Replaced 'struct statfs' parameter with 'struct statvfs' in
 * version 2.5
 */
int bb_statfs(const char *path, struct statvfs *statv)
{
   //printf("statfs:%s\n", path);
   int retstat = 0;
   char fpath[PATH_MAX];

   bb_fullpath(fpath, path);

   // get stats for underlying filesystem
   retstat = get_error(statvfs(fpath, statv));

   return retstat;
}

/** Possibly flush cached data
 *
 * BIG NOTE: This is not equivalent to fsync().  It's not a
 * request to sync dirty data.
 *
 * Flush is called on each close() of a file descriptor.  So if a
 * filesystem wants to return write errors in close() and the file
 * has cached dirty data, this is a good place to write back data
 * and return any errors.  Since many applications ignore close()
 * errors this is not always useful.
 *
 * NOTE: The flush() method may be called more than once for each
 * open().  This happens if more than one file descriptor refers
 * to an opened file due to dup(), dup2() or fork() calls.  It is
 * not possible to determine if a flush is final, so each flush
 * should be treated equally.  Multiple write-flush sequences are
 * relatively rare, so this shouldn't be a problem.
 *
 * Filesystems shouldn't assume that flush will always be called
 * after some writes, or that if will be called at all.
 *
 * Changed in version 2.2
 */
// this is a no-op in BBFS.  It just logs the call and returns success
int bb_flush(const char *path, struct fuse_file_info *fi)
{
   return 0;
}

/** Release an open file
 *
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file descriptor.  It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 *
 * Changed in version 2.2
 */
int bb_release(const char *path, struct fuse_file_info *fi)
{
   //printf("release:%s\n", path);
   char fpath[PATH_MAX];
   bb_fullpath(fpath, path);

   char sfpath[PATH_MAX];
   serverFullName(sfpath, path);
   int rc;
   int retstat;

   // TODO: on close check for dirty and if so copy to server

   // We need to close the file.  Had we allocated any resources
   // (buffers etc) we'd need to free them here as well.

   if (fi->flags & O_WRONLY || fi->flags & O_RDWR)
   {
      rc = copyOutFile(sfpath, fpath);

      if (rc < -1)
      {
         printf("Failed Copying to Server");
         return -1;
      }
      utime(fpath, NULL);
   }
   retstat = get_error(close(fi->fh));

   return retstat;
}

/** Synchronize file contents
 *
 * If the datasync parameter is non-zero, then only the user data
 * should be flushed, not the meta data.
 *
 * Changed in version 2.2
 */
int bb_fsync(const char *path, int datasync, struct fuse_file_info *fi)
{
   //printf("fsync%s\n", path);
   // some unix-like systems (notably freebsd) don't have a datasync call
#ifdef HAVE_FDATASYNC
   if (datasync)
      return get_error(fdatasync(fi->fh));
   else
#endif
      return get_error(fsync(fi->fh));
}

/** Open directory
 *
 * This method should check if the open operation is permitted for
 * this  directory
 *
 * Introduced in version 2.3
 */
int bb_opendir(const char *path, struct fuse_file_info *fi)
{
   //printf("opendir:%s\n", path);
   DIR *dp;
   int retstat = 0;
   char fpath[PATH_MAX];

   bb_fullpath(fpath, path);

   // since opendir returns a pointer, takes some custom handling of
   // return status.
   dp = opendir(fpath);
   if (dp == NULL)
      retstat = -errno;

   fi->fh = (intptr_t)dp;

   return retstat;
}

/** Read directory
 *
 * This supersedes the old getdir() interface.  New applications
 * should use this.
 *
 * The filesystem may choose between two modes of operation:
 *
 * 1) The readdir implementation ignores the offset parameter, and
 * passes zero to the filler function's offset.  The filler
 * function will not return '1' (unless an error happens), so the
 * whole directory is read in a single readdir operation.  This
 * works just like the old getdir() method.
 *
 * 2) The readdir implementation keeps track of the offsets of the
 * directory entries.  It uses the offset parameter and always
 * passes non-zero offset to the filler function.  When the buffer
 * is full (or an error happens) the filler function will return
 * '1'.
 *
 * Introduced in version 2.3
 */

int bb_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
               struct fuse_file_info *fi)
{
   //printf("readdir:%s\n", path);
   int retstat = 0;
   DIR *dp;
   struct dirent *de;

   // once again, no need for fullpath -- but note that I need to cast fi->fh
   dp = (DIR *)(uintptr_t)fi->fh;

   // Every directory contains at least two entries: . and ..  If my
   // first call to the system readdir() returns NULL I've got an
   // error; near as I can tell, that's the only condition under
   // which I can get an error from readdir()
   de = readdir(dp);
   if (de == 0)
   {
      retstat = -errno;
      return retstat;
   }

   // This will copy the entire directory into the buffer.  The loop exits
   // when either the system readdir() returns NULL, or filler()
   // returns something non-zero.  The first case just means I've
   // read the whole directory; the second means the buffer is full.
   do
   {
      if (filler(buf, de->d_name, NULL, 0) != 0)
      {
         return -ENOMEM;
      }
   } while ((de = readdir(dp)) != NULL);

   return retstat;
}

/** Release directory
 *
 * Introduced in version 2.3
 */
int bb_releasedir(const char *path, struct fuse_file_info *fi)
{
   //printf("releasedir:%s\n", path);
   int retstat = 0;

   closedir((DIR *)(uintptr_t)fi->fh);

   return retstat;
}

/** Synchronize directory contents
 *
 * If the datasync parameter is non-zero, then only the user data
 * should be flushed, not the meta data
 *
 * Introduced in version 2.3
 */
// when exactly is this called?  when a user calls fsync and it
// happens to be a directory? ??? >>> I need to implement this...
int bb_fsyncdir(const char *path, int datasync, struct fuse_file_info *fi)
{

   int retstat = 0;

   return retstat;
}

/**
 * Initialize filesystem
 *
 * The return value will passed in the private_data field of
 * fuse_context to all file operations and as a parameter to the
 * destroy() method.
 *
 * Introduced in version 2.3
 * Changed in version 2.6
 */
// Undocumented but extraordinarily useful fact:  the fuse_context is
// set up before this function is called, and
// fuse_get_context()->private_data returns the user_data passed to
// fuse_main().  Really seems like either it should be a third
// parameter coming in here, or else the fact should be documented
// (and this might as well return void, as it did in older versions of
// FUSE).
void *bb_init(struct fuse_conn_info *conn)
{
   return BB_DATA;
}

/**
 * Clean up filesystem
 *
 * Called on filesystem exit.
 *
 * Introduced in version 2.3
 */
void bb_destroy(void *userdata)
{
   struct bb_state *data = (struct bb_state *)userdata;
   sftp_free(data->bb_sftp_session);
   ssh_disconnect(data->bb_ssh_session);
   ssh_free(data->bb_ssh_session);
}

/**
 * Check file access permissions
 *
 * This will be called for the access() system call.  If the
 * 'default_permissions' mount option is given, this method is not
 * called.
 *
 * This method is not called under Linux kernel versions 2.4.x
 *
 * Introduced in version 2.5
 */
int bb_access(const char *path, int mask)
{
   //printf("access:%s\n", path);
   int retstat = 0;
   char fpath[PATH_MAX];

   bb_fullpath(fpath, path);

   retstat = get_error(access(fpath, mask));

   return retstat;
}

/**
 * Create and open a file
 *
 * If the file does not exist, first create it with the specified
 * mode, and then open it.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the mknod() and open() methods
 * will be called instead.
 *
 * Introduced in version 2.5
 */
// Not implemented.  I had a version that used creat() to create and
// open the file, which it turned out opened the file write-only.

/**
 * Change the size of an open file
 *
 * This method is called instead of the truncate() method if the
 * truncation was invoked from an ftruncate() system call.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the truncate() method will be
 * called instead.
 *
 * Introduced in version 2.5
 */
int bb_ftruncate(const char *path, off_t offset, struct fuse_file_info *fi)
{
   //printf("ftruncate:%s\n", path);
   int retstat = 0;

   retstat = get_error(ftruncate(fi->fh, offset));

   return retstat;
}

/**
 * Get attributes from an open file
 *
 * This method is called instead of the getattr() method if the
 * file information is available.
 *
 * Currently this is only called after the create() method if that
 * is implemented (see above).  Later it may be called for
 * invocations of fstat() too.
 *
 * Introduced in version 2.5
 */
int bb_fgetattr(const char *path, struct stat *statbuf, struct fuse_file_info *fi)
{
   //printf("fgetAttr:%s\n", path);
   int retstat = 0;

   // On FreeBSD, trying to do anything with the mountpoint ends up
   // opening it, and then using the FD for an fgetattr.  So in the
   // special case of a path of "/", I need to do a getattr on the
   // underlying root directory instead of doing the fgetattr().
   if (!strcmp(path, "/"))
      return bb_getattr(path, statbuf);

   retstat = get_error(fstat(fi->fh, statbuf));

   return retstat;
}

/*
int bb_setxattr(const char *path, const char *name, const char *value, size_t size, int flags)
{
   char fpath[PATH_MAX];
   bb_fullpath(fpath, path);
   return get_error(lsetxattr(fpath, name, value, size, flags));
}
int bb_getxattr(const char *path, const char *name, char *value, size_t size)
{
   int retstat = 0;
   char fpath[PATH_MAX];
   bb_fullpath(fpath, path);
   retstat = get_error(lgetxattr(fpath, name, value, size));
   return retstat;
}
int bb_listxattr(const char *path, char *list, size_t size)
{
   int retstat = 0;
   char fpath[PATH_MAX];
   char *ptr;
   bb_fullpath(fpath, path);
   retstat = get_error(llistxattr(fpath, list, size));
   return retstat;
}
int bb_removexattr(const char *path, const char *name)
{
   char fpath[PATH_MAX];
   bb_fullpath(fpath, path);
   return get_error(lremovexattr(fpath, name));
} */

struct fuse_operations bb_oper = {
    .getattr = bb_getattr,
    .readlink = bb_readlink,
    // no .getdir -- that's deprecated
    // .getdir = NULL,
    .mknod = bb_mknod,
    //.mkdir = bb_mkdir,
    .unlink = bb_unlink,
    // .rmdir = bb_rmdir,
    //.symlink = bb_symlink,
    //.rename = bb_rename,
    .link = bb_link,
     //.chmod = bb_chmod,
    //.chown = bb_chown,
    .truncate = bb_truncate,
    .utime = bb_utime,
    .open = bb_open,
    .read = bb_read,
    .write = bb_write,
    //.statfs = bb_statfs,
    //.flush = bb_flush,
    //.release = bb_release,
    //.fsync = bb_fsync,
    .opendir = bb_opendir,
    .readdir = bb_readdir,
    //.releasedir = bb_releasedir,
    //.fsyncdir = bb_fsyncdir,
    .init = bb_init,
    .destroy = bb_destroy,
    //.access = bb_access,
    //.ftruncate = bb_ftruncate,
    //.fgetattr = bb_fgetattr,
    /* .setxattr = bb_setxattr,
    .getxattr = bb_getxattr,
    .listxattr = bb_listxattr,
    .removexattr = bb_removexattr, */
};

int setup_ssh(ssh_session *my_ssh_session, char *hostname)
{
   int rc;
   char *password;

   //printf("Connecting to hostname:%s", hostname);

   // Open session and set options
   *my_ssh_session = ssh_new();
   if (*my_ssh_session == NULL)
      exit(-1);
   ssh_options_set(*my_ssh_session, SSH_OPTIONS_HOST, hostname);
   // ssh_options_set(*my_ssh_session, SSH_OPTIONS_USER, "nalin");

   // Connect to server
   rc = ssh_connect(*my_ssh_session);
   if (rc != SSH_OK)
   {
      fprintf(stderr, "Error connecting to %s: %s\n", hostname,
              ssh_get_error(*my_ssh_session));
      ssh_free(*my_ssh_session);
      exit(-1);
   }

   // Authenticate ourselves
   rc = ssh_userauth_publickey_auto(*my_ssh_session, NULL, NULL);
   if (rc == SSH_AUTH_ERROR)
   {
      fprintf(stderr, "Authentication failed: %s\n",
              ssh_get_error(*my_ssh_session));
      return SSH_AUTH_ERROR;
   }

   return rc;
}

int setup_sftp(sftp_session *my_sftp_session, ssh_session *my_ssh_session)
{
   int rc;
   *my_sftp_session = sftp_new(*my_ssh_session);
   if (my_sftp_session == NULL)
   {
      fprintf(stderr, "Error allocating SFTP session: %s\n",
              ssh_get_error(*my_ssh_session));
      return SSH_ERROR;
   }

   rc = sftp_init(*my_sftp_session);
   if (rc != SSH_OK)
   {
      fprintf(stderr, "Error initializing SFTP session: code %d.\n",
              sftp_get_error(*my_sftp_session));
      sftp_free(*my_sftp_session);
      return rc;
   }
}

int main(int argc, char *argv[])
{
   int fuse_stat;
   struct bb_state *bb_data;

   bb_data = malloc(sizeof(struct bb_state));
   if (bb_data == NULL)
   {
      perror("main calloc");
      abort();
   }

   // Pull the rootdir out of the argument list and save it in my
   // internal data
   bb_data->rootdir = local_dir;
   bb_data->serverdir = server_dir;

   int rc;
   char *hostname;

   if (argc < 1)
   {
      printf("Need Hostname argument");
   }

   // gather hostname at end of line
   hostname = argv[argc - 1];
   argc--;

   bb_data->hostname = hostname;

   // setup ssh session
   rc = setup_ssh(&(bb_data->bb_ssh_session), hostname);

   if (rc < 0)
   {
      exit(-1);
   }

   // setup sftp session
   rc = setup_sftp(&(bb_data->bb_sftp_session), &(bb_data->bb_ssh_session));

   if (rc < 0)
   {
      exit(-1);
   }

   // turn over control to fuse
   fuse_stat = fuse_main(argc, argv, &bb_oper, bb_data);

   return fuse_stat;
}