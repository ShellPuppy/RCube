#include "Cube.h"
#include <direct.h>

int main()
{
	int n = 0;		//Cube size
	int seed = 0;	//Random seed
	int tmp = 0;

	printf("Starting a new cube\n");
	printf("Cube Size: ");
	tmp = scanf("%i", &n);
	printf("Choose a random seed: ");
	tmp = scanf("%i", &seed);
	printf("Generating Cube...\n");

	Cube cube(n);

	cube.scramble(seed);

	//No need to save progress for smaller cubes
	if (n >= 4196)
	{
		cube.saveenabled = true;
		cube.SaveCubeState();
	}

	printf("solving\n");

	tmp = mkdir("stats");
	tmp = mkdir("state");

	cube.Solve();

	cube.PrintStats();

	printf("done\n");

	return 0;
}