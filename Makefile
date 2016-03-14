# debug :run -n 4 xterm -e gdb ../bin/fname
CC := mpic++
BUILDDIR := bin

CFLAGS := -Wall -std=c++14 -O3
LIB := -lboost_mpi -lboost_serialization 

INC := -I include

unittest:
	@echo " making tests..."
	@echo " $(CC) test/unitTest.cpp $(CFLAGS) -I test $(INC) -o $(BUILDDIR)/unitTest $(LIB)"; $(CC) test/unitTest.cpp $(CFLAGS) -I test $(INC) -o $(BUILDDIR)/unitTest $(LIB)

example:
	@echo " making $(fname)..."
	@echo " $(CC) examples/$(fname).cpp $(CFLAGS) $(INC) -o $(BUILDDIR)/$(fname) $(LIB)"; $(CC) examples/$(fname).cpp $(CFLAGS) $(INC) -o $(BUILDDIR)/$(fname) $(LIB)

clean:
	@echo " Cleaning...";
	@echo " $(RM) -r $(BUILDDIR) $(TARGET)"; $(RM) -r $(BUILDDIR) $(TARGET)

.PHONY: clean
