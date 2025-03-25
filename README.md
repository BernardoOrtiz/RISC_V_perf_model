open RISC_V_perf_model.cpp first. 

I am figuring out GIThub, haven't figured out how to make subfolders to help legibility.
Currently adding SIMD 128-1024 support in model, "pre-comipler" (regular 'c' to 'C'w/ SIMD data types) and "compiler" (convert 'C' to useable assembly). 
compiler and pre-compiler are in paranthesis since they are not fully vetted products, only enough to proof the model.
