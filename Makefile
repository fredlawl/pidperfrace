
.PHONY: all
all: pidperfrace
	./pidperfrace

pidperfrace: src/pidperfrace.o
	$(CC) -g -o $@ $< -lbsd

src/pidperfrace.o: src/pidperfrace.c
	$(CC) -g -o $@ -c $<
