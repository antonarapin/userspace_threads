all:
	gcc counter.c uthread.c -o counter -Wno-deprecated -Wno-pointer-to-int-cast
	gcc thread-hello.c uthread.c -o hello -Wno-deprecated -Wno-pointer-to-int-cast
	gcc demo.c -o demo -Wno-deprecated

debug:
	gcc -g counter.c uthread.c -o counter -Wno-deprecated -Wno-pointer-to-int-cast
	gcc -g thread-hello.c uthread.c -o hello -Wno-deprecated -Wno-pointer-to-int-cast
	gcc -g demo.c -o demo -Wno-deprecated
