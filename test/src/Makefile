TARGET=print_test
INCLUDE=-I../include
BIN=../bin
FLAG=-Wall
GCC=gcc
PRINT_OBJECT = main.o function.o
# LIB=-lm
.PHONY: clean
all: $(TARGET)

%.o: %.c
	@echo "making: " $@
	@$(GCC) $< -c $(FLAG) $(INCLUDE)
clean:
	rm -rf *.o
	@for bin in $(TARGET); do \
		echo "rm -rf $(BIN)/$$bin"; \
		rm -rf $(BIN)/$$bin; \
	done

print_test: $(PRINT_OBJECT)
	@echo "making: " $@
	@$(GCC) -o $(BIN)/$@ $(FLAG) $(INCLUDE) $(PRINT_OBJECT) # $(LIB)
