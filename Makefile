CC=gcc
# Test file name we want to run
TEST_SOURCE = main.c
# Test directory with a source, containing main() 
TEST_DIR = test

# This is the name of the created library
LIB_FNAME = libcjson.a
# Directory where the library will be saved
LIB_DIR = lib
# Library source and include files directories
SRC_DIR = src
INC_DIR = include

LDFLAGS =
LDLIBS = 
TEST =

CFLAGS = -Wall -s -march=native -O2
CFLAGS += -I$(INC_DIR)

LIB_SRC = $(wildcard $(SRC_DIR)/*.c)
LIB_OBJ = $(patsubst %.c,%.o,$(notdir $(LIB_SRC)))
LIB_TARGET = $(LIB_DIR)/$(LIB_FNAME)

TEST_SRC = $(TEST_DIR)/$(TEST_SOURCE)
TEST_OBJ = $(patsubst %.c,%.o,$(notdir $(TEST_SRC)))

ifeq ($(OS),Windows_NT)
	RM = del /Q /F
	EXE = $(basename $(TEST_SOURCE)).exe
else
	RM = -rm -f
	EXE = $(basename $(TEST_SOURCE))
endif

.PHONY: test

print-%: ; @echo $* = $($*)

all: $(LIB_TARGET)

# run test executable aumatically after successful build
test: test_target
	./$(basename $(TEST_SOURCE))

test_target: $(TEST_SRC) $(LIB_TARGET)
	$(CC) $(CFLAGS) $(LDFLAGS) -L./$(LIB_DIR) $^ $(LDLIBS) -o $(basename $(TEST_SOURCE))
	@echo Test executable $@ created..
	@echo Now running..	

.SECONDEXPANSION:

$(LIB_DIR)/$(LIB_FNAME): $(LIB_OBJ) | $$(@D)
	ar rcs $@ $(LIB_OBJ)
	@echo Library $@ created..

%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -c -o $@
	@echo Obj $@ for the library target compiled..	

$(LIB_DIR):
	mkdir $@ >nul

clean:
	$(RM) $(LIB_OBJ) $(TEST_OBJ) $(EXE)