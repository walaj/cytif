CC = g++

TIFF = $(shell pkg-config --define-variable=prefix=/home/jaw34/software/libtiff-git --cflags vips-cpp libtiff-4)

EIGEN = -I/home/jaw34/software/eigen-3.4.0
KNN = -I/home/jaw34/git/knncolle/include
#VOR = -I/home/jaw34/git/voronoi/src
ARAND = -I//home/jaw34/git/aarand/include
UMAP =  -I/home/jaw34/git/umappp/include
ANNOY = -I/home/jaw34/git/annoy
HNSW = -I/home/jaw34/git/hnswlib
KMEANS = -I/home/jaw34/git/CppKmeans/include
IRLBA = -I/home/jaw34/git/CppIrlba/include

# old
##RCPP = $(shell Rscript -e "Rcpp:::CxxFlags()")
##RR = $(shell R CMD config --cppflags)
##PYTHON=$(shell pkg-config --cflags python3)

MATPLOTPLUS=-I/home/jaw34/software/matplotplusplus-1.1.0/include
MATPLOTPLUSLD=/home/jaw34/software/matplotplusplus-1.1.0/lib/libmatplot.a /home/jaw34/software/matplotplusplus-1.1.0/lib/Matplot++/libnodesoup.a

# this line needs to be fixed to make it portable
# this link depends on libffi, so need to do "conda install libffi"
PYTHONLD=-L/home/jaw34/mambaforge/envs/scimap/lib -lpython3 -lffi -lpcre

##RCPPLD = $(shell Rscript -e "Rcpp:::LdFlags()")
##RLD = $(shell R CMD config --ldflags)
#TIFFLD = $(shell pkg-config --define-variable=prefix=/home/jaw34/software/libtiff-git --libs libtiff-4)
TIFFLD = -llzma /home/jaw34/software/libtiff-git/lib/libtiff.a 
##VIPSLD = $(shell pkg-config --libs vips-cpp)

CFLAGS = -g -fopenmp -std=c++17 $(TIFF) $(UMAP) $(KNN) $(VOR) $(EIGEN) $(ARAND) $(IRLBA) $(HNSW) $(KMEANS) $(ANNOY) $(MATPLOTPLUS)
LDFLAGS = $(RCPPLD) $(TIFFLD) $(VIPSLD) $(MATPLOTPLUSLD) -ljpeg -lz -lpng

# Specify the source files
SRCS = tiffo.cpp tiff_header.cpp tiff_image.cpp tiff_cp.cpp cell.cpp plot.cpp tiff_reader.cpp tiff_ifd.cpp tiff_utils.cpp tiff_writer.cpp

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
