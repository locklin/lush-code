all: driver1 driver2 driver3 driver4 driver5

CC = gcc -lm -W -Wall -pedantic -Wmissing-prototypes \
	-Wredundant-decls -Wnested-externs -Wdisabled-optimization \
	-ansi -g -fexceptions -Wno-parentheses -Wshadow -Wcast-align \
	-Winline -Wstrict-prototypes -Wno-unknown-pragmas -g

# for speed:
# CC = gcc -lm -O3

OBJ = cg_descent.o

$(OBJ): $(INCLUDE)

driver1: $(OBJ) driver1.c
	$(CC) -o driver1 driver1.c $(OBJ)

driver2: $(OBJ) driver2.c
	$(CC) -o driver2 driver2.c $(OBJ)

driver3: $(OBJ) driver3.c
	$(CC) -o driver3 driver3.c $(OBJ)

driver4: $(OBJ) driver4.c
	$(CC) -o driver4 driver4.c $(OBJ)

driver5: $(OBJ) driver5.c
	$(CC) -o driver5 driver5.c $(OBJ)

clean:
	rm *.o

purge:
	rm *.o driver1 driver2 driver3 driver4 driver5
