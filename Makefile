default:
	gcc -O2 -o 2in1screen 2in1screen.c
install:
	install -m 755 2in1screen /usr/local/bin/2in1screen
uninstall:
	rm -fv /usr/local/bin/2in1screen
