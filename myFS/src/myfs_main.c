#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/mman.h>

#include <fuse.h>
#include "myfs_operations.h"
#include "bitmap.h"

static struct fuse_operations myfs_operations = {
    .getattr        = myfs_getattr,
    //.readlink     = myfs_readlink,
    .mknod          = myfs_mknod,
    .mkdir          = myfs_mkdir,
    .unlink         = myfs_unlink,
    .rmdir	        = myfs_rmdir,
    //.symlink	    = myfs_symlink,
    .rename	        = myfs_rename,
    //.link	        = myfs_link,
    .chmod	        = myfs_chmod,
    .chown	        = myfs_chown,
    .truncate       = myfs_truncate,
    .open	        = myfs_open,
    .read	        = myfs_read,
    .write	        = myfs_write,
    //.statfs	    = myfs_statfs,
    .flush	        = myfs_flush,
    //.release	    = myfs_release,
    //.fsync	    = myfs_fsync,
    //.setxattr	    = myfs_setxattr,
    //.getxattr	    = myfs_getxattr,
    //.listxattr    = myfs_listxattr,
    //.removexattr  = myfs_removexattr,
    //.opendir	    = myfs_opendir,
    .readdir	    = myfs_readdir,
    //.releasedir	= myfs_releasedir,
    //.fsyncdir	    = myfs_fsyncdir,
    //.init	        = myfs_init,
    //.destroy	    = myfs_destroy,
    //.access	    = myfs_access,
    //.create	    = myfs_create,
    //.ftruncate	= myfs_ftruncate,
    //.fgetattr     = myfs_fgetattr,
    //.lock	        = myfs_lock,
    .utime	        = myfs_utimens,
    //.bmap	        = myfs_bmap,
    //.ioctl	    = myfs_ioctl,
    //.poll	        = myfs_poll,
    //.write_buf	= myfs_write_buf,
    //.read_buf	    = myfs_read_buf,
    //.flock	    = myfs_flock,
    //.fallocate	= myfs_fallocate,
};

int main(int argc, char *argv[]) {
    int fuse_stat;

    disk = mmap(NULL, FSSIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
    bitmap = mmap(NULL, sizeof(word_t) * NUMWORDS, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);

    fprintf(stderr, "Size of inode: %d\n", sizeof(inode));
    fprintf(stderr, "Size of simpleBlock: %d\n", sizeof(simpleBlock));
    fprintf(stderr, "Size of indirectionBlock: %d\n", sizeof(indirectionBlock));
    fprintf(stderr, "Size of linkedBlock: %d\n", sizeof(linkedBlock));

    // turn over control to fuse
    fuse_stat = fuse_main(argc, argv, &myfs_operations);
    return fuse_stat;
}