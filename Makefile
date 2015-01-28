CC=gcc
CCFLAGS=-W -Wall -pedantic -std=gnu99 -I./include -I/usr/include
LDFLAGS=
SRC=$(wildcard src/*.c)
OBJ=$(SRC:.c=.o)
EXEC=Simulateur

all: $(EXEC)

$(EXEC): $(OBJ)
  $(CC) -o $(EXEC) $^ $(LDFLAGS)

%.o: %.c
  %$(CC) -o $@ -c $< $(CCFLAGS)

clean:
  @echo "Clean .o files"
  rm -rf */*.o

mrproper: clean
  @echo "Remove $(EXEC) file"
  rm -rf $(EXEC)

rebuild:
  @echo " --- Cleaning --- "
  make mrproper
  @echo " \n\n--- Building --- "
  make all
