GXX=clang
SOURCE=HelloWorld.c
FLAGS=-emit-llvm
EXECUTABLE=HelloWorld

run:
	# Clang을 사용하여 소스코드를 LLVM IR 형태로 컴파일 할 수 있다.
	$(GXX) $(FLAGS) -S $(SOURCE) -o $(EXECUTABLE).ll
	$(GXX) $(FLAGS) -c $(SOURCE) -o $(EXECUTABLE).bc
	llvm-as $(EXECUTABLE).ll -o=$(EXECUTABLE).2.bc
	llvm-dis $(EXECUTABLE).bc -o=$(EXECUTABLE).2.ll
	vimdiff $(EXECUTABLE).ll $(EXECUTABLE).2.ll

clean:
	rm -rf *.ll
	rm -rf *.bc
	rm -rf $(EXECUTABLE)
