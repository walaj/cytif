#CC = /opt/homebrew/Cellar/gcc/13.1.0/bin/g++-13 

HMSHOME=/home/jaw34
BREWHOME=/opt/homebrew/opt

## check compiler type. If clang then add -Xpreprocessor flag                                                                                                    
COMPILER_TYPE := $(shell $(CC) --version)                                                                                                                        
ifneq (,$(findstring clang,$(COMPILER_TYPE)))                                                                                                                    
    OPENMP = -Xpreprocessor -fopenmp
    LSTD = -lc++
    LIBTIFFHOME=/opt/homebrew/Cellar/libtiff/4.7.0
else
    OPENMP = -fopenmp
    LSTD =
    LIBTIFFHOME=/home/jaw34/software/libtiff-4.4.0
    CC=g++
endif   

### HMS ###
ifeq ($(wildcard $(BREWHOME)),)  # If dir does not exist, probably on HMS server
    OMP = -I$(HMSHOME)/software/openmpi-4.1.5/include -DHAVE_OMP
    OMPL= -L$(HMSHOME)/software/openmpi-4.1.5/lib -lmpi -Wl,-rpath=$(HMSHOME)/software/openmpi-4.1.5/lib/
#### MAC ###
else
    ## Optional libaries for OMP multithreading support
    OMP_INCDIR=$(BREWHOME)/libomp/include
    ifneq ($(wildcard $(OMP_INCDIR)/*),) 
        OMP = -I$(OMP_INCDIR) -DHAVE_OMP
        OMPL= -L$(BREWHOME)/libomp/lib -lomp
    else
        $(warning ******************************************************************)
        $(warning Warning: Directory $(OMP_INCDIR) does not exist or is empty. OMP will not be included)
        $(warning To fix, please include OMP libraries for multithreading support)
        $(warning ******************************************************************)
    endif
endif

#TIFF = $(shell pkg-config --define-variable=prefix=/home/jaw34/software/libtiff-git --cflags libtiff-4)
#TIFF = -I$(HOME)/git/libtiff/libtiff
#TIFF = -I$(HOME)/git/libtiff/includesoftware/libtiff-git/include
#TIFF=-I$(HOME)/software/libtiff-4.6.0/include

TIFF=-I$(LIBTIFFHOME)/include

#JPEG = -L/opt/homebrew/opt/jpeg/lib -L/opt/homebrew/Cellar/libpng/1.6.39/lib

# old
##RCPP = $(shell Rscript -e "Rcpp:::CxxFlags()")
##RR = $(shell R CMD config --cppflags)
##PYTHON=$(shell pkg-config --cflags python3)

#TIFFLD = $(shell pkg-config --define-variable=prefix=$(HOME)/software/libtiff-git --libs libtiff-4)
#TIFFLD = -llzma $(HOME)/software/libtiff-git/lib/libtiff.a
#TIFFLD = -llzma $(HOME)/git/libtiff/libtiff/libtiff.la
TIFFLD=-llzma -L$(LIBTIFFHOME)/lib -ltiff

CFLAGS = -g -std=c++17 -I.. $(OPENMP) $(TIFF) $(OMP)
LDFLAGS = $(OMPL) $(TIFFLD) $(JPEG) -lz $(LSTD)

# Specify the source files
SRCS = cytif.cpp tiff_header.cpp tiff_image.cpp tiff_cp.cpp tiff_reader.cpp tiff_ifd.cpp tiff_utils.cpp tiff_writer.cpp channel.cpp

# Specify the object files
OBJS = $(SRCS:.cpp=.o)

# Specify the executable file
EXEC = cytif

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
