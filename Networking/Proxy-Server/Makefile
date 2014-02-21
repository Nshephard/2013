CC	= cc

PROGRAMS= proxy

all	: $(PROGRAMS)

%	: %.c
	@echo "Compiling $< .."
	$(CC) $< -o $@


clean	:
	/bin/rm $(PROGRAMS)

