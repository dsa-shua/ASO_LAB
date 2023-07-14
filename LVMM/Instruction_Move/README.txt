==============================
A. Build and Execution Command
==============================

1. build compiler
	$ make

2. build example
	$ make -C ./2DCONV

3. run exmample
	$ cd ./2DCONV

	$ cp kernel.bin.0 kernel.bin
	$ ./2DConvolution.exe

==============================
B. Description
==============================

compiler.cpp / compiler : implemenation of instruction mover( move instruction )

	- usage : ./compiler <kernel_source_path> <output_file_path>
	- output : 
		- output_file_path.ll   : original IR
		- output_file_path.0    : move 0th load immediately after
		- output_file_path.0.ll : its human-readable IR
		- output_file_path.1    : move 0th load just before(because of dependency, error occurs)
		- output_file_path.1.ll : its human-readable IR(not error occurs)
