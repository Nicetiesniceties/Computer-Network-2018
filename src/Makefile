TARGET=client
INCLUDE=-I../include
BIN=../bin
FLAG=-Wall -D debug
GCC=gcc
PRINT_OBJECT = client.o client_main.o
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

client: $(PRINT_OBJECT)
	@echo "making: " $@
	@$(GCC) -o $(BIN)/$@ $(FLAG) $(INCLUDE) $(PRINT_OBJECT) # $(LIB)
