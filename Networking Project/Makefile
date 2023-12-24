# Compiler settings
CXX = g++
CXXFLAGS = -Wall -g -m64

# Directories
SERVER_DIR = Server
CLIENT_DIR = Client
OUTPUT_DIR = output

# Source and object files for server and client
SERVER_SRCS = $(wildcard $(SERVER_DIR)/*.cpp)
CLIENT_SRCS = $(wildcard $(CLIENT_DIR)/*.cpp)
SERVER_OBJS = $(SERVER_SRCS:.cpp=.o)
CLIENT_OBJS = $(CLIENT_SRCS:.cpp=.o)

# Executable names
SERVER_EXE = $(OUTPUT_DIR)/Server.exe
CLIENT_EXE = $(OUTPUT_DIR)/Client.exe

# SFML
SFML_INCLUDE = "C:/tools/sfml-mingw64/SFML-2.5.1/include"
SFML_LIB = "C:/tools/sfml-mingw64/SFML-2.5.1/lib"
SFML_LIBS = -lsfml-graphics-d -lsfml-window-d -lsfml-system-d
CXXFLAGS += -I$(SFML_INCLUDE)

# Linker
LDFLAGS = -L$(SFML_LIB) $(SFML_LIBS) -lws2_32

# Phony targets for building server and client
.PHONY: all server client clean

all: server client run_script

server: $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) -o $(SERVER_EXE) $^ $(LDFLAGS)

client: $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) -o $(CLIENT_EXE) $^ $(LDFLAGS)

run_script:
	cmd /C start_run.bat

# To clean up the compiled files
clean:
	if exist "$(SERVER_DIR)\*.o" del /Q "$(SERVER_DIR)\*.o"
	if exist "$(CLIENT_DIR)\*.o" del /Q "$(CLIENT_DIR)\*.o"
	if exist "$(OUTPUT_DIR)\*.exe" del /Q "$(OUTPUT_DIR)\*.exe"

# Suffix rule for compiling .cpp to .o
.cpp.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@