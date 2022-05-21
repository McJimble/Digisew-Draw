#include "SketchProgram.h"
#include <iostream>

int main()
{
	SketchProgram program = SketchProgram();
	program.Initialize();
	program.MainLoop();

	return 0;
}