#include "Cube.h"
#include "CubeViewer.h"

void StartNewCube()
{
	unsigned int n = 0;		//Cube size
	unsigned int seed = 0;	//Random seed
	int tmp = 0;

	printf("Starting a new cube\n");

	do
	{
		//Limit cube size from 1 to 65536
		printf("Cube Size (1-65536) : ");
		tmp = scanf("%u", &n);
	} while (n < 1 || n > 65536);

	printf("Choose a random seed: ");
	tmp = scanf("%u", &seed);

	//Create a new cube
	printf("Generating Cube...\n");
	Cube cube(n);

	//Scramble the cube using the seed value
	cube.Scramble(seed);

	//No need to save progress for smaller cubes
	if (n >= 8192)
	{
		printf("Saving enabled\n");
		cube.SaveEnabled = true;
		cube.SaveCubeState();
	}

	//Print stats before solving
	cube.PrintStats();

	//Solve it!
	printf("Solving\n");
	cube.Solve();

	//Print stats after solving
	cube.PrintStats();

	printf("Done\n");
	tmp = scanf("%i", &tmp);
}

void LoadExistingCube()
{
	Cube cube;

	//Load the cube from the save state
	cube.LoadCubeState();

	//Continue solving
	cube.Solve();

	cube.PrintStats();

	printf("Done\n");
	int tmp = scanf("%i", &tmp);
}

void ExampleImageOutput()
{
	//Create a small cube
	Cube cube(32);

	//Scramble the cube with a random seed
	cube.Scramble(1234);

	//Create instance of a cube viewer
	CubeViewer cview;

	//Export images of each face

	//Specify cube face, filename, image size, include gridlines
	cview.ExportFaceDiagram(cube.faces[0], "Front Face.png", 1024, true);
	cview.ExportFaceDiagram(cube.faces[1], "Right Face.png", 1024, true);
	cview.ExportFaceDiagram(cube.faces[2], "Back Face.png", 1024, true);
	cview.ExportFaceDiagram(cube.faces[3], "Left Face.png", 1024, true);
	cview.ExportFaceDiagram(cube.faces[4], "Top Face.png", 1024, true);
	cview.ExportFaceDiagram(cube.faces[5], "Bottom Face.png", 1024, true);
}

int main()
{

	//Start a new cube and solve it
	StartNewCube();
	
	//Uncomment to load an existing cube from a save state
	//LoadExistingCube();

	//Uncomment to run the example image output
	//ExampleImageOutput();

	return EXIT_SUCCESS;
}