#include "myfs_operations.h"

// Error logging for THIS MODULE, helps differentiate from logging of other modules
// Prints errors and logging info to STDOUT
// Passes format strings and args to vprintf, basically a wrapper for printf
static void error_log(char *fmt, ...) {
#ifdef ERR_FLAG
    va_list args;
    va_start(args, fmt);
    
    printf("\n");
    printf("FfS OPS : ");
    vprintf(fmt, args);
    printf("\n");

    va_end(args);
#endif
}


inode *node_exists(const char *path) {
    return NULL;
}


int myfs_getattr(const char *path, struct stat *s) {
    error_log("%s called on path : %s", __func__, path);

    inode *curr = NULL;
    if(!(curr = node_exists(path))) {
        error_log("curr = %p ; not found returning!", curr);
        return -ENOENT;
    }

    memset(s, 0, sizeof(struct stat));

    s->st_dev = 666;

    switch(curr->type) {
        case 1:
            s->st_mode = S_IFREG | curr->perms;
            s->st_nlink = 1;
            break;

        case 2:
            s->st_mode = S_IFDIR | curr->perms;
            s->st_nlink = 2;
            break;

        default:
            error_log("Type not supported : %d", curr->type);
            return -ENOTSUP;
    }

    s->st_nlink += curr->len;
    s->st_uid = curr->uid;
    s->st_gid = curr->gid;

    s->st_size = curr->data_size;
    s->st_blocks = curr->num_blocks;

    s->st_atime = (curr->st_atim).tv_sec;
    s->st_mtime = (curr->st_mtim).tv_sec;
    s->st_ctime = (curr->st_ctim).tv_sec;
    
    return 0;
}


int myfs_mknod(const char *path, mode_t m, dev_t d) {

}


int myfs_mkdir(const char *path, mode_t m) {

}


int myfs_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {

}


int myfs_rmdir(const char *path) {

}


int myfs_open(const char *path, struct fuse_file_info *fi) {

}


int myfs_read(const char *path, char *buf, size_t size, off_t offset,struct fuse_file_info *fi) {

}


int myfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {   
    
}


int myfs_utimens(const char *path, struct utimbuf *tv) {
    
}


int myfs_truncate(const char* path, off_t size) {

}


int myfs_unlink(const char *path) {
  
}


int myfs_rename(const char *from, const char *to) {
    
}


int myfs_chmod(const char *path, mode_t setPerm) {
    
}


int myfs_chown(const char *path, uid_t u, gid_t g) {

}


int myfs_flush(const  char *path, struct fuse_file_info *fi) {

}