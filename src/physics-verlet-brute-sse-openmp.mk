OMPFLAGS = -fopenmp
CFLAGS += -DVECTOR_SIZE=2 -msse $(OMPFLAGS)
LDFLAGS += $(OMPFLAGS)