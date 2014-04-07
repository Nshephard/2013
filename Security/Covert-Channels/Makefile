CC = gcc

PROGRAMS= covertReader covertWriter

all	:$(PROGRAMS)

%	: %.c
	@echo "Compiling $< .."
	$(CC) $< -o $@

clean	:
	/bin/rm $(PROGRAMS)

