DIST_DIR = dist
SRC_DIR = ./src
IMG_DIR = ./img

SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(patsubst %.c, $(DIST_DIR)/%.o, $(notdir $(SRC))) $(DIST_DIR)/icon.o

TARGET = SkyCoL-VolRst.exe
BIN_TARGET = $(DIST_DIR)/$(TARGET)

CC = gcc
CC_PARAM = -Os -ffunction-sections -fdata-sections -Wl,--gc-sections -static -flto -s

$(BIN_TARGET):$(OBJ)
	$(CC) $(CC_PARAM) $(OBJ) -o $@ -luuid -lole32 -loleaut32 -lpsapi

$(DIST_DIR)/%.o:$(SRC_DIR)/%.c
	$(CC) $(CC_PARAM) -c $< -o $@

$(DIST_DIR)/icon.o: ./img/skycol-volrst-ico.ico $(IMG_DIR)/icon.rc
	windres -i $(IMG_DIR)/icon.rc -o $(DIST_DIR)/icon.o

clean:
	del .\dist\*.o
	del .\dist\SkyCoL-VolRst.exe

test: $(BIN_TARGET)
	$(DIST_DIR)\SkyCoL-VolRst.exe