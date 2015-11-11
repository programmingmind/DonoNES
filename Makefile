CXX = clang++
SDL = -framework SDL2 -framework SDL2_image
# If your compiler is a bit older you may need to change -std=c++11 to -std=c++0x
CXXFLAGS = -g -D_DEBUG -Wall -c -std=c++11 -I include -I Frameworks/SDL2.framework/Headers -I Frameworks/SDL2_image.framework/Headers
LDFLAGS = $(SDL) -F Frameworks/ -Xlinker -rpath -Xlinker ../Frameworks/

SRCDIR = src
SOURCEFILES := cpu.c memory.c DonoNES.c
SOURCES := $(addprefix $(SRCDIR)/, $(SOURCEFILES))
OBJECTS := $(addprefix obj/, $(SOURCEFILES:.c=.o))

DonoNES: $(OBJECTS)
	$(CXX) $^ -o $@

SDL: $(OBJECTS)
	$(CXX) $(LDFLAGS) $^ -o $@

obj/%.o: $(SRCDIR)/%.c $(SRCDIR)/%.h Makefile
	$(CXX) $(CXXFLAGS) $< -o $@

obj/%.o: $(SRCDIR)/%.c Makefile
	$(CXX) $(CXXFLAGS) $< -o $@

clean:
	rm -f obj/*.o DonoNES
