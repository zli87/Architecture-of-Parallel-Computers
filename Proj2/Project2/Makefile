CC = g++
OPT = -O3
OPT = -g
WARN = -Wall
ERR = -Werror

CFLAGS = $(OPT) $(WARN) $(ERR) $(INC) $(LIB)

SIM_SRC = main.cc cache.cc

SIM_OBJ = main.o cache.o

all: clean smp_cache
	@echo "Compilation Done ---> nothing else to make :) "

smp_cache: $(SIM_OBJ)
	$(CC) -o smp_cache $(CFLAGS) $(SIM_OBJ) -lm
	@echo "----------------------------------------------------------"
	@echo "-----------FALL21-506-406 SMP SIMULATOR (SMP_CACHE)-----------"
	@echo "----------------------------------------------------------"
 
.cc.o:
	$(CC) $(CFLAGS)  -c $*.cc

clean:
	rm -f *.o smp_cache

clobber:
	rm -f *.o

msi: clean smp_cache
	./smp_cache 8192 8 64 4 0 ../trace/canneal.04t.debug

msiv1: clean smp_cache
	./smp_cache 8192 8 64 4 0 ../trace/canneal.04t.debug > ../out/MSI_debug.val
	diff -iw ../out/MSI_debug.val ../val.v2/MSI_debug.val
	
msi2: clean smp_cache
	./smp_cache 8192 8 64 4 0 ../trace/canneal.04t.longTrace

msiv2: clean smp_cache
	./smp_cache 8192 8 64 4 0 ../trace/canneal.04t.longTrace > ../out/MSI_long.val
	diff -iw ../out/MSI_long.val ../val.v2/MSI_long.val

mesi: clean smp_cache
	./smp_cache 8192 8 64 4 1 ../trace/canneal.04t.debug 
	
mesiv1: clean smp_cache
	./smp_cache 8192 8 64 4 1 ../trace/canneal.04t.debug > ../out/MESI_debug.val 
	diff -iw ../out/MESI_debug.val ../val.v2/MESI_debug.val

mesi2: clean smp_cache
	./smp_cache 8192 8 64 4 1 ../trace/canneal.04t.longTrace

mesiv2: clean smp_cache
	./smp_cache 8192 8 64 4 1 ../trace/canneal.04t.longTrace > ../out/MESI_long.val
	diff -iw ../out/MESI_long.val ../val.v2/MESI_long.val

dragon1: clean smp_cache
	./smp_cache 8192 8 64 4 2 ../trace/canneal.04t.debug

dragonv1: clean smp_cache
	./smp_cache 8192 8 64 4 2 ../trace/canneal.04t.debug > ../out/Dragon_debug.val
	diff -iw  ../val.v2/Dragon_debug.val ../out/Dragon_debug.val

dragonv2: clean smp_cache
	./smp_cache 8192 8 64 4 2 ../trace/canneal.04t.longTrace > ../out/Dragon_long.val
	diff -iw  ../val.v2/Dragon_long.val ../out/Dragon_long.val