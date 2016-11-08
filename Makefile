LIB=./lib/
INC=./include/
BIN=./bin/
SRC=./src/
TST=./teste/

DIRS=$(LIB) $(INC) $(BIN) $(SRC) $(TST)

CC=gcc
CCFLAGS=-m32 -Wall -I$(INC)

_OBJS=$(wildcard $(SRC)*.c)
OBJS=$(addprefix $(BIN), $(notdir $(_OBJS:.c=.o)))

.PHONY: directories tests

all: directories $(OBJS)
	ar crs $(LIB)libt2fs.a $(LIB)apidisk.o $(LIB)bitmap2.o $(OBJS)

directories:
	mkdir -p -v $(DIRS)

$(BIN)%.o: $(SRC)%.c
	$(CC) $(CCFLAGS) -o $@ -c $<

clean:
	find $(BIN) $(LIB) $(TST) -type f \
	! -name 'apidisk.o' ! -name 'bitmap2.o' ! -name "*.c" -delete
