PREFIX ?= /usr/local
all: hello
hello: hello.c
	gcc -o hello hello.c
install: hello
	install -D -m 0755 hello $(DESTDIR)$(PREFIX)/bin/hello
clean:
	rm -f hello