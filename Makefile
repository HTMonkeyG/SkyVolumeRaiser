MAKEFLAGS += -s -j

# Directories.
DIST_DIR = ./dist
SRC_DIR = ./src

SRC_DIRS = $(SRC_DIR) $(wildcard $(SRC_DIR)/*/)

# Source files.
C_SRC = $(wildcard $(SRC_DIR)/*.c $(SRC_DIR)/*/*.c)
CPP_SRC = $(wildcard $(SRC_DIR)/*.cpp $(SRC_DIR)/*/*.cpp)
RES_SRC = ./res/res.rc

DLL_BIN = ./dll/dist/sky-volume-keep.dll
RESOURCES = $(RES_SRC) ./res/manifest.xml ./res/skycol-volrst-ico.ico

# Object files.
C_OBJ = $(addprefix $(DIST_DIR)/, $(notdir $(C_SRC:.c=.o)))
CPP_OBJ = $(addprefix $(DIST_DIR)/, $(notdir $(CPP_SRC:.cpp=.o)))
RES_OBJ = $(DIST_DIR)/res.o

# Header files.
CXX_HEADER = $(wildcard $(SRC_DIR)/*.h $(SRC_DIR)/*/*.h)

# Target name.
TARGET = sky-volume-keep.exe
BIN_TARGET = $(DIST_DIR)/$(TARGET)

# Compiler paths.
CC = gcc
CXX = g++

# Params.
CFLAGS = -O3 -ffunction-sections -fdata-sections -static -flto=auto -s -mwindows
CFLAGS += -Wall -Wformat
CFLAGS += -I./src

LFLAGS = -Wl,--gc-sections,-O3,--as-needed

vpath %.c $(SRC_DIRS)
vpath %.cpp $(SRC_DIRS)

.PHONY: all clean dll

all: $(DIST_DIR) $(BIN_TARGET)

$(BIN_TARGET): $(C_OBJ) $(CPP_OBJ) $(RES_OBJ)
	@echo Linking ...
	@$(CXX) $(CFLAGS) $^ -o $@ $(LFLAGS)
	@echo Done.

$(DIST_DIR)/%.o: %.c $(CXX_HEADER) $(DIST_DIR)
	@echo Compiling file "$<" ...
	@$(CC) $(CFLAGS) -c $< -o $@

$(DIST_DIR)/%.o: %.cpp $(CXX_HEADER) $(DIST_DIR)
	@echo Compiling file "$<" ...
	@$(CXX) $(CFLAGS) -c $< -o $@

$(RES_OBJ): $(RESOURCES) $(DIST_DIR) dll
	@echo Building resources...
	windres -i $(RES_SRC) -o $@

dll:
	@echo Building dll...
	$(MAKE) -C ./dll all

$(DIST_DIR):
	-@mkdir dist

clean:
	-@del .\dist\*.o
	-@del .\dist\*.exe
	-@$(MAKE) -C ./dll clean
