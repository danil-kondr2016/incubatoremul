all: incubator_emul

incubator_emul: incubator_emul.c
	gcc incubator_emul.c -lpthread -o incubator_emul

clean:
	rm incubator_emul


