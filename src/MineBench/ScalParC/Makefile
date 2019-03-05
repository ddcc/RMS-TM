#CC = icc -openmp -g
CC = icc -openmp -Qtm_enabled -g
# CFLAGS = -O2 $(INCLUDE_DIR) 
CFLAGS  = -fbuiltin -O0 -I /usr/include/c++
LINKFLAGS = 

INCLUDE_DIR = -I. 

LIBS = -lm
TARGET = scalparc
OBJ  = rdtsc.o drive.o getdata.o parsort.o parclass.o

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) -o $@ $(LINKFLAGS) $(OBJ) $(LIBS)


.c.o: 
	$(CC) -c $(CFLAGS) $*.c

clean:
	rm -f $(TARGET) $(OBJ)
