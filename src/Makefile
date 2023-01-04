CC = g++
CFLAGS = -g -std=c++17 `pkg-config --cflags vips-cpp libtiff-4` -I/home/jaw34/software/boost-1.81.0/include -I/home/jaw34/git/voronoi/src
LDFLAGS = `pkg-config --libs vips-cpp libtiff-4`

# Specify the source files
SRCS = tiffo.cpp tiff_header.cpp tiff_image.cpp tiff_cp.cpp cell.cpp 

# Specify the object files
OBJS = $(SRCS:.cpp=.o)

# Specify the executable file
EXEC = tiffo

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
