.SUFFIXES:
.SUFFIXES: .o .c .check .out .xml .log

CFLAGS=-g -Os -std=c99 -Wpedantic -Wall -Wextra -Werror
CHECKLIBS=$(shell pkg-config --libs check)
LDFLAGS=--fatal-warnings

ALLOBJECT=$(sort \
	$(patsubst %.c,%.o,$(wildcard *.c)) \
	$(patsubst %.check,%.o,$(wildcard *.check)))
CHECKC=$(patsubst %.check,%.c,$(wildcard *.check))
CCOUNT=$(words $(filter-out $(CHECKC), $(wildcard *.c)))
AOUTOBJECT=$(filter-out $(CHECKC:.c=.o), $(filter-out $(LIBOBJECT), $(ALLOBJECT)))
UNITOBJECT=$(filter-out main.o start.o, $(ALLOBJECT))

all:	run

test:	unittest.out
	./unittest.out

grade:	unittest.out
	./unittest.out

valgrind:	unittest.out
	rm -f valgrind.log
	-valgrind --leak-check=full --track-fds=yes --log-file=valgrind.log ./unittest.out
	cat valgrind.log

run:	a.out
	./a.out

debug:	a.out $(HOME)/.gdbinit
	gdb ./a.out

$(HOME)/.gdbinit:
	echo set auto-load safe-path / > $(HOME)/.gdbinit

.c.o:
	gcc $(CFLAGS) $< -c -o $@

.check.c:
	checkmk $< > $@

a.out:	$(AOUTOBJECT)
ifeq ($(CCOUNT), 0)
	ld $(LDFLAGS) $^
else
	gcc $(CFLAGS) $^ -o $@
endif

unittest.out:	$(UNITOBJECT)
	gcc $(CFLAGS) $^ $(CHECKLIBS) -o $@

setup:
	# install build tools, unit test library, and valgrind
	sudo apt install -y build-essential make gdb valgrind check pkg-config python3

clean:
	rm -f $(ALLOBJECT) $(CHECKC) *.out *.xml *.log
