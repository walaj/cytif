CC = /opt/homebrew/Cellar/gcc/12.2.0/bin/g++-12

#TIFF = $(shell pkg-config --define-variable=prefix=/home/jaw34/software/libtiff-git --cflags libtiff-4)
TIFF = -I$(HOME)/software/libtiff-git/include

EIGEN = -I$(HOME)/software/eigen-3.4.0
KNN = -I$(HOME)/git/knncolle/include
#VOR = -I$(HOME)/git/voronoi/src
ARAND = -I/$(HOME)/git/aarand/include
UMAP =  -I$(HOME)/git/umappp/include
ANNOY = -I$(HOME)/git/annoy
HNSW = -I$(HOME)/git/hnswlib
KMEANS = -I$(HOME)/git/CppKmeans/include
IRLBA = -I$(HOME)/git/CppIrlba/include
JPEG = -L/opt/homebrew/opt/jpeg/lib -L/opt/homebrew/Cellar/libpng/1.6.39/lib

# old
##RCPP = $(shell Rscript -e "Rcpp:::CxxFlags()")
##RR = $(shell R CMD config --cppflags)
##PYTHON=$(shell pkg-config --cflags python3)

#MATPLOTPLUS=-I$(HOME)/software/matplotplusplus-1.1.0/include
#MATPLOTPLUSLD=$(HOME)/software/matplotplusplus-1.1.0/lib/libmatplot.a $(HOME)/software/matplotplusplus-1.1.0/lib/Matplot++/libnodesoup.a

# this line needs to be fixed to make it portable
# this link depends on libffi, so need to do "conda install libffi"
#PYTHONLD=-L$(HOME)/mambaforge/envs/scimap/lib -lpython3 -lffi -lpcre

##RCPPLD = $(shell Rscript -e "Rcpp:::LdFlags()")
##RLD = $(shell R CMD config --ldflags)
#TIFFLD = $(shell pkg-config --define-variable=prefix=$(HOME)/software/libtiff-git --libs libtiff-4)
TIFFLD = -llzma $(HOME)/software/libtiff-git/lib/libtiff.a
##VIPSLD = $(shell pkg-config --libs vips-cpp)

CFLAGS = -g -fopenmp -std=c++17 -I.. $(TIFF) $(UMAP) $(KNN) $(VOR) $(EIGEN) $(ARAND) $(IRLBA) $(HNSW) $(KMEANS) $(ANNOY) $(MATPLOTPLUS)
LDFLAGS = $(JPEG) $(RCPPLD) $(TIFFLD) $(VIPSLD) $(MATPLOTPLUSLD) -ljpeg -lz -lpng

# Specify the source files
SRCS = tiffo.cpp tiff_header.cpp tiff_image.cpp tiff_cp.cpp cell2.cpp tiff_reader.cpp tiff_ifd.cpp tiff_utils.cpp tiff_writer.cpp jsoncpp.cpp

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
