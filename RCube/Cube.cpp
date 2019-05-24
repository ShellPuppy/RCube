#include "Cube.h"
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <chrono>

//Pairs of edge colors
const byte Cube::edgemap[24] =
{
	5,2,
	2,3,
	2,4,
	2,1,
	5,1,
	4,1,
	4,3,
	5,3,
	0,5,
	0,1,
	0,4,
	0,3
};

//Parameters for center commutators 
const byte Cube::cmap[30][6] =
{
	{0,1,4,5,3,3},
	{0,2,0,0,0,0},	//TODO not filled out (not used)
	{0,3,5,4,1,1},
	{0,4,3,1,0,0},
	{0,5,1,3,2,2},
	{1,0,5,4,1,1},
	{1,2,4,5,3,3},
	{1,3,0,0,0,0},	//TODO not filled out (not used)
	{1,4,0,2,0,1},
	{1,5,2,0,2,1},
	{2,0,1,3,0,2},
	{2,1,5,4,1,1},
	{2,3,4,5,3,3},
	{2,4,1,3,0,2},
	{2,5,3,1,2,0},
	{3,0,4,5,3,3},
	{3,1,4,5,3,3},
	{3,2,5,4,1,1},
	{3,4,2,0,0,3},
	{3,5,0,2,2,3},
	{4,0,1,3,2,2},
	{4,1,2,0,3,2},
	{4,2,3,1,0,2},
	{4,3,0,2,1,2},
	{4,5,0,2,1,3},
	{5,0,3,1,0,0},
	{5,1,0,2,3,0},
	{5,2,1,3,2,0},
	{5,3,2,0,1,0},
	{5,4,0,0,0,0}	//TODO not filled out (not used)
};

//Corner color definitions
const byte Cube::corners[8][3] =
{
	{4,0,3},
	{4,3,2},
	{4,1,2},
	{4,0,1},
	{5,3,2},
	{5,0,3},
	{5,0,1},
	{5,1,2},
};

void Cube::initalize(uint rsize)
{
	//Size of the cube
	RowSize = rsize;

	//Size - 1 (used to simplify array indexing)
	R1 = RowSize - 1;

	//Mid point = RowSize / 2
	Mid = RowSize >> 1;

	//Is the cube an even or odd size
	IsEven = (RowSize & 0x01) != 0x01;

	//Force memory size to be a power of 2
	int MemSize = (int)pow(2, (ceil(log2(rsize))));

	//Build the faces
	faces = new Face[6];
	for (char i = 0; i < 6; i++)
	{
		faces[i].Initialize(i, RowSize, MemSize);
	}
}

void Cube::scramble(int seed)
{
	uint rnd;

	//Do the entire process 3 times
	for (uint r = 0; r < 3; r++)
	{
		srand(seed);

		//Rotate a random slice by a random amount 
		for (uint i = 0; i < 3 * RowSize; i++)
		{
			rnd = rand() % 3;

			if (rnd == 0) RotateX(rand() % RowSize, (rand() & 3) + 1);
			if (rnd == 1) RotateY(rand() % RowSize, (rand() & 3) + 1);
			if (rnd == 2) RotateZ(rand() % RowSize, (rand() & 3) + 1);
		}

		//Force every row and column to rotate atleast once
		for (uint i = 0; i < RowSize; i++)
		{
			RotateX(i, (rand() & 3) + 1);
			RotateY(i, (rand() & 3) + 1);
			RotateZ(i, (rand() & 3) + 1);
		}

		//Rotate a random slice by a random amount 
		for (uint i = 0; i < 3 * RowSize; i++)
		{
			rnd = rand() % 3;

			if (rnd == 0) RotateX(rand() % RowSize, (rand() & 3) + 1);
			if (rnd == 1) RotateY(rand() % RowSize, (rand() & 3) + 1);
			if (rnd == 2) RotateZ(rand() % RowSize, (rand() & 3) + 1);
		}

		seed = (seed + 1) % 0x0FFFFFF;
	}

}

void Cube::Solve()
{
	//Keep track of processing time
	ProcessStartTime = std::chrono::high_resolution_clock::now();

	AlignTrueCenters();

	//Stage 0 through 15 (solve centers)
	if (stage < 15)
	{
		SolveCenters();
	}

	//State 16 (solve corners)
	if (stage == 16)
	{
		SaveCubeState();
		SolveCorners();
	}

	//State 17 (solve edges)
	if (stage == 17)
	{
		SolveEdges();
	}

	//Cube is solved
}

#pragma region Centers

void Cube::SolveCenters()
{
	//Solve each center by 'pushing' pieces to the desired face
	//The stages are used to start the solving from a saved state

	//Push R color pieces from F to R
	if (stage == 0)
	{
		PushCenterPieces(F, R, R); 
		stage++;
	}

	//Push R color pieces from U to R
	if (stage == 1)
	{
		SaveCubeState();
		PushCenterPieces(U, R, R);
		stage++;
	}

	//Push R color pieces from B to R
	if (stage == 2)
	{
		SaveCubeState();
		PushCenterPieces(B, R, R);
		stage++;
	}

	//Push R color pieces from L to R
	if (stage == 3)
	{
		SaveCubeState();
		PushCenterPieces(L, R, R);
		stage++;
	}

	//Push R color pieces from D to R
	if (stage == 4)
	{
		SaveCubeState();
		PushCenterPieces(D, R, R);
		stage++;
	}

	//Push L color pieces from U to L
	if (stage == 5)
	{
		SaveCubeState();
		PushCenterPieces(U, L, L);
		stage++;
	}

	//Push L color pieces from D to L
	if (stage == 6)
	{
		SaveCubeState();
		PushCenterPieces(D, L, L);
		stage++;
	}

	//Push L color pieces from B to L
	if (stage == 7)
	{
		SaveCubeState();
		PushCenterPieces(B, L, L);
		stage++;
	}

	//Push L color pieces from F to L
	if (stage == 8)
	{
		SaveCubeState();
		PushCenterPieces(F, L, L);
		stage++;
	}

	//Push F color pieces from B to F
	if (stage == 9)
	{
		SaveCubeState();
		PushCenterPieces(B, F, F);
		stage++;
	}

	//Push F color pieces from U to F
	if (stage == 10)
	{
		SaveCubeState();
		PushCenterPieces(U, F, F);
		stage++;
	}

	//Push F color pieces from D to F
	if (stage == 11)
	{
		SaveCubeState();
		PushCenterPieces(D, F, F);
		stage++;
	}

	//Push D color pieces from U to D
	if (stage == 12)
	{
		SaveCubeState();
		PushCenterPieces(U, D, D);
		stage++;
	}

	//Push D color pieces from B to D
	if (stage == 13)
	{
		SaveCubeState();
		PushCenterPieces(B, D, D);
		stage++;
	}

	//Optomize Top Center
	if (stage == 14)
	{
		SaveCubeState();
		OptomizeTopCenter();
		stage++;
	}

	//Push U color pieces from B to U
	if (stage == 15)
	{
		SaveCubeState();
		PushCenterPieces(U, B, B);
		stage++;
	}
}

//Align the center piece of an odd sized cube
void Cube::AlignTrueCenters()
{
	if (IsEven) return; //skip this step if its an even size cube
	
	byte q;

	//Find Front Center piece
	for (byte i = 0; i < 6; i++)
	{
		if (faces[i].GetRC(Mid, Mid) == F) q = i;
	}

	//Move Front Center piece to the front
	if (q == U) Move(L, Mid, 1);
	if (q == D) Move(L, Mid, -1);
	if (q == L) Move(U, Mid, -1);
	if (q == R) Move(U, Mid, 1);
	if (q == B) Move(U, Mid, 2);

	//Find Up Center piece
	for (byte i = 0; i < 6; i++)
	{
		if (faces[i].GetRC(Mid, Mid) == U) q = i;
	}

	//Move up to the top
	if (q == D) Move(F, Mid, 2);
	if (q == L) Move(F, Mid, 1);
	if (q == R) Move(F, Mid, -1);
}

//Return true if src and dst faces are opposites
bool Cube::IsOpposite(byte src, byte dst)
{
	if ((src == 0 && dst == 2) || (src == 2 && dst == 0)) return true;
	if ((src == 1 && dst == 3) || (src == 3 && dst == 1)) return true;
	if ((src == 4 && dst == 5) || (src == 5 && dst == 4)) return true;

	return false;
}

//Push center pieces of any color from one face to another using commutators 
void Cube::PushCenterPieces(byte src, byte dst, byte color)
{
	//Lookup table to find translation parameters
	int map = FindCommutatorMap(src, dst);

	byte srcl = cmap[map][2];			//The face thats 'left' of the src face (in the direction of the destination)
	int sq = -(int)cmap[map][4];		//Quadrant to use on the source face 
	int dq = -(int)cmap[map][5];		//The destination is rotated relative to the source
	int d = 1;

	if (IsOpposite(src, dst)) d = 2;

	uint *mstack = new uint[Mid];		//Temporary array to keep track of rows that were used

	int stkptr = 0;
	int pieces = 0;
	uint start = Mid;

	//If starting from a save state then set the start point
	if (itteration > 0) start = itteration;
	
	for (int quadrant = qstate; quadrant < 4; ++quadrant)
	{
		this->qstate = quadrant;

		for (uint r = start; r < R1; ++r)
		{
			this->itteration = r;

			if (saveenabled)
			{
				//Save 
				if (CurrentProcessDuration() >= 3.0)
				{
					SaveCubeState();
				}
			}

			while (true)
			{
				pieces = 0;
				stkptr = 0;
				for (uint c = 1; c < Mid; ++c)
				{
					if (faces[src].GetRCQ(r, c, sq) == color)
					{
						pieces++;
						if (faces[dst].GetRCQ(r, c, dq) != color)
						{
							mstack[stkptr++] = c;
						}
					}
				}

				//The row is clear - move on
				if (pieces == 0) break;

				//The row is not clear but has no valid moves (rotate the destination face and continue)
				if (stkptr <= 0)
				{
					Move(dst, 0, 1);
					continue;
				}

				//Move the pieces
				for (uint c = 0; c < stkptr; ++c) Move(srcl, mstack[c], -d);

				Move(dst, 0, 1);

				Move(srcl, r, -d);

				Move(dst, 0, -1);

				for (uint c = 0; c < stkptr; ++c) Move(srcl, mstack[c], d);

				Move(dst, 0, 1);

				Move(srcl, r, d);
			}
		}

		//reset the start point
		start = Mid;

		Move(src, 0, 1);
	}

	qstate = 0;
	itteration = 0;
	delete mstack;
}

//Save some time by swapping rows between Up and Back faces that have more Up or Back pieces 
void Cube::OptomizeTopCenter()
{
	uint r, c;

	int Br, Ur;
	for (int z = 0; z < 8; z++)
	{
		for (int q = 0; q < 8; q++)
		{
			for (c = 1; c < Mid; ++c)
			{
				Br = 0;
				Ur = 0;
				//count B pieces in B and U columns
				for (r = 1; r < R1; ++r)
				{
					if (faces[B].GetRC(r, R1 - c) == B) Br++;
					if (faces[U].GetRC(r, R1 - c) == B) Ur++;
				}
				if (Ur - Br > 0)
				{
					Move(L, c, 1);
					Move(U, 0, 2);
					Move(L, c, -1);
				}
			}
			Move(U, 0, 1);
		}
		Move(B, 0, 1);
	}


}

//Find the correct commuator map
int Cube::FindCommutatorMap(byte src, byte dst)
{
	for (int i = 0; i < 30; i++)
		if (cmap[i][0] == src && cmap[i][1] == dst) return i;
	return 0;
}

#pragma endregion

#pragma region Edges

void Cube::SolveEdges()
{
	byte f0, f1;
	byte l0, l1;
	byte r0, r1;
	byte d0, d1;
	
	int *mstack = new int[Mid];
	
	int mptr = 0;
	bool found;

	for (int de = 0; de < 12; ++de)
	{

		//Move edge into place
		SetDestinationEdge(de, true);

		//Get the two edge colors 

		f0 = edgemap[de << 1];
		f1 = edgemap[(de << 1) + 1];

		for (int se = 0; se < 12; ++se)
		{
			//todo skip if edge is solved

			//move edge into place
			SetSourceEdge(se, true);

			//transfer good rows
			do
			{
				found = false;

				mptr = 0;
				for (int r = 1; r < Mid; ++r)
				{
					GetLeftEdgePieces(r, l0, l1);
					if (l0 == f0 && l1 == f1) mstack[mptr++] = r;
				}

				if (mptr > 0)
				{
					found = true;
					for (int i = 0; i < mptr; ++i)
					{
						Move(D, mstack[i], 1);
						Move(D, R1 - mstack[i], 1);
					}

					FlipEdge();

					for (int i = 0; i < mptr; ++i)
					{
						Move(D, mstack[i], -1);
					}

					UnFlipEdge();

					for (int i = 0; i < mptr; ++i)
					{
						Move(D, R1 - mstack[i], -1);
					}
				}

				mptr = 0;
				for (int r = Mid; r < R1; ++r)
				{
					GetLeftEdgePieces(r, l0, l1);
					if (l0 == f0 && l1 == f1)
					{
						mstack[mptr++] = r;
					}
				}

				if (mptr > 0)
				{
					found = true;
					for (int i = 0; i < mptr; ++i)
					{
						Move(D, mstack[i], 1);
						Move(D, R1 - mstack[i], 1);
					}

					FlipEdge();

					for (int i = 0; i < mptr; ++i)
					{
						Move(D, mstack[i], -1);
					}

					UnFlipEdge();

					for (int i = 0; i < mptr; ++i)
					{
						Move(D, R1 - mstack[i], -1);
					}
				}

				//transfer backward rows
				mptr = 0;
				for (int r = 1; r < R1; ++r)
				{
					GetLeftEdgePieces(r, l0, l1);
					if (l0 == f1 && l1 == f0) 	mstack[mptr++] = r;
				}

				if (mptr > 0)
				{
					found = true;
					FlipEdge();

					for (int i = 0; i < mptr; ++i)
					{
						Move(D, mstack[i], 1);
					}

					UnFlipEdge();

					for (int i = 0; i < mptr; ++i)
					{
						Move(D, mstack[i], -1);
					}
				}

			} while (found);




			//Move edge back to origin
			SetSourceEdge(se, false);
		}

		//Fix parity

		for (int r = 1; r < Mid; ++r)
		{
			GetRightEdgePieces(r, r0, r1);

			if (r0 == f1 && r1 == f0) FixParity(r);
			{
				mstack[mptr++] = r;
			}
		}


		//Move edge back to origin
		SetDestinationEdge(de, false);

	}

	delete mstack;

}

//Flips the F-R edge
void Cube::FlipEdge()
{
	Move(R, 0, 1);
	Move(U, 0, 1);
	Move(R, 0, -1);
	Move(F, 0, 1);
	Move(R, 0, -1);
	Move(F, 0, -1);
	Move(R, 0, 1);
}

//Un-Flips the F-R edge 
void Cube::UnFlipEdge()
{
	Move(R, 0, -1);
	Move(F, 0, 1);
	Move(R, 0, 1);
	Move(F, 0, -1);
	Move(R, 0, 1);
	Move(U, 0, -1);
	Move(R, 0, -1);
}

//Get the values of an edge piece on the left side of the F face
void Cube::GetLeftEdgePieces(int row, byte &l0, byte &l1)
{
	l0 = faces[L].GetRC(row, R1);
	l1 = faces[F].GetRC(row, 0);
}

//Get the values of an edge piece on the right side of the F face
void Cube::GetRightEdgePieces(int row, byte &r0, byte &r1)
{
	r0 = faces[F].GetRC(row, R1);
	r1 = faces[R].GetRC(row, 0);
}

//Fixes edge parity on a row (front right edge only)
void Cube::FixParity(int row)
{
	Move(D, row, -1);
	Move(R, 0, 2);
	Move(U, row, 1);
	Move(F, 0, 2);
	Move(U, row, -1);
	Move(F, 0, 2);
	Move(D, row, 2);
	Move(R, 0, 2);
	Move(D, row, 1);
	Move(R, 0, 2);
	Move(D, row, -1);
	Move(R, 0, 2);
	Move(F, 0, 2);
	Move(D, row, 2);
	Move(F, 0, 2);
}

//Prepare an edge to be solved - or put the edge back into place set or !set
void Cube::SetDestinationEdge(int edge, bool set)
{
	if (set)
	{
		switch (edge)
		{
		case 0: //D-B
			Move(D, 0, -1);
			Move(R, 0, 1);
			break;
		case 1: //B-L
			Move(B, 0, 2);
			Move(R, 0, 2);
			break;
		case 2://B-U
			Move(B, 0, -1);
			Move(R, 0, 2);
			break;
		case 3://B-R
			Move(R, 0, 2);
			break;
		case 4://D-R
			Move(R, 0, 1);
			break;
		case 5://U-R
			Move(R, 0, -1);
			break;
		case 6://U-L
			Move(U, 0, 2);
			Move(R, 0, -1);
			break;
		case 7://D-L
			Move(D, 0, 2);
			Move(R, 0, 1);
			break;
		case 8://F-D
			Move(F, 0, -1);
			break;
		case 9://F-R
			break;
		case 10://F-U
			Move(F, 0, 1);
			break;
		case 11://F-L
			Move(F, 0, 2);
			break;
		}
		return;
	}

	if (!set)
	{
		switch (edge)
		{
		case 0: //D-B
			Move(R, 0, -1);
			Move(D, 0, 1);
			break;
		case 1: //B-L
			Move(R, 0, 2);
			Move(B, 0, 2);
			break;
		case 2://B-U
			Move(R, 0, 2);
			Move(B, 0, 1);
			break;
		case 3://B-R
			Move(R, 0, 2);
			break;
		case 4://D-R
			Move(R, 0, -1);
			break;
		case 5://U-R
			Move(R, 0, 1);
			break;
		case 6://U-L
			Move(R, 0, 1);
			Move(U, 0, 2);
			break;
		case 7://D-L
			Move(R, 0, -1);
			Move(D, 0, 2);
			break;
		case 8://F-D
			Move(F, 0, 1);
			break;
		case 9://F-R
			break;
		case 10://F-U
			Move(F, 0, -1);
			break;
		case 11://F-L
			Move(F, 0, 2);
			break;
		}
	}
}

//Prepare an edge to the source of solved pieces - or put the edge back into place set or !set
void Cube::SetSourceEdge(int edge, bool set)
{
	if (set)
	{
		switch (edge)
		{
		case 0: //D-B
			Move(D, 0, 1);
			Move(L, 0, -1);
			break;
		case 1: //B-L
			Move(L, 0, 2);
			break;
		case 2://B-U
			Move(U, 0, -1);
			Move(L, 0, 1);
			break;
		case 3://B-R
			Move(B, 0, 2);
			Move(L, 0, 2);
			break;
		case 4://D-R
			Move(D, 0, 2);
			Move(L, 0, -1);
			break;
		case 5://U-R
			Move(U, 0, 2);
			Move(L, 0, 1);
			break;
		case 6://U-L
			Move(L, 0, 1);
			break;
		case 7://D-L
			Move(L, 0, -1);
			break;
		case 8://F-D
			Move(D, 0, -1);
			Move(L, 0, -1);
			break;
		case 9://F-R
			//Move(F, 0, 2);
			break;
		case 10://F-U
			Move(U, 0, 1);
			Move(L, 0, 1);
			break;
		case 11://F-L

			break;
		}
		return;
	}

	if (!set)
	{
		switch (edge)
		{
		case 0: //D-B
			Move(L, 0, 1);
			Move(D, 0, -1);
			break;
		case 1: //B-L
			Move(L, 0, 2);
			break;
		case 2://B-U
			Move(L, 0, -1);
			Move(U, 0, 1);
			break;
		case 3://B-R
			Move(L, 0, 2);
			Move(B, 0, 2);
			break;
		case 4://D-R
			Move(L, 0, 1);
			Move(D, 0, 2);
			break;
		case 5://U-R
			Move(L, 0, -1);
			Move(U, 0, 2);
			break;
		case 6://U-L
			Move(L, 0, -1);
			break;
		case 7://D-L
			Move(L, 0, 1);
			break;
		case 8://F-D
			Move(L, 0, 1);
			Move(D, 0, 1);
			break;
		case 9://F-R
			//Move(F, 0, 2);
			break;
		case 10://F-U
			Move(L, 0, -1);
			Move(U, 0, -1);
			break;
		case 11://F-L

			break;
		}
	}
}

#pragma endregion

#pragma region Corners

void Cube::SolveCorners()
{
	
	byte c0, c1, c2;
	int pos = 0;

	//Solve the U face corners
	for (int i = 0; i < 4; ++i)
	{
		pos = FindCorner(i);

		switch (pos)
		{
		case 0:
			Move(L, 0, 1);
			Move(D, 0, 1);
			Move(L, 0, -1);
			break;
		case 1:
			Move(L, 0, -1);
			Move(D, 0, 2);
			Move(L, 0, 1);
			break;
		case 2:
			Move(R, 0, 1);
			Move(D, 0, 1);
			Move(R, 0, -1);
			Move(D, 0, 2);
			break;
		case 3:
			Move(R, 0, -1);
			Move(D, 0, -1);
			Move(R, 0, 1);
			Move(D, 0, 1);
			break;
		case 4:
			Move(D, 0, 2);
			break;
		case 5:
			Move(D, 0, 1);
			break;
		case 6:
			break;
		case 7:
			Move(D, 0, -1);
			break;
		}
		
		pos = FindCorner(i);

		while (!(pos == 3 && faces[U].GetRC(0, R1) == U))
		{
			Move(R, 0, -1);
			Move(D, 0, -1);
			Move(R, 0, 1);
			Move(D, 0, 1);
			pos = FindCorner(i);
		}

		if (i < 3) Move(U, 0, -1);

	}

	//Temporarily move the U face corners to the D face
	Move(L, 0, 2);
	Move(R, 0, 2);


	//Solve the D face corners

	//Put one corner in a known position
	pos = FindCorner(4);
	if (pos == 0) Move(U, 0, -1);
	if (pos == 1) Move(U, 0, 2);
	if (pos == 2) Move(U, 0, 1);

	int c[3];

	bool aligned = false;

	//The remaining corners can end up in 6 different configurations
	c[FindCorner(5)] = 5;
	c[FindCorner(6)] = 6;
	c[FindCorner(7)] = 7;

	//Solve each configuration 

	if (c[0] == 5 && c[1] == 6 && c[2] == 7)
	{
		Move(U, 0, 1);
	}
	
	if (c[0] == 5 && c[1] == 7 && c[2] == 6)
	{
		Move(U, 0, 1);
		FlipCorners();
		FlipCorners();
		Move(U, 0, -1);
	}

	if (c[0] == 6 && c[1] == 5 && c[2] == 7)
	{
		Move(U, 0, 2);
		FlipCorners();
		FlipCorners();
		Move(U, 0, 2);
	}

	if (c[0] == 6 && c[1] == 7 && c[2] == 5)
	{
		FlipCorners();
		FlipCorners();
		Move(U, 0, 1);
	}

	if (c[0] == 7 && c[1] == 5 && c[2] == 6)
	{
		FlipCorners();
		Move(U, 0, 1);
	}

	if (c[0] == 7 && c[1] == 6 && c[2] == 5)
	{
		FlipCorners();
		Move(U, 0, -1);
		FlipCorners();
		Move(U, 0, -1);
	}

	//Force all of the D face colors in the same direction
	for (int i = 0; i < 4; ++i)
	{

		while (faces[U].GetRC(0, R1) != D)
		{
			Move(R, 0, -1);
			Move(D, 0, -1);
			Move(R, 0, 1);
			Move(D, 0, 1);
		}
		Move(U, 0, 1);
	}

	//Push the D corners to the D face and bring the U face corners up
	Move(L, 0, 2);
	Move(R, 0, 2);

	stage++;
}

//Rotate 3 corners on the U face
void Cube::FlipCorners()
{
	Move(U, 0, 1);
	Move(R, 0, 1);
	Move(U, 0, -1);
	Move(L, 0, -1);
	Move(U, 0, 1);
	Move(R, 0, -1);
	Move(U, 0, -1);
	Move(L, 0, 1);
}

//Gets the 3 face colors of a corner
void Cube::GetCorner(int cr, byte &c0, byte &c1, byte &c2)
{
	switch (cr)
	{
	case 0:
		c0 = faces[U].GetRC(0, 0);
		c1 = faces[F].GetRC(R1, 0);
		c2 = faces[L].GetRC(R1, R1);
		return;
	case 1:
		c0 = faces[U].GetRC(R1, 0);
		c1 = faces[L].GetRC(R1, 0);
		c2 = faces[B].GetRC(R1, R1);
		return;
	case 2:
		c0 = faces[U].GetRC(R1, R1);
		c1 = faces[R].GetRC(R1, R1);
		c2 = faces[B].GetRC(R1, 0);
		return;
	case 3:
		c0 = faces[U].GetRC(0, R1);
		c1 = faces[F].GetRC(R1, R1);
		c2 = faces[R].GetRC(R1, 0);
		return;
	case 4:
		c0 = faces[D].GetRC(0, 0);
		c1 = faces[L].GetRC(0, 0);
		c2 = faces[B].GetRC(0, R1);
		return;
	case 5:
		c0 = faces[D].GetRC(R1, 0);
		c1 = faces[F].GetRC(0, 0);
		c2 = faces[L].GetRC(0, R1);
		return;
	case 6:
		c0 = faces[D].GetRC(R1, R1);
		c1 = faces[F].GetRC(0, R1);
		c2 = faces[R].GetRC(0, 0);
		return;
	case 7:
		c0 = faces[D].GetRC(0, R1);
		c1 = faces[R].GetRC(0, R1);
		c2 = faces[B].GetRC(0, 0);
		return;

	}
}

//Returns true if the corner in position cr has these three colors
bool Cube::IsCorner(int cr, byte c0, byte c1, byte c2)
{
	byte b0, b1, b2;

	GetCorner(cr, b0, b1, b2);

	return	(c0 == b0 || c0 == b1 || c0 == b2) &&
		(c1 == b0 || c1 == b1 || c1 == b2) &&
		(c2 == b0 || c2 == b1 || c2 == b2);

}

//Finds the position of corner cr
int Cube::FindCorner(int cr)
{
	for (int i = 0; i < 8; ++i)
	{
		if (IsCorner(i, corners[cr][0], corners[cr][1], corners[cr][2])) return i;
	}

	return -1;
}



#pragma endregion

#pragma region Cube State

void Cube::SaveCubeState()
{
	if (!saveenabled) return;

	printf("Saving cube state\n");

	//Keep track of total hours of processing
	Hours += CurrentProcessDuration();

	printf("Total duration = %.3f hours\n", Hours);

	std::ofstream out("state//cubestate.bin", std::ios::out | std::ios::binary);
	out.write((char*)this, sizeof(Cube));
	out.flush();
	out.close();

	for (int i = 0; i < 6; ++i) faces[i].SaveFaceState();
	
	printf("Done saving cube state\n");

	//Reset process timer
	ProcessStartTime = std::chrono::high_resolution_clock::now();
}

void Cube::LoadCubeState()
{
	printf("Loading cube from file\n");
	
	std::ifstream in("state//cubestate.bin", std::ios::out | std::ios::binary);
	in.read((char*)this, sizeof(Cube));
	in.close();

	faces = new Face[6];
	for (byte b = 0; b < 6; ++b)
	{
		faces[b].LoadFaceState(b);
	}

	printf("Done loading cube\n");

}

void Cube::PrintStats()
{
	printf("\nCube Size : %i\n", RowSize);
	printf("Total Moves : %llu\n", MoveCount);
	printf("Total Hours : %.7f\n", Hours);
	printf("Stage : %i\n", stage);
	printf("Quadrant : %i\n", qstate);
	printf("Itteration : %i\n\n", itteration);
}

#pragma endregion

Cube::Cube(int size)
{
	MoveCount = 0;
	Hours = 0.0;
	faces = nullptr;
	saveenabled = false;
	stage=0;			//stage of the solve (used for recovering from a restart)
	qstate=0;			//current quadrant being solved
	itteration=0;		//itteration of the current stage 
	initalize(size);
	ProcessStartTime = std::chrono::high_resolution_clock::now(); //Reset process timer
}

Cube::Cube()
{
	//use this constructor when reloading the cube state
}

Cube::~Cube()
{
	//Cleanup
	if (faces != nullptr) delete[] faces;
}
