CC  = icc -openmp -Qtm_enabled -g
CFLAGS  = -fbuiltin -O0 -I /usr/include/c++
OBJS	= ListItemset.o
OBJS2   = rdtsc.o TimeCalculation.o Database.o Itemset.o HashTree.o
OPTS	= -DBALT -DOPTIMAL 
LIBS = -lm 

TARGET  = utility_mine 

default:	$(TARGET)

clean:
	rm -rf *~ *.o $(TARGET)

utility_mine: utility.cpp $(OBJS) $(OBJS2) 
	$(CC) $(CFLAGS) $(OPTS)\
	-o utility_mine utility.cpp $(OBJS) $(OBJS2) $(LIBS)

# rdtsc.o: rdtsc.cpp
# 	$(CC) $(CFLAGS) $(OPTS) -c rdtsc.cpp
# 
# TimeCalculation.o: TimeCalculation.cpp
# 	$(CC) $(CFLAGS) $(OPTS) -c TimeCalculation.cpp

Database.o: Database.cpp Database.h
	$(CC) $(CFLAGS) $(OPTS) -c Database.cpp

Itemset.o: Itemset.cpp Itemset.h 
	$(CC) $(CFLAGS) $(OPTS) -c Itemset.cpp

ListItemset.o: ListItemset.cpp ListItemset.h
	$(CC) $(CFLAGS) -c ListItemset.cpp

HashTree.o: HashTree.cpp HashTree.h
	$(CC) $(CFLAGS) $(OPTS) -c HashTree.cpp

