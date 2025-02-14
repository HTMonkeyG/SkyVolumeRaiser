DIST_DIR = dist
SRC_DIR = ./src
 
SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(patsubst %.c, $(DIST_DIR)/%.o, $(notdir $(SRC)))

TARGET = SkyCoL-VolRst.exe
BIN_TARGET = $(DIST_DIR)/$(TARGET)

CC = gcc

$(BIN_TARGET):$(OBJ)
	$(CC) $(OBJ) -o $@ -luuid -lole32 -loleaut32 -lpsapi

$(DIST_DIR)/%.o:$(SRC_DIR)/%.c
	$(CC) -c $< -o $@

clean:
	del .\dist\*.o
	del .\dist\SkyCoL-VolRst.exe

test: $(BIN_TARGET)
	$(DIST_DIR)\SkyCoL-VolRst.exe