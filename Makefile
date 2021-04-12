OBJS = src/editor.cpp
CC = x86_64-w64-mingw32-g++
CFLAGS = -std=c++17

default:

all: $(OBJS)
	$(CC) $(OBJS)