CC = g++
CFLAGS = -std=c++11 `pkg-config --cflags vips-cpp libtiff-4`
LDFLAGS = `pkg-config --libs vips-cpp libtiff-4`

# Specify the source files
SRCS = small.cpp tiff_header.cpp tiff_image.cpp tiff_cp.cpp

# Specify the object files
OBJS = $(SRCS:.cpp=.o)

# Specify the executable file
EXEC = small

# The default target
all: $(EXEC)

# Build the executable from the object files
$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile the source files into object files
%.o: %.cpp
	$(CC) $(CFLAGS) -c $<

#wips: wips.cpp
#	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# Clean up
clean:
	rm -f $(OBJS) $(EXEC)

.PHONY: all clean
