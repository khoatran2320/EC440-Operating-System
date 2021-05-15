override CFLAGS :=  -Wall -std=gnu99 -pedantic -O0 -g $(CFLAGS)
override LDLIBS := -pthread $(LDLIBS)

fs.o: fs.c
disk.o:	disk.c
# Automatically discover all test files
test_c_files=$(shell find tests -type f -name '*.c')
test_o_files=$(test_c_files:.c=.o)
test_files=$(test_c_files:.c=)

# The intermediate test .o files shouldn't be auto-deleted in test runs; they
# may be useful for incremental builds while fixing fs.c bugs.
.SECONDARY: $(test_o_files)

.PHONY: clean check checkprogs

tests/%: tests/%.o fs.o disk.o
	$(CC) $(LDFLAGS) $+ $(LOADLIBES) $(LDLIBS) -o $@

# Build all of the test programs
checkprogs: $(test_files)

clean:
	rm -f *.o $(test_files) $(test_o_files)
