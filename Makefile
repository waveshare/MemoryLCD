DIR_SRC = ./src
DIR_OBJ = ./obj

OBJ_C = $(wildcard ${DIR_SRC}/*.c)
OBJ_O = $(patsubst %.c,${DIR_OBJ}/%.o,$(notdir ${OBJ_C}))

TARGET = mem_lcd

CC = gcc

DEBUG = -g -O0 -Wall
CFLAGS += $(DEBUG)

LIB = -l bcm2835 -l pthread

${TARGET}:${OBJ_O}
	$(CC) $(CFLAGS) $(OBJ_O) -o $@ $(LIB)

${DIR_OBJ}/%.o : $(DIR_SRC)/%.c
	$(CC) $(CFLAGS) -c  $< -o $@ $(LIB)
	
clean :
	rm $(DIR_OBJ)/*.* 
	rm $(TARGET) 