#
# TODO: Move `libmongoclient.a` to /usr/local/lib so this can work on production servers
#
 
CC := gcc # This is the main compiler
SRCDIR := src/util
MAINSRCDIR := src
BUILDDIR := build
TARGET1 := bin/worker
TARGET2 := bin/proxy
TARGET3 := bin/client
 
SRCEXT := c
SOURCES := $(shell find $(SRCDIR) -type f -name '*.$(SRCEXT)')
OBJECTS := $(patsubst $(SRCDIR)/%.c,$(BUILDDIR)/%.o,$(SOURCES))
CFLAGS := -Wall -std=c99 -D_POSIX_C_SOURCE
LIB := -lpthread -lssl -lcrypto
NOLIB :=  
INC := -I include

$(TARGET2): $(BUILDDIR)/proxy.o $(OBJECTS)
	@echo " Linking..."
	@echo " $(CC) $^ -o $(TARGET2) $(NOLIB)"; $(CC) $^ -o $(TARGET2) $(LIB)

$(TARGET1): $(BUILDDIR)/worker.o $(OBJECTS)
	@echo " Linking..."
	@echo " $(CC) $^ -o $(TARGET1) $(LIB)"; $(CC) $^ -o $(TARGET1) $(LIB)

$(TARGET3): $(BUILDDIR)/client.o $(OBJECTS)
	@echo " Linking..."
	@echo " $(CC) $^ -o $(TARGET3) $(LIB)"; $(CC) $^ -o $(TARGET3) $(LIB)

$(BUILDDIR)/%.o: $(SRCDIR)/%.$(SRCEXT)
	@mkdir -p $(BUILDDIR)
	@echo " $(CC) $(CFLAGS) $(INC) -c -o $@ $<"; $(CC) $(CFLAGS) $(INC) -c -o $@ $<

$(BUILDDIR)/%.o: $(MAINSRCDIR)/%.$(SRCEXT)
	@mkdir -p $(BUILDDIR)
	@echo " $(CC) $(CFLAGS) $(INC) -c -o $@ $<"; $(CC) $(CFLAGS) $(INC) -c -o $@ $<

clean:
	@echo " Cleaning..."; 
	@echo " $(RM) -r $(BUILDDIR) $(TARGET)"; $(RM) -r $(BUILDDIR) $(TARGET)

# Tests
tester:
	$(CC) $(CFLAGS) test/tester.c $(INC) $(LIB) -o bin/tester

# Spikes
ticket:
	$(CC) $(CFLAGS) spikes/ticket.c $(INC) $(LIB) -o bin/ticket

.PHONY: clean
