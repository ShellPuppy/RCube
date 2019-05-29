#include "Cube.h"
#include "CubeViewer.h"
#include <fstream>


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

	cube.Scramble(seed);

	//No need to save progress for smaller cubes
	if (n >= 4196)
	{
		cube.SaveEnabled = true;
		cube.SaveCubeState();
	}

	printf("solving\n");
	cube.Solve();

	cube.PrintStats();

	//Export images of each face
	CubeViewer cview;
	cview.ExportFaceDiagram(cube.faces[0], "Front Face.png", 1024, true);
	cview.ExportFaceDiagram(cube.faces[1], "Right Face.png", 1024, true);
	cview.ExportFaceDiagram(cube.faces[2], "Back Face.png", 1024, true);
	cview.ExportFaceDiagram(cube.faces[3], "Left Face.png", 1024, true);
	cview.ExportFaceDiagram(cube.faces[4], "Top Face.png", 1024, true);
	cview.ExportFaceDiagram(cube.faces[5], "Bottom Face.png", 1024, true);

	return EXIT_SUCCESS;
}