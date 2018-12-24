src = $(wildcard *.c)

main: $(src)
	gcc -o $@ -L/usr/lib/ -I/usr/include/ -lncurses $^

run: main
	./main