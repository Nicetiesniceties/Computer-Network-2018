TARGET=client
INCLUDE=-I../include
BIN=../bin
FLAG=-Wall -D debug -lpthread
GCC=gcc
CLIENT_OBJECT =  client.o client_main.o common.o 
SERVER_OBJECT =  server.o server_main.o common.o
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

client: $(CLIENT_OBJECT)
	@echo "making: " $@
	@$(GCC) -o $(BIN)/$@ $(FLAG) $(INCLUDE) $(CLIENT_OBJECT) # $(LIB)

server: $(SERVER_OBJECT)
	@echo "making: " $@
	@$(GCC) -o $(BIN)/$@ $(FLAG) $(INCLUDE) $(SERVER_OBJECT) # $(LIB)
