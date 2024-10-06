TARGET = gen
LIBNAME = Dyn
LIBFILE = lib$(LIBNAME).so

CPPFLAGS  += -g 
LDFLAGS += -L. 
LDLIBS += -l$(LIBNAME)

LIBINTDIR = DynInt
LIBFLOATDIR = DynFloat


ifeq ($(shell uname -m),x86_64) 
 CPPFLAGS += -fPIC
endif

all: $(TARGET)

clean:
	rm -rf $(TARGET) $(LIBFILE) *.o

$(LIBINTDIR)/$(LIBFILE): $(LIBNAME).o
	g++ $(CPPFLAGS) $^ -shared $(LDFLAGS) -o $@

$(LIBFLOATDIR)/$(LIBFILE): $(LIBNAME).o
	g++ $(CPPFLAGS) $^ -shared $(LDFLAGS) -o $@

$(TARGET): $(TARGET).cpp $(LIBFILE)
	g++ $(CFLAGS) $< $(LDFLAGS) $(LDLIBS) -o $@
