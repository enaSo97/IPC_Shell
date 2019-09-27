#
#	Makefile for dish interpreter
#

EXE	= dish
OBJS = \
	dish_main.o \
	dish_run.o \
	dish_tokenize.o

CFLAGS = -g

$(EXE) : $(OBJS)
	$(CC) $(CFLAGS) -o $(EXE) $(OBJS)

clean :
	- rm -f $(EXE)
	- rm -f $(OBJS)
	- rm -f tags

tests: test1 test2

test1:
	@echo Test 1 "ls *.c"
	ls *.c > ./expected/test1.expected
	./dish < ./cmd/test1.cmd > ./output/test1.output
	- diff ./expected/test1.expected ./output/test1.output

test2:
	@echo Test 2 "cat dish_main.c | grep include"
	cat dish_main.c | grep include > ./expected/test2.expected
	./dish < ./cmd/test2.cmd > ./output/test2.output
	- diff ./expected/test2.expected ./output/test2.output

test3:
	@echo Test 3 "ls -l *.c | sort -M | cat"
	ls -l *.c | sort -M | cat > ./expected/test3.expected
	./dish < ./cmd/test3.cmd > ./output/test3.output
	- diff ./expected/test3.expected ./output/test3.output

test4:
	@echo Test 4 "cat *.h | grep include | sort -M | cat"
	cat *.h | grep include | sort -M | cat > ./expected/test4.expected
	./dish < ./cmd/test4.cmd > ./output/test4.output
	- diff ./expected/test4.expected ./output/test4.outpu

test5:
	@echo Test 5 "echo Hello World! | cat"
	echo "Hello World!" | cat > ./expected/test5.expected
	./dish < ./cmd/test5.cmd > ./output/test5.output
	- diff ./expected/test5.expected ./output/test5.output

test6:
	@echo Test 5 "ls *.h dish_main.c"
	ls *.h dish_main.c > ./expected/test6.expected
	./dish < ./cmd/test6.cmd > ./output/test6.output
	- diff ./expected/test6.expected ./output/test6.output

# ctags makes a database of source code tags for use with vim and co
tags ctags : dummy
	- ctags *.c

# a rule like this dummy rule can be use to make another rule always
# run - ctags depends on this, so therefore ctags will always be
# executed by "make ctags" as make is fooled into thinking that it
# has performed a dependency each time, and therefore re-does the work
dummy:
