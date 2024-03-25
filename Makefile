CC=gcc

CFLAGS= -Wall -pedantic -std=c99
LDFLAGS =
DEBUG= -g

HEADER = mytar.h archiveprint.h archiveextract.h archivebuild.h
MYTAR = mytar.c archiveprint.c archiveextract.c archivebuild.c utils.c

all: $(MYTAR) $(HEADER)
	$(CC) $(DEBUG) $(LDFLAGS) -o mytar $(MYTAR)
 
%.o: %.c
	$(CC) $(DEBUG) $(CFLAGS) -c -o $@ $^

.PHONY: clean

clean:
	rm -f *.o