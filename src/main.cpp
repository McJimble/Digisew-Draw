#include "SketchProgram.h"
#include <iostream>

// Ex. command args (files must be found in correct resource folders):
// SketchProgram.exe normal_map1.png density_map_up.png
int main(int args, char** argv)
{
	SketchProgram program = SketchProgram();
	program.Initialize(args, argv);
	program.MainLoop();

	return 0;
}