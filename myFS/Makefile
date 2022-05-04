mountpoint = ./mountpoint
includepath = -I./include/
srcprefix = ./src/
files = $(srcprefix)myfs_main.c $(srcprefix)myfs_operations.c $(srcprefix)bitmap.c
compileflags = -D_FILE_OFFSET_BITS=64
opflag = -o ffs
neededflag = `pkg-config fuse --libs` -DFUSE_USE_VERSION=22 -lm

all: run cleanup

run: compile cleanup
	./ffs -f $(mountpoint)

drun: dcompile cleanup
	./ffs -d -f -s $(mountpoint)

bgrun: compile
	./ffs $(mountpoint)

compile: checkdir
	gcc -Wall $(includepath) $(files) $(compileflags) $(opflag) $(neededflag)

dcompile: checkdir
	gcc -Wall $(includepath) -g -DERR_FLAG $(files) $(compileflags) $(opflag) $(neededflag)

checkdir:
	if [ -d "$(mountpoint)" ]; then echo "mountpoint exists"; else mkdir $(mountpoint); fi

test:
	gcc -o program program.c

cleanup :
	-fusermount -u $(mountpoint)
