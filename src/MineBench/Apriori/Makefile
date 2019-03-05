CC  = icc -openmp -Qtm_enabled -g
CFLAGS  = -fbuiltin -O0 -I /usr/include/c++
OBJS	= ListItemset.o
OBJS2   = rdtsc.o Database.o Itemset.o HashTree.o
OPTS	= -DBALT 
LIBS =  -lm
LD = ld

TARGET  = no_output_apriori

default:	$(TARGET)

clean:
	rm -rf *~ *.o $(TARGET)

no_output_apriori: no_output_parapr.cpp $(OBJS) $(OBJS2) 
	$(CC) $(CFLAGS) $(OPTS)\
	-o no_output_apriori no_output_parapr.cpp $(OBJS) $(OBJS2) $(LIBS)

rdtsc.o: rdtsc.cpp
	$(CC) $(CFLAGS) -c rdtsc.cpp

Database.o: Database.cpp Database.h
	$(CC) $(CFLAGS) $(OPTS) -c Database.cpp

Itemset.o: Itemset.cpp Itemset.h 
	$(CC) $(CFLAGS) $(OPTS) -c Itemset.cpp

ListItemset.o: ListItemset.cpp ListItemset.h
	$(CC) $(CFLAGS) -c ListItemset.cpp

HashTree.o: HashTree.cpp HashTree.h
	$(CC) $(CFLAGS) $(OPTS) -c HashTree.cpp

dec_shlib.o: dec_shlib.C dec_shlib.h
	$(CC) $(CFLAGS) $(OPTS) -c dec_shlib.C 

dec_llsc.o: dec_llsc.s llsc.h
	cc  -c dec_llsc.s 

