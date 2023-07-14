CXX := g++
CXXFLAG := -I /usr/local/cuda/include -L /usr/local/cuda/lib64

.PHONY : all clean binary_execution original_execution

all: binary_execution

binary_execution : 2DConvolution.c kernel.bin
	g++ 2DConvolution.c -o 2DConvolution.exe  $(CXXFLAG) -lOpenCL -D BINARY -D PLATFORM_IDX=1 -D DEVICE_IDX=0 -D BINARY_EXECUTION

original_execution : 2DConvolution.c
	g++ 2DConvolution.c -o 2DConvolution.exe  $(CXXFLAG) -lOpenCL -D BINARY -D PLATFORM_IDX=1 -D DEVICE_IDX=0 -D ORIGINAL_EXECUTION

kernel.bin : ../compiler 2DConvolution.cl
	../compiler 2DConvolution.cl kernel.bin

../compiler :
	make -C ../

clean:
	rm -vf 2DConvolution.exe *.ll *.bc *.nvptx.s *.bin*
	
