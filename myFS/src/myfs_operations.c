#include "myfs_operations.h"
#include <assert.h>

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
    // fprintf(stderr, "node_exists on path: %s\n", path);
    
    int s = 1, e = 0, l = strlen(path), sublen = 0;
    int i, j, found = 0;
    char *sub = NULL;
    inode *curr = root;
    
    if (!strcmp(path, "/")) {
        return curr;
    }

    do {
        found = 0;
        for(i = s ; i < l ; i++) {
            if(path[i] == '/') {
                e = i;
                break;
            }
        }

        if(i == l)
            e = l;

        if(path[s] == '/')
            s += 1;

        sublen = e - s + 1;
        if(sublen > 0) {
            error_log("Length of part: %d", sublen);
            sub = (char *)malloc(sizeof(char) * sublen);
            sub[sublen - 1] = 0;

            for(i = s, j = 0 ; i < e ; i++, j++)
                sub[j] = path[i];

            // fprintf(stderr, "Part found : %s\n", sub);
            error_log("Searching for part in %d children!", curr->len);


            childrenBlock *curChildrenBlock = get_childrenBlock(curr->ch_inodes);
            int numFullChildrenBlocks = (curr->len / NUM_IN_CHILDREN);
            int numInLastChildBlock = curr->len % NUM_IN_CHILDREN;
            if (numInLastChildBlock == 0) {
                numFullChildrenBlocks -= 1;
                if (numFullChildrenBlocks != -1) {
                    numInLastChildBlock = NUM_IN_CHILDREN;
                }
            }

            // fprintf(stderr, "Full blocks to check: %d\n", numFullChildrenBlocks);
            // fprintf(stderr, "num in last block: %d\n", numInLastChildBlock);

            for (int cblock = 0; cblock < numFullChildrenBlocks; cblock++) {
                for (int cptr = 0; cptr < NUM_IN_CHILDREN; cptr++) {
                    if (!strcmp((curChildrenBlock->children[cptr]).name, sub)) {
                        curr = get_inode(curChildrenBlock->children[cptr].node);
                        found = 1;
                        break;
                    }
                }

                if (found) {
                    break;
                }

                curChildrenBlock = get_childrenBlock(curChildrenBlock->next);
            }

            if (!found) {
                // keep going
                for (int cptr = 0; cptr < numInLastChildBlock; cptr++) {
                    if (!strcmp((curChildrenBlock->children[cptr]).name, sub)) {
                        curr = get_inode(curChildrenBlock->children[cptr].node);
                        found = 1;
                        break;
                    }
                }
            }   

            // for(i = 0 ; i < curr->len ; i++) {
            //     if(!strcmp(((curr->children)[i])->name, sub)) {
            //         curr = curr->children[i];
            //         error_log("curr changed to %p", curr);
            //         found = 1;
            //         break;
            //     }
            // }

            // fprintf(stderr, "Done searching\n");
            if (!sub)
                free(sub);
            if (!found) {
                error_log("%s returning with 0 not found!", __func__);
                return 0;
            }
        }
        else {
            break;
        }
        
        s = e + 1;

    } while(e != l);
    
    error_log("%s returning with %p!", __func__, curr);
    return curr;


    return NULL;
}

int extendByBlock(inode *node, indirectionBlock *indirBlock) {
    uint64_t newBlock = findFirstFreeBlockAndSet();
    if (newBlock == -1) {
        return -1;
    }

    if (node->numDirectAlloc < NUMDIRECT) {
        node->directBlocks[node->numDirectAlloc] = newBlock;
        node->numDirectAlloc += 1;
        return 0;
    }

    if (node->numIndirectAlloc < NUM_IN_INDIRECT) {
        int retCode = 0;
        if (node->numIndirectAlloc == 0) {
            uint64_t newIndirBlock = findFirstFreeBlockAndSet();
            if (newIndirBlock == -1) {
                return -1;
            }
            node->indirectionBlock = newIndirBlock;
            indirBlock = get_indirectionBlock(newIndirBlock);
            retCode = 1;
        }

        indirBlock->blocks[node->numIndirectAlloc] = newBlock;
        node->numIndirectAlloc += 1;
        return retCode;
    }

    linkedBlock *end = get_linkedBlock(node->linkedEnd);
    end->next = newBlock;
    node->numLinkedAlloc += 1;
    node->linkedEnd = newBlock;
    return 0;
}

int expand(inode *node, size_t size) {
    uint64_t len = node->data_size;
    fprintf(stderr, "old size: %d, newSize: %d\n", len, size);

    if (len > size) {
        return 0;
    }

    // check if in same block
    int curEndBlock = (len / DATA_IN_BLOCK) + 1;
    if (len % DATA_IN_BLOCK == 0) {
        curEndBlock -= 1;
    }

    int newEndBlock = (size / DATA_IN_BLOCK) + 1;
    if (size % DATA_IN_BLOCK == 0) {
        newEndBlock -= 1;
    }

    indirectionBlock *indirBlock = get_indirectionBlock(node->indirectionBlock);
    for (int i = 0; i < newEndBlock - curEndBlock; i++) {
        int extendRet = extendByBlock(node, indirBlock);
        if (extendRet == -1) {
            return -1;
        }

        if (extendRet == 1) {
            indirBlock = get_indirectionBlock(node->indirectionBlock);
        }
    }

    return 0;
}

// int expand(inode *node, size_t size) {
//     fprintf(stderr, "NUM IN DIRECT: %ld\n", NUMDIRECT);
//     fprintf(stderr, "NUM IN INDIRECT: %ld\n", NUM_IN_INDIRECT);

//     // print_bitmap(70);

//     uint64_t len = node->len;

//     len = 0;
//     size = 8138113;
//     fprintf(stderr, "old size: %d, newSize: %d\n", len, size);

//     if (len > size) {
//         return 0;
//     }

//     long lenSoFar = 0;

//     if (len < BLOCKSIZE * (NUMDIRECT + NUM_IN_INDIRECT)) {
//         // check if in same block
//         int curEndBlock = (len / BLOCKSIZE) + 1;
//         if (len % BLOCKSIZE == 0) {
//             curEndBlock -= 1;
//         }

//         int newEndBlock = (size / BLOCKSIZE) + 1;
//         if (size % BLOCKSIZE == 0) {
//             newEndBlock -= 1;
//         }

//         fprintf(stderr, "curEnd: %d, newEnd: %d\n", curEndBlock, newEndBlock);
//         lenSoFar = curEndBlock * BLOCKSIZE;

//         if (curEndBlock == newEndBlock) {
//             return 0;
//         }

//         long numToAddForNow = newEndBlock;
//         if (newEndBlock > (NUMDIRECT + NUM_IN_INDIRECT)) {
//             numToAddForNow = (NUMDIRECT + NUM_IN_INDIRECT);
//         }
//         numToAddForNow -= curEndBlock;

//         if (curEndBlock < NUMDIRECT) {
//             long numDirectToAdd = numToAddForNow;
//             if (numDirectToAdd > NUMDIRECT) {
//                 numDirectToAdd = NUMDIRECT;
//             }
//             numDirectToAdd -= curEndBlock;

//             for (int i = curEndBlock; i < numDirectToAdd; i++) {
//                 // fprintf(stderr, "directSet\n");
//                 node->directBlocks[i] = findFirstFreeBlockAndSet();
//             }

//             lenSoFar += numDirectToAdd * BLOCKSIZE;
//             node->numDirectAlloc += numDirectToAdd;
//             numToAddForNow -= numDirectToAdd;
//         }

//         // create indirection block if needed
//         if (curEndBlock <= NUMDIRECT) {
//             // fprintf(stderr, "addIndirection\n");
//             node->indirectionBlock = findFirstFreeBlockAndSet();
//         }

//         indirectionBlock *indirBlock = get_indirectionBlock(node->indirectionBlock);
        
//         int startPos = curEndBlock - NUMDIRECT;
//         if (startPos < 0) {
//             startPos = 0;
//         }

//         // fprintf(stderr, "adding %d to indir\n", numToAddForNow);
//         int temp = 0;
//         for (int i = startPos; i < startPos + numToAddForNow; i++) {
//             // fprintf(stderr, "addToIndirection\n");
//             indirBlock->blocks[i] = findFirstFreeBlockAndSet();
//             temp += 1;
//         }
//         // fprintf(stderr, "added %d to indir\n", temp);

//         lenSoFar += (numToAddForNow - NUMDIRECT) * BLOCKSIZE;
//         node->numIndirectAlloc += numToAddForNow;
//     } else {
//         lenSoFar = BLOCKSIZE * (NUMDIRECT + NUM_IN_INDIRECT);
//     }

//     // fprintf(stderr, "lenSoFar: %ld\n", lenSoFar);

//     if (lenSoFar >= size) {
//         return 0;
//     }

//     assert(size > (BLOCKSIZE * (NUMDIRECT + NUM_IN_INDIRECT)));

//     long curLinkedLen = 0;
//     if (len > BLOCKSIZE * (NUMDIRECT + NUM_IN_INDIRECT)) {
//         curLinkedLen = len - (BLOCKSIZE * (NUMDIRECT + NUM_IN_INDIRECT));
//     }
//     long newLinkedLen = size - (BLOCKSIZE * (NUMDIRECT + NUM_IN_INDIRECT));

//     // check if in same block
//     int curLinkedBlock = (curLinkedLen / DATA_IN_LINKED) + 1;
//     if (curLinkedLen % DATA_IN_LINKED == 0) {
//         curLinkedBlock -= 1;
//     }

//     int newLinkedBlock = (newLinkedLen / DATA_IN_LINKED) + 1;
//     if (newLinkedLen % DATA_IN_LINKED == 0) {
//         newLinkedBlock -= 1;
//     }

//     fprintf(stderr, "curEnd: %d, newEnd: %d\n", curLinkedBlock, newLinkedBlock);

//     if (curLinkedBlock == newLinkedBlock) {
//         return 0;
//     }

//     int numLinkToAppend = newLinkedBlock - curLinkedBlock;
//     if (node->numLinkedAlloc == 0) {
//         // fprintf(stderr, "addLinkedFirst\n");
//         node->linkedStart = findFirstFreeBlockAndSet();
//         node->linkedEnd = node->linkedStart;
//         node->numLinkedAlloc += numLinkToAppend;
//         numLinkToAppend -= 1;
//     } else {
//         node->numLinkedAlloc += numLinkToAppend;
//     }

//     uint64_t curBack = node->linkedEnd;
//     for (int i = 0; i < numLinkToAppend; i++) {
//         // fprintf(stderr, "addLinked\n");
//         linkedBlock *curBackBlock = get_linkedBlock(curBack);
//         curBackBlock->next = findFirstFreeBlockAndSet();
//         curBack = curBackBlock->next;
//     }

//     node->linkedEnd = curBack;

//     fprintf(stderr, "direct %d\n", node->numDirectAlloc);
//     fprintf(stderr, "indire %d\n", node->numIndirectAlloc);
//     fprintf(stderr, "linked %d\n", node->numLinkedAlloc);
// }


int myfs_getattr(const char *path, struct stat *s) {
    fprintf(stderr, "\ngetattr on path: %s\n", path);
    error_log("%s called on path : %s", __func__, path);

    inode *curr = node_exists(path);
    if (!curr) {
        fprintf(stderr, "path not found\n");
        error_log("curr = %p ; not found returning!", curr);
        return -ENOENT;
    }

    // fprintf(stderr, "name found: %s\n", curr->name);

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
            // fprintf(stderr, "type not supported\n");
            return -ENOTSUP;
    }

    s->st_nlink += curr->len;
    s->st_uid = curr->uid;
    s->st_gid = curr->gid;

    s->st_size = curr->data_size;
    s->st_blocks = curr->block_count;
    s->st_blksize = BLOCKSIZE;

    s->st_atime = (curr->st_atim).tv_sec;
    s->st_mtime = (curr->st_mtim).tv_sec;
    s->st_ctime = (curr->st_ctim).tv_sec;
    
    return 0;
}


int myfs_mknod(const char *path, mode_t m, dev_t d) {
    uint64_t block_num = findFirstFreeBlockAndSet();

    // Extract fileName and path
    int pathLength = strlen(path), sublen = 0;
    int i, j;
    char *temp = (char *)malloc(sizeof(char) * (pathLength + 1));     //to store path until one level higher than path given
    strcpy(temp, path);
    
    for(i = pathLength - 1 ; temp[i] != '/' ; i--);     //find first / from back of path
    temp[i] = 0;
    i += 1;

    if(i == 1) {  //if root's child
        error_log("Found to be root's child!");
        strcpy(temp, "/");
    }

    sublen = (pathLength - i + 1);
    char *fileName = (char *)malloc(sizeof(char) * sublen);
    for(j = 0 ; i < pathLength ; i++, j++)                  //extract name of file from full path   
        fileName[j] = path[i];
    fileName[sublen - 1] = 0;




    // fprintf(stderr, "PATH: %s\n", temp);
    // fprintf(stderr, "PATH: %s\n", fileName);



    // // just add onto root
    // uint64_t parent_num = 0;

    inode *parent = node_exists(temp);
    uint32_t pos = parent->len;
    parent->len += 1;
    // fprintf(stderr, "putting at pos %d\n", pos);
    childrenBlock *pChildren = get_childrenBlock(parent->ch_inodes);
    if (pos % NUM_IN_CHILDREN == 0) {
        if (pos == 0) {
            pChildren->children[pos].node = block_num;
            strcpy(pChildren->children[pos].name, fileName);
        } else {
            for (int cblock = 0; cblock < (pos / NUM_IN_CHILDREN) - 1; cblock++) {
                pChildren = get_childrenBlock(pChildren->next);
            }

            pChildren->next = findFirstFreeBlockAndSet();
            pChildren = get_childrenBlock(pChildren->next);

            pChildren->children[0].node = block_num;
            strcpy(pChildren->children[0].name, fileName);
        }
    } else {
        for (int cblock = 0; cblock < pos / NUM_IN_CHILDREN; cblock++) {
            pChildren = get_childrenBlock(pChildren->next);
        }


        pChildren->children[pos % NUM_IN_CHILDREN].node = block_num;
        strcpy(pChildren->children[pos % NUM_IN_CHILDREN].name, fileName);
    }

    // fprintf(stderr, "test: %s\n", )


    inode *newNode = get_inode(block_num);

    // add names
    strcpy(newNode->name, fileName);
    newNode->fullname = (char *)malloc(sizeof(char) * (pathLength + 1));       //add full name to FS node
    strcpy(newNode->fullname, path);

    newNode->ch_inodes = NULL;

    newNode->inode_no = block_num;
    newNode->type = 1;
    newNode->len = 0;
    newNode->parent_inode = parent->inode_no;

    newNode->uid = getuid();
    newNode->gid = getgid();

    newNode->data_size = 0;
    newNode->block_count = 0;

    newNode->numDirectAlloc = 0;
    newNode->numIndirectAlloc = 0;
    newNode->numLinkedAlloc = 0;

    time(&(newNode->st_ctim).tv_sec);
    newNode->st_mtim = newNode->st_atim = newNode->st_ctim;

    newNode->perms = DEF_FILE_PERM;
    newNode->nlinks = 1;

    return 0;
}


int myfs_mkdir(const char *path, mode_t m) {
    fprintf(stderr, "\nmyfs_mkdir\n");
    return -1;
}


int myfs_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
    fprintf(stderr, "\nmyfs_readdir\n");

    filler(buffer, ".", NULL, 0);
    filler(buffer, "..", NULL, 0);

    inode *curr = node_exists(path);
    if (curr == 0) {
        return -ENOENT;
    }

    childrenBlock *curChildrenBlock = get_childrenBlock(curr->ch_inodes);
    int numFullChildrenBlocks = (curr->len / NUM_IN_CHILDREN);
    int numInLastChildBlock = curr->len % NUM_IN_CHILDREN;
    if (numInLastChildBlock == 0) {
        numFullChildrenBlocks -= 1;
        if (numFullChildrenBlocks != -1) {
            numInLastChildBlock = NUM_IN_CHILDREN;
        }
    }

    for (int cblock = 0; cblock < numFullChildrenBlocks; cblock++) {
        for (int cptr = 0; cptr < NUM_IN_CHILDREN; cptr++) {
            filler(buffer, (curChildrenBlock->children[cptr]).name, NULL, 0);
        }

        curChildrenBlock = get_childrenBlock(curChildrenBlock->next);
    }

    // keep going
    for (int cptr = 0; cptr < numInLastChildBlock; cptr++) {
        filler(buffer, (curChildrenBlock->children[cptr]).name, NULL, 0);
    }

    return 0;
}


int myfs_rmdir(const char *path) {
    fprintf(stderr, "\nmyfs_rmdir\n");
}


int myfs_open(const char *path, struct fuse_file_info *fi) {
    fprintf(stderr, "\nmyfs_open\n");

    error_log("%s called on path : %s", __func__, path);
    
    inode *curr = node_exists(path);
    uint32_t check = 0;
    switch(fi->flags & O_ACCMODE) {
        case O_RDWR:
            error_log("O_RDWR");
            check = check | 0666;
            break;
        
        case O_RDONLY:
            error_log("O_RDONLY");
            check = check | 0444;
            break;

        case O_WRONLY:
            error_log("O_WRONLY");
            check = check | 0222;
            break;
    }

    error_log("%d %d %d", curr->perms, check, curr->perms & check);
    if(curr->perms & check) {
        error_log("Allowed to open!");
        return 0;
    }

    return -EACCES;
}


int myfs_read(const char *path, char *buf, size_t size, off_t offset,struct fuse_file_info *fi) {
    fprintf(stderr, "\nmyfs_read\n");

    inode *curr = node_exists(path);
    size_t len = curr->data_size;

    memcpy(buf, get_simpleBlock(curr->directBlocks[0])->data, size);

    fprintf(stderr, "stuff: %s\n", get_simpleBlock(curr->directBlocks[0])->data);

    return size;
}


void writeToInode(inode *node, const char *buf, size_t size, off_t offset) {
    fprintf(stderr, "writing %d bytes to offset %d\n", size, offset);

    if (offset < NUMDIRECT * DATA_IN_BLOCK) {
        // in direct block

        // get the direct block
        int block = offset / DATA_IN_BLOCK;
        off_t blockOffset = offset - (block * DATA_IN_BLOCK);
        simpleBlock *dirBlock = get_simpleBlock(node->directBlocks[block]);
        memcpy(dirBlock->data + blockOffset, buf, size);
        fprintf(stderr, "Writing to direct block %d at offset %d\n", block, blockOffset);

    } else if (offset < (NUMDIRECT + NUM_IN_INDIRECT) * DATA_IN_BLOCK) {
        // in indirect block

        // get the indirect block
    } else {
        // in linked

        // find linked block
    }
}

int myfs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {   
    fprintf(stderr, "\nmyfs_write\n");

    inode *curr = node_exists(path);
    size_t len = curr->data_size;
    size_t newLen = size + offset;    

    expand(curr, newLen);
    if (newLen > len) {
        curr->data_size = newLen;
    }

    // handle first
    off_t curPos = offset;
    size_t sizeLeft = size;
    int endOfFirst = ((curPos / DATA_IN_BLOCK) + 1) * DATA_IN_BLOCK;
    if (endOfFirst > curPos + sizeLeft) {
        endOfFirst = curPos + sizeLeft;
    }
    size_t curSize = endOfFirst - curPos;

    writeToInode(curr, buf, curSize, curPos);
    curPos += curSize;
    buf += curSize;
    sizeLeft -= curSize;

    // handle mid
    int temp = 0;
    while (sizeLeft > 0) {
        fprintf(stderr, "size left: %d\n", sizeLeft);
        int endOfThis = ((curPos / DATA_IN_BLOCK) + 1) * DATA_IN_BLOCK;
        if (endOfThis > curPos + sizeLeft) {
            endOfThis = curPos + sizeLeft;
        }
        curSize = endOfFirst - curPos;

        writeToInode(curr, buf, curSize, curPos);
        curPos += curSize;
        buf += curSize;
        sizeLeft -= curSize;
        temp += 1;
        if (temp == 15) {
            break;
        }
    }


    // fprintf(stderr, "stuff: %s\n", get_simpleBlock(curr->directBlocks[0])->data);
    

    return size;
}


int myfs_utimens(const char *path, struct utimbuf *tv) {
    fprintf(stderr, "\nmyfs_utimens\n");

    error_log("%s called on path : %s", __func__, path);
    error_log("atime = %s; mtime = %s ", ctime(&(tv->actime)), ctime(&(tv->modtime)));

    inode *curr = node_exists(path);

    if (curr == 0)
        return -ENOENT;
    
    if (curr->st_atim.tv_sec < tv->actime)
        curr->st_atim.tv_sec = tv->actime;

    if (curr->st_mtim.tv_sec < tv->modtime)
        curr->st_mtim.tv_sec = tv->modtime;
    
    /*
    curr->st_atim.tv_nsec = tv[0].tv_nsec;
    curr->st_mtim.tv_nsec = tv[1].tv_nsec;
    */

    return 0;
}


int myfs_truncate(const char* path, off_t size) {
    fprintf(stderr, "\nmyfs_truncate\n");

    error_log("%s called on path : %s ; to change to size = %d ;", __func__, path,size);

    inode *curr = node_exists(path);
    size_t len = curr->data_size;
    fprintf(stderr, "old size: %d; new size: %d\n", len, size);

    error_log("curr found at %p with data %d", curr, len);
    

    if (len < size) {
        // resize up!
        

    } else if (len > size) {
        // resize down!

    }

    curr->data_size = size;
    return 0;
}


int myfs_unlink(const char *path) {
    fprintf(stderr, "\nmyfs_unlink\n");
}


int myfs_rename(const char *from, const char *to) {
    fprintf(stderr, "\nmyfs_rename\n");
}


int myfs_chmod(const char *path, mode_t setPerm) {
    fprintf(stderr, "\nmyfs_chmod\n");

    error_log("%s called on path : %s ; to set : %d", __func__, path, setPerm);

    inode *curr = node_exists(path);
    if (!curr) {
        error_log("File not found!");
        
        return -ENOENT;
    }

    uint32_t curr_uid = getuid();
    if (curr_uid == curr->uid || !curr_uid) {        // if owner is doing chmod or root is
        error_log("Current user (%d) has permissions to chmod", curr_uid);

        curr->perms = setPerm;
    }
    else {
        error_log("Current user (%d) DOESNT permissions to chown", curr_uid); 
        return -EACCES;
    }

    return 0;
}


int myfs_chown(const char *path, uid_t u, gid_t g) {
    fprintf(stderr, "\nmyfs_chown\n");

    error_log("%s called on path : %s ; to set : u = %d\t g = %d", __func__, path, u, g);

    inode *curr = node_exists(path);
    if (curr == 0) {
        error_log("File not found!");
        return -ENOENT;
    }

    uid_t curr_user = getuid();

    if(curr_user != 0 && curr_user != curr->uid) {       // only root or owner can chown a file
        error_log("Current user (%d) DOESNT permissions to chown file owned by %d", curr_user, curr->uid); 
        return -EACCES;
    }
    error_log("Current user (%d) has permissions to chown", curr_user); 

    if(u != -1)
        curr->uid = u;

    if(g != -1)
        curr->gid = g;

    return 0;
}


int myfs_flush(const  char *path, struct fuse_file_info *fi) {
    fprintf(stderr, "\nmyfs_flush\n");

    // error_log("%s called on path : %s", __func__, path);

    // inode *node = node_exists(path);
    // void *buf = NULL;
    // uint64_t blocks = constructBlock(node, &buf);
    // diskWriter(buf, blocks, node->inode_no);
    // error_log("Wrote file!");
    // //free(buf);    // free(): invalid pointer = error thrown, unknown reason

    return 0;
}