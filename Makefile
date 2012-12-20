ifeq ($(PREFIX),)
PREFIX=/usr/local
endif

LIB_OPTIM=-O2
TEST_OPTIM=

libCIieeefp.a: CIieeefp.o x87FPUcmds.o x87FPUutil.o
	ar ruv libCIieeefp.a CIieeefp.o x87FPUcmds.o x87FPUutil.o
	ranlib libCIieeefp.a

CIieeefp.o: CIieeefp.h CIieeefp.c CIieeefp-sys.h x87FPUutil.h x87FPUcmds.h x87FPUusys.h x87FPUsys.h
	gcc $(LIB_OPTIM) -I. -fPIC -c -o CIieeefp.o CIieeefp.c

x87FPUcmds.o: x87FPUcmds.h x87FPUcmds.c x87FPUsys.h
	gcc $(LIB_OPTIM) -fPIC -c -o x87FPUcmds.o x87FPUcmds.c

x87FPUutil.o: x87FPUutil.h x87FPUutil.c x87FPUusys.h x87FPUcmds.h x87FPUsys.h
	gcc $(LIB_OPTIM) -fPIC -c -o x87FPUutil.o x87FPUutil.c

comparison: test-CIieeefp test-CIieeefp.sun
	./test-CIieeefp -cmp test-CIieeefp.sun

test-CIieeefp.sun:
	$(error Cannot find test-CIieeefp.sun -- this file should have been supplied in the original tarball.)

test: test-CIieeefp
	@./test-CIieeefp -test && echo "*** Test completed successfully ***"

test-CIieeefp: test-CIieeefp.c libCIieeefp.a
	gcc $(TEST_OPTIM) -I. -L. -o test-CIieeefp test-CIieeefp.c -lCIieeefp

install: libCIieeefp.a
	@test -d $(PREFIX) || mkdir -p $(PREFIX) || echo "Problem making directory $PREFIX, try: env PREFIX=//c/$(PREFIX) make install"
	test -d $(PREFIX)/include || mkdir $(PREFIX)/include
	test -d $(PREFIX)/lib || mkdir $(PREFIX)/lib
	cp CIieeefp.h $(PREFIX)/include
	cp CIieeefp-sys.h $(PREFIX)/include
	cp libCIieeefp.a $(PREFIX)/lib

clean:
	-/bin/rm -f *.o *.a *.exe test-CIieeefp test-CIieeefp.out
