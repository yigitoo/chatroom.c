# Makefile for the project
APP_NAME := poll

SRC_DIR := src
BIN_DIR := build
LIB_DIR := lib
INC_DIR := include
BUILD_FILE := main.c

ifeq ($(OS), Windows_NT)
EXE_FILE := $(BIN_DIR)/$(APP_NAME).exe 
CLEAN_CMD := rd /s $(BIN_DIR)
else
EXE_FILE :=  $(BIN_DIR)/$(APP_NAME)
CLEAN_CMD := rm -rf $(BIN_DIR)
endif

CC = gcc

all: 
	make build -s
ifeq ($(OS), Windows_NT)
	cls
else
	clear
endif
	make run -s

build: $(SRC_DIR)/$(BUILD_FILE)
	mkdir build
	$(CC) $(CUSTOM_FLAGS) $(CFLAGS) -o $(EXE_FILE) $<

run: 
	$(EXE_FILE)
clean:
	$(CLEAN_CMD)
PHONY: all