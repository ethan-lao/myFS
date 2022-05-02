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

    // int fd = open("./backing.txt", O_RDWR);
    // disk = mmap(0, FSSIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    disk = mmap(0, FSSIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
    // memset(disk, 0, FSSIZE);
    // fprintf(stderr, "%d\n", *(disk));
    // fprintf(stderr, "%d\n", *(disk + FSSIZE - 20));
    // fprintf(stderr, "%x\n", (disk + FSSIZE - 20));

    // fprintf(stderr, "TEST\n");
    // fprintf(stderr, "disk: %ld\n", get_linkedBlock(0)->next);
    // fprintf(stderr, "disk: %ld\n", get_linkedBlock(1026)->next);
    // fprintf(stderr, "disk: %x\n", get_linkedBlock(0));
    // fprintf(stderr, "disk: %x\n", get_linkedBlock(1026));
    // fprintf(stderr, "disk: %x\n", get_linkedBlock(1026) - get_linkedBlock(0));
    // fprintf(stderr, "disk: %ld\n", FSSIZE);
    bitmap = mmap(NULL, NUMWORDS * sizeof(word_t), PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
    print_bitmap(70);

    fprintf(stderr, "Size of inode: %ld\n", sizeof(inode));
    fprintf(stderr, "Size of simpleBlock: %ld\n", sizeof(simpleBlock));
    fprintf(stderr, "Size of indirectionBlock: %ld\n", sizeof(indirectionBlock));
    fprintf(stderr, "Size of linkedBlock: %ld\n", sizeof(linkedBlock));
    fprintf(stderr, "Size of childrenBlock: %ld\n", sizeof(childrenBlock));
    fprintf(stderr, "Size of Num in children: %ld\n", NUM_IN_CHILDREN);

    // --------------
    // setup root dir
    // --------------

    // update the bitmap
    set_bit(0);
    print_bitmap(70);
    uint64_t chInode = findFirstFreeBlockAndSet();
    print_bitmap(70);

    root->type = 2;

    // names
    root->fullname = (char *)malloc(sizeof(char) * 2);
    strcpy(root->fullname, "/");
    strcpy(root->name, "/");

    root->inode_no = 0;
    root->ch_inodes = chInode;
    root->len = 0;
    root->nlinks = 2;
    root->parent_inode = NULL;
    root->data_size = 0;
    root->block_count = 0;
    root->uid = getuid();
    root->gid = getgid();
    root->perms = DEF_DIR_PERM;

    root->numDirectAlloc = 0;
    root->numIndirectAlloc = 0;
    root->numLinkedAlloc = 0;

    time(&(root->st_ctim).tv_sec);
    root->st_mtim = root->st_atim = root->st_ctim;

    fprintf(stderr, "name: %s\n", root->name);
    // fprintf(stderr, "chInod: %d\n", root->ch_inodes * BLOCKSIZE);

    // turn over control to fuse
    fuse_stat = fuse_main(argc, argv, &myfs_operations);
    return fuse_stat;
}