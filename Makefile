DIST_DIR = dist
SRC_DIR = ./src
RES_DIR = ./res

SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(patsubst %.c, $(DIST_DIR)/%.o, $(notdir $(SRC))) $(DIST_DIR)/res.o

TARGET = skyvol-rst.exe
BIN_TARGET = $(DIST_DIR)/$(TARGET)

CC = gcc
CC_PARAM = -Wall -Wno-implicit-function-declaration -Os -ffunction-sections -fdata-sections -Wl,--gc-sections -static -flto -s
LINK_PARAM = -luuid -lole32 -loleaut32 -lpsapi

$(BIN_TARGET):$(OBJ)
	$(CC) $(CC_PARAM) $(OBJ) -o $@ $(LINK_PARAM)

$(DIST_DIR)/%.o:$(SRC_DIR)/%.c
	$(CC) $(CC_PARAM) -c $< -o $@

$(DIST_DIR)/res.o: $(RES_DIR)/manifest.xml $(RES_DIR)/skycol-volrst-ico.ico
	windres -i $(RES_DIR)/res.rc -o $(DIST_DIR)/res.o

clean:
	del .\dist\*.o
	del .\dist\*.exe
