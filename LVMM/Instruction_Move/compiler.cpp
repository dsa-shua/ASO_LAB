#include <iostream>
#include <fstream>
#include <string>
#include <vector>

#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/DiagnosticPrinter.h"
#include "llvm/IR/DiagnosticHandler.h"
#include "llvm/Bitcode/BitcodeReader.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/Transforms/Utils/ValueMapper.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/raw_os_ostream.h"

/////////////////////////
// Macros
////////////////////////
#define QUOTE( X ) #X
#define STRING_OF_VALUE( X ) QUOTE( X )

#ifdef LLVM_PATH
#define LLVM_PATH_STRING std::string( STRING_OF_VALUE( LLVM_PATH ) )
#else
#define LLVM_PATH_STRING "/usr/local/llvm"
#endif 

#ifdef LIBCLC_PATH
#define LIBCLC_PATH_STRING std::string( STRING_OF_VALUE( LIBCLC_PATH ) )
#else
#define LIBCLC_PATH_STRING "/usr/local/libclc"
#endif


////////////////////////
// Global Variables
///////////////////////
llvm::LLVMContext* TheContext;
std::unique_ptr< llvm::Module > TheModule;
llvm::IRBuilder<>* TheBuilder;
std::string kernel_source_path;
std::string output_file_path;
std::string working_file_path;
std::vector< std::string > temporary_files;
llvm::raw_os_ostream raw_cout(std::cout);

///////////////////////
// Functions
///////////////////////
void print_usage( void );
void parse_argument( int argc, char* argv[] );
void compile_source_code( void );
void get_module( void );
void module_writing( void );
void module_writing( std::string output_bitcode_path );
void module_writing_to_readable_bitcode( std::string output_bitcode_path );
void backend_process( void );
void remove_temporary_files( void );

int get_num_load( void );
#define DIRECTION_BEFORE 0
#define DIRECTION_AFTER 1
void move_load( int load_index, int direction );

/////////////////
// MAIN Functions
/////////////////
int main( int argc, char* argv[] )
{
	parse_argument( argc, argv );
	compile_source_code();
	get_module();

	std::string original_output_file_path = output_file_path;
	// Print Original IR
	module_writing_to_readable_bitcode( output_file_path + ".ll" );

	// Move 0th load forward(DIRECTION_AFTER)
	output_file_path = original_output_file_path + "." + std::to_string( 0 );
	move_load( 0, DIRECTION_AFTER );
	module_writing();
	module_writing_to_readable_bitcode( output_file_path + ".ll" );
	backend_process();

	// Move 1th load before(DIRECTION_BEFORE)
	output_file_path = original_output_file_path + "." + std::to_string( 1 );
	move_load( 1, DIRECTION_BEFORE );
	module_writing();
	module_writing_to_readable_bitcode( output_file_path + ".ll" );
	backend_process();

	//clean
	remove_temporary_files( );

	return 0;
}

///////////////////////////
// Function Impementations
///////////////////////////

//print usage
void print_usage( void )
{
	std::cout << "approximator kernel_source_path output_file_path" << std::endl;
}

//initialize global variables with command arguments
void parse_argument( int argc, char* argv[] )
{
	if( argc != 3 )
	{
		std::cerr <<"["<<__FILE__<<":"<<__LINE__<<"] "<< "# of Argument is not 3" << std::endl;
		print_usage();
		exit(-1);
	}

	temporary_files.clear();
	TheModule.reset();
	TheBuilder = nullptr;
	TheContext = nullptr;
	kernel_source_path = std::string( argv[1] );
	output_file_path = std::string( argv[2] );
	working_file_path = kernel_source_path;
}

//compile source code(opencl kernel) to llvm IR
void compile_source_code( void )
{
	std::string temp_bitcode_path = "__kernel.temp.bc";
	std::string compile_command = 
		LLVM_PATH_STRING + "/bin/clang " 
		+ "-emit-llvm -c "
		+ "-target -nvptx64-nvidia-nvcl "
		+ "-Dcl_clang_storage_class_specifiers "
		+ "-include " + LIBCLC_PATH_STRING + "/include/clc/clc.h "
		+ "-I " + LIBCLC_PATH_STRING + "/include/ "
		+ "-fpack-struct=64 -O3 "
		+ "-o " + temp_bitcode_path + " "
		+ working_file_path;

	system( compile_command.c_str() );
	working_file_path = temp_bitcode_path;
	temporary_files.push_back( temp_bitcode_path );
}

//load module from bitcode file
/*
void get_module( void )
{
	//read bitcode file
	std::string kernel_bitcode_contents;
	std::ifstream kernel_bitcode_ifstream( working_file_path );
	if( kernel_bitcode_ifstream.fail() )
	{
		std::cerr <<"["<<__FILE__<<":"<<__LINE__<<"] "<< "File Read Failed : " << working_file_path << std::endl;
		exit( -1 );
	}
	kernel_bitcode_contents.assign(
			(std::istreambuf_iterator< char >( kernel_bitcode_ifstream ) ),
			(std::istreambuf_iterator< char >( )) 
			);
	kernel_bitcode_ifstream.close();

	//theContext allocation
	TheContext = new llvm::LLVMContext();

	//parse bitcode file
	llvm::MemoryBufferRef kernel_bitcode_contents_mbref( llvm::StringRef( kernel_bitcode_contents ), "kernel_membuf" );
	auto expected_unq_ptr_module = llvm::parseBitcodeFile( kernel_bitcode_contents_mbref, *TheContext );

	if( !expected_unq_ptr_module )
	{
		std::cerr <<"["<<__FILE__<<":"<<__LINE__<<"] "<< "Parse Bitcode File Failed" << std::endl;
		exit( -1 );
	}

	TheModule = std::move(expected_unq_ptr_module.get());
}
*/

void get_module( void )
{
	//Create LLVMContext
	TheContext = new llvm::LLVMContext();

	if( ! TheContext )
	{
		std::cerr <<"["<<__FILE__<<":"<<__LINE__<<"] " << "LLVMContext Allocation Failed" << std::endl;
		exit( -1 );
	}


	//Load llvm::Module from bitcode file
	llvm::SMDiagnostic Err;
	TheModule = llvm::parseIRFile( working_file_path, Err, *TheContext );
	if( ! TheModule )
	{
		std::cerr <<"["<<__FILE__<<":"<<__LINE__<<"] " << "Parse IR File Failed : " << working_file_path  << std::endl;
		exit( -1 );
	}
}



//print loaded module to bitcode file
void module_writing( void )
{
	//print loaded( modified ) module to std::string
	std::string bitcode_string;
	llvm::raw_string_ostream bitcode_string_ostream( bitcode_string );
	llvm::WriteBitcodeToFile( TheModule.get(), bitcode_string_ostream );
	bitcode_string_ostream.flush();


	//print out bitcode string
	std::string output_bitcode_path = "__kernel.temp.processed.bc";
	std::ofstream output_file_ofstream( output_bitcode_path );
	if( !output_file_ofstream.good() )
	{
		std::cerr <<"["<<__FILE__<<":"<<__LINE__<<"] "<< "File Open Failed" << std::endl;
		exit( -1 );
	}

	output_file_ofstream << bitcode_string;
	output_file_ofstream.close();

	temporary_files.push_back( output_bitcode_path );
	working_file_path = output_bitcode_path;
}

//print llvm module to bitcode file
void module_writing( std::string output_bitcode_path )
{
	//print llvm module to std::string
	std::string bitcode_string;
	llvm::raw_string_ostream bitcode_string_ostream( bitcode_string );
	llvm::WriteBitcodeToFile( TheModule.get(), bitcode_string_ostream );
	bitcode_string_ostream.flush();

	//print out bitcode string
	std::ofstream output_file_ofstream( output_bitcode_path );
	if( !output_file_ofstream.good() )
	{
		std::cerr <<"["<<__FILE__<<":"<<__LINE__<<"] "<< "File Open Failed" << std::endl;
		exit( -1 );
	}

	output_file_ofstream << bitcode_string;
	output_file_ofstream.close();
}

//print llvm module to bitcode file
void module_writing_to_readable_bitcode( std::string output_bitcode_path )
{
	//print llvm module to std::string
	std::string bitcode_string;
	llvm::raw_string_ostream bitcode_string_ostream( bitcode_string );
	TheModule->print( bitcode_string_ostream, NULL );
	bitcode_string_ostream.flush();

	//print out bitcode string
	std::ofstream output_file_ofstream( output_bitcode_path );
	if( !output_file_ofstream.good() )
	{
		std::cerr <<"["<<__FILE__<<":"<<__LINE__<<"] "<< "File Open Failed" << std::endl;
		exit( -1 );
	}

	output_file_ofstream << bitcode_string;
	output_file_ofstream.close();
}

//backend process of bitcode file
void backend_process( void )
{
	//IR-level link with libclc libs
	std::string linked_bitcode_path = "__kernel.linked.bc";
	std::string ir_link_command = 
		LLVM_PATH_STRING + "/bin/llvm-link " 
		+ "-o " + linked_bitcode_path + " "
		+ working_file_path + " "
		+ LIBCLC_PATH_STRING + "/lib/clc/nvptx64--nvidiacl.bc";

	system( ir_link_command.c_str() );

	temporary_files.push_back( linked_bitcode_path );
	working_file_path = linked_bitcode_path;

	//llvm static compile
	std::string static_compile_command =
		LLVM_PATH_STRING + "/bin/llc "
		+ "-mcpu=sm_20 -march=nvptx64 "
		+ "-o " + output_file_path + " "
		+ working_file_path;

	system( static_compile_command.c_str() );
	working_file_path = output_file_path;
}

//remove temporary files
void remove_temporary_files( void )
{
	for( int i=0, Size=temporary_files.size(); i<Size; ++i )
	{
		std::string temporary_file_path = temporary_files[i];
		remove( temporary_file_path.c_str() );
	}
}


//get number of load instructions in module
int get_num_load( void )
{
	int number_of_load = 0;

	//a module has list of function
	for( llvm::Module::iterator ModIter = TheModule->begin();
			ModIter != TheModule->end();
			++ModIter )
	{
		//llvm::Module::iteartor == llvm::Function*
		llvm::Function *Func = llvm::cast< llvm::Function >( ModIter );
		//a function has list of basicblocks
		for( llvm::Function::iterator FuncIter = Func->begin();
				FuncIter != Func->end();
				++FuncIter )
		{
			//llvm::Function::iterator == llvm::BasicBlock*
			llvm::BasicBlock *BB = llvm::cast< llvm::BasicBlock >( FuncIter );

			//a basicblock has list of instructions
			for( llvm::BasicBlock::iterator BBIter = BB->begin();
					BBIter != BB->end(); 
					++BBIter )
			{
				//llvm::BasicBlock::iterator == llvm::Instruction*
				llvm::Instruction* Inst = llvm::cast< llvm::Instruction >( BBIter );


				//check is load instruction
				if( llvm::isa< llvm::LoadInst >( Inst ) )
				{
					number_of_load++;
				}
			}
		}
	}

	//return
	return number_of_load;
}

//replace load instructions
void move_load( int load_index, int direction )
{
	//get load instruction
	static std::vector< llvm::LoadInst* > load_inst_vector;

	if( load_inst_vector.empty() )
	{
		//a module has list of function
		for( llvm::Module::iterator ModIter = TheModule->begin();
				ModIter != TheModule->end();
				++ModIter )
		{
			//llvm::Module::iteartor == llvm::Function*
			llvm::Function *Func = llvm::cast< llvm::Function >( ModIter );
			//a function has list of basicblocks
			for( llvm::Function::iterator FuncIter = Func->begin();
					FuncIter != Func->end();
					++FuncIter )
			{
				//llvm::Function::iterator == llvm::BasicBlock*
				llvm::BasicBlock *BB = llvm::cast< llvm::BasicBlock >( FuncIter );

				//a basicblock has list of instructions
				for( llvm::BasicBlock::iterator BBIter = BB->begin();
						BBIter != BB->end(); 
						++BBIter )
				{
					//llvm::BasicBlock::iterator == llvm::Instruction*
					llvm::Instruction* Inst = llvm::cast< llvm::Instruction >( BBIter );

					//check is load instruction
					if( llvm::isa< llvm::LoadInst >( Inst ) )
					{
						llvm::LoadInst* lInst = llvm::cast< llvm::LoadInst >( Inst );
						load_inst_vector.push_back( lInst );
					}
				}
			}
		}
	}


	//get target load instructions
	llvm::LoadInst *LInst = load_inst_vector[load_index];

	//get instrution immediately before/after load instruction
	llvm::BasicBlock* LParent = LInst->getParent();
	llvm::Instruction* LBeforeInst = nullptr;
	llvm::Instruction* LAfterInst = nullptr;

	for( llvm::BasicBlock::iterator BBIter = LParent->begin(); BBIter != LParent->end(); ++BBIter )
	{
		if( llvm::cast< llvm::Instruction >(BBIter) == LInst )
		{
			llvm::BasicBlock::iterator BBNextIter = BBIter;
			llvm::BasicBlock::iterator BBPrevIter = BBIter;

			BBNextIter++; BBPrevIter--;

			if( BBIter != LParent->begin() ) LBeforeInst = llvm::cast< llvm::Instruction >( BBPrevIter );
			if( BBNextIter != LParent->end() ) LAfterInst = llvm::cast< llvm::Instruction >( BBNextIter );
		}
	}

	//move instruction by direction
	if( direction == DIRECTION_BEFORE )
	{
		LInst->moveBefore( LBeforeInst );
		std::cout << "Move Instruction Immediately Before : ";
		LInst->print( raw_cout ); raw_cout.flush();
		std::cout << std::endl;
	}
	else if( direction == DIRECTION_AFTER ) 
	{
		LInst->moveAfter( LAfterInst );
		std::cout << "Move Instruction Immediately After : ";
		LInst->print( raw_cout ); raw_cout.flush();
		std::cout << std::endl;
	}
}
