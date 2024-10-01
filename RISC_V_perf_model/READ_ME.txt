// Author: Bernardo Ortiz
// bernardo.ortiz.vanderdys@gmail.com
// cell: 949/400-5158
// addr: 67 Rockwood	Irvine, CA 92614

This is a sample performance model. characterised by:
* runs assembly code
* handles multithreading
* models TLB full scenario

NOTE: 
1) Model uses my understanding of RISC V instruction set, not validated against 
any official test test vectors. 
2) Model emphasis is:
	a) run fast, 
		-> avoid C++ structures whic ten to slow things down
		-> appropriate use of pointers to not move large structures around,
			just their pointers
	b) easy resource resizing (basic to any performance model)
		-> cach sizes
		-> decode width
		-> reorder buffer size
		-> etc.
	c) be easy to correct, re-write to model specific needs
		-> fix errors
		-> changes to meet specific micro architectures
		-> allow instruction set changes if desired
	d) visualize what the processor is actually doing
		-> help pinpoint performance bottlenecks
			* for example; Large number of registers make 4k pages inpractical 
			since the cost of page miss is so high. 
			SIMD will only make the situation worse.
		-> RSIC V, like all reduced instruction set architectures is more heavily
			memory access dependent than other arcitectures. Therefore needs more 
			attention to memory infrastructure, data organization and SIMD 
			instructions than other architectures.

Feedback is appreciated. corrections, comments, etc.

I am a hardware guy, not software guru. Compiler needs a lot of work, not well oorganized,
not organized for maintainabilty (quick and dirty, a bit too dirty).
