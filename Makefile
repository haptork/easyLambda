# debug :run -n 4 xterm -e gdb ../bin/fname
CC := mpic++
BUILDDIR := build
SRCDIR := src
TESTDIR := test
EGDIR := examples
BUILDDIR := build
BINDIR := bin
TESTTARGET := bin/test
SRCEXT := cpp
CFLAGS := -Wall -std=c++14 -O3# -fno-omit-frame-pointer
# perf report -g 'graph,0.5,caller'
LIB := -lboost_mpi -lboost_serialization 
TESTS := $(shell find $(TESTDIR) -type f -name *.$(SRCEXT))
SOURCES := $(shell find $(EGDIR) -type f -name *.$(SRCEXT))
OBJECTS := $(patsubst $(EGDIR)/%,$(BINDIR)/%,$(SOURCES:.$(SRCEXT)=))
OBJECTSTEST := $(patsubst $(TESTDIR)/%,$(BUILDDIR)/%,$(TESTS:.$(SRCEXT)=.o))
TESTINC := -I test
INC := -I include

examples: $(OBJECTS)
	@mkdir -p $(BINDIR)
	@echo "Finished"

$(TESTTARGET): $(OBJECTSTEST)
	@mkdir -p $(BINDIR)
	@echo " Linking..."
	@echo " $(CC) $^ -o $(TESTTARGET) $(LIB)"; $(CC) $^ -o $(TESTTARGET) $(LIB)

$(BUILDDIR)/%.o: $(TESTDIR)/%.$(SRCEXT)
	@mkdir -p $(BUILDDIR)
	@echo " $(CC) $(CFLAGS) $(INC) $(TESTINC) -c -o $@ $<"; $(CC) $(CFLAGS) $(INC) $(TESTINC) -c -o $@ $<

example:
	@mkdir -p $(BINDIR)
	@echo " making $(fname)..."
	@echo " $(CC) examples/$(fname).cpp $(CFLAGS) $(INC) -o $(BINDIR)/$(fname) $(LIB) "; $(CC) examples/$(fname).cpp $(CFLAGS) $(INC) -o $(BINDIR)/$(fname) $(LIB)

$(BINDIR)/%: $(EGDIR)/%.$(SRCEXT)
	@mkdir -p $(BINDIR)
	@echo " $(CC) $(CFLAGS) $(INC) -o $@ $< $(LIB)"; $(CC) $(CFLAGS) $(INC) -o $@ $< $(LIB)

clean:
	@echo " Cleaning...";
	$(RM) -r $(BUILDDIR);
	$(RM) -r $(BINDIR);

.PHONY: clean
