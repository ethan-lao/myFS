CC = gcc
fuse_test: filesys.c
	${CC} filesys.c -o fuse_filesys `pkg-config fuse --cflags --libs` -lssh  
	${CC} program.c -o program 
	${CC} printFileSize.c -o printFileSize
	mkdir mnt
clean:
	rm fuse_filesys
	rm program
	rm printFileSize
	rm -rf mnt

# ./fuse_filesys -d -f ./mnt ethanlao@chess.cs.utexas.edu
# ./fuse_filesys -f ./mnt ethanlao@chess.cs.utexas.edu
# sshfs ethanlao@chess.cs.utexas.edu:/tmp/fuse_test mnt