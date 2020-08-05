#include "Cube.h"
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <chrono>
#include <math.h>
#include <cstring>

//Pairs of edge colors
const byte Cube::EdgeColorMap[24] =
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
	{0,2,3,1,0,2},
	{0,3,5,4,1,1},
	{0,4,3,1,0,0},
	{0,5,1,3,2,2},
	{1,0,5,4,1,1},
	{1,2,4,5,3,3},
	{1,3,5,4,1,1},
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
	{5,4,2,0,1,3}
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

//Edge rotation map - used to keep track of edges as the move during the solve
const byte Cube::EdgeRotMap[6][4] =
{
	{8,11,10,9},//F
	{3,4,9,5},	//R
	{0,3,2,1},	//B
	{1,6,11,7},	//L
	{2,5,10,6},	//U
	{0,7,8,4}	//D
};

void Cube::Initalize(uint rsize)
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
void Cube::Cleanup()
{
	if (faces != nullptr) delete[] faces;
	Reset();
}
void Cube::Scramble(int seed)
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

	//Cube Size 1 : Trivial case
	if (RowSize == 1)
	{
		MoveCount++;
		for (byte i = 0; i < 6; ++i) faces[i].SetRC(0, 0, i);
		return;
	}

	//Size 2 : Only have to solve corners
	if (RowSize == 2)
	{
		SolveCorners();
		return;
	}

	//Odd size cubes need to have their center pieces aligned first
	AlignTrueCenters();

	//Stage 0 through 14 (solve centers)
	if (Stage <= 14)
	{
		SolveCenters();
	}

	//Stage 15 (solve corners)
	if (Stage == 15)
	{
		SaveCubeState();
		SolveCorners();
	}

	//State 16 (solve edges)
	if (Stage == 16)
	{
		if (IsEven)
		{
			SolveEdgesEven();
		}
		else
		{
			SolveEdgesOdd();
			if (!this->IsCubeSolved()) SolveEdgesOdd();
		}
	}

	//Cube is solved
	//Keep track of total hours of processing
	Hours += CurrentProcessDuration();
}

#pragma region Centers

void Cube::SolveCenters()
{
	//Solve each center by 'pushing' pieces to the desired face
	//The stages are used to start the solving from a saved state

	//No need to solve centers for cubes less than size 4
	if (RowSize < 4)
	{
		Stage = 15;
		return;
	}

	//Push R color pieces from F to R
	if (Stage == 0)
	{
		PushCenterPieces(F, R, R);
		Stage++;
	}

	//Stage = 19; return;

	//Push R color pieces from U to R
	if (Stage == 1)
	{
		SaveCubeState();
		PushCenterPieces(U, R, R);
		Stage++;
	}

	//Push R color pieces from B to R
	if (Stage == 2)
	{
		SaveCubeState();
		PushCenterPieces(B, R, R);
		Stage++;
	}

	//Push R color pieces from L to R
	if (Stage == 3)
	{
		SaveCubeState();
		PushCenterPieces(L, R, R);
		Stage++;
	}

	//Push R color pieces from D to R
	if (Stage == 4)
	{
		SaveCubeState();
		PushCenterPieces(D, R, R);
		Stage++;
	}

	//Push L color pieces from U to L
	if (Stage == 5)
	{
		SaveCubeState();
		PushCenterPieces(U, L, L);
		Stage++;
	}

	//Push L color pieces from D to L
	if (Stage == 6)
	{
		SaveCubeState();
		PushCenterPieces(D, L, L);
		Stage++;
	}

	//Push L color pieces from B to L
	if (Stage == 7)
	{
		SaveCubeState();
		PushCenterPieces(B, L, L);
		Stage++;
	}

	//Push L color pieces from F to L
	if (Stage == 8)
	{
		SaveCubeState();
		PushCenterPieces(F, L, L);
		Stage++;
	}

	//Push F color pieces from B to F
	if (Stage == 9)
	{
		SaveCubeState();
		PushCenterPieces(B, F, F);
		Stage++;
	}

	//Push F color pieces from U to F
	if (Stage == 10)
	{
		SaveCubeState();
		PushCenterPieces(U, F, F);
		Stage++;
	}

	//Push F color pieces from D to F
	if (Stage == 11)
	{
		SaveCubeState();
		PushCenterPieces(D, F, F);
		Stage++;
	}

	//Push D color pieces from U to D
	if (Stage == 12)
	{
		SaveCubeState();
		PushCenterPieces(U, D, D);
		Stage++;
	}

	//Push D color pieces from B to D
	if (Stage == 13)
	{
		SaveCubeState();
		PushCenterPieces(B, D, D);
		Stage++;
	}

	//Push U color pieces from B to U
	if (Stage == 14)
	{
		SaveCubeState();
		PushCenterPieces(U, B, B);
		Stage++;
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
	//Lookup table to find commuator parameters
	int map = FindCommutatorMap(src, dst);

	byte srcl = cmap[map][2];			//The face thats 'left' of the src face (in the direction of the destination)
	int sq = -(int)cmap[map][4];		//Quadrant to use on the source face 
	int dq = -(int)cmap[map][5];		//The destination is rotated relative to the source
	int d = 1;

	if (IsOpposite(src, dst)) d = 2;

	uint* mstack = new uint[Mid];		//Temporary array to keep track of columns that are being moved 

	uint stkptr = 0;
	uint pieces = 0;
	uint start = Mid;

	//If starting from a save state then set the start point
	if (Itteration > 0) start = Itteration;

	for (int quadrant = QState; quadrant < 4; ++quadrant)
	{
		this->QState = quadrant;

		for (uint r = start; r < R1; ++r)
		{
			this->Itteration = r;

			if (SaveEnabled)
			{
				//Save cube state every 1.0 hours
				if (CurrentProcessDuration() >= 1.0)
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

				OptomizedMovement(mstack, stkptr, r, src, dst, sq, dq);

				//Move the pieces
				//for (uint c = 0; c < stkptr; ++c) Move(srcl, mstack[c], -d);

				//Move(dst, 0, 1);

				//Move(srcl, r, -d);

				//Move(dst, 0, -1);

				//for (uint c = 0; c < stkptr; ++c) Move(srcl, mstack[c], d);

				//Move(dst, 0, 1);

				//Move(srcl, r, d);
			}
		}

		//reset the start point
		start = Mid;

		//Rotate the src face to prepare for the next quadrant
		Move(src, 0, 1);
	}

	QState = 0;
	Itteration = 0;
	delete[] mstack;
}

//Speed up the processing by eliminating uneeded memory swapping
void Cube::OptomizedMovement(uint* mstack, int stkptr, uint r, byte src, byte dst, int sq, int dq)
{
	byte a, b;
	
	for (uint c = 0; c < stkptr; ++c)
	{
		a = faces[dst].GetRCQ(r, mstack[c], dq);
		faces[dst].SetRCQ(r, mstack[c], dq, dst);
		b = faces[src].GetRCQ(r, mstack[c], sq - 1);
		faces[src].SetRCQ(r, mstack[c], sq - 1, a);
		faces[src].SetRCQ(r, mstack[c], sq, b);
	}

	MoveCount += (((uint64)stkptr) << 1) + 5;
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

//Solve the edges for a odd size cube
void Cube::SolveEdgesOdd()
{
	byte c0, c1;
	byte r0, r1;
	byte l0, l1;

	uint* mstack = new uint[RowSize];	//Array to store potential edge pairings
	uint mptr;
	bool found;

	//Reset the edge solve states
	memset(EdgeState, 0, 12);

	//Pre check for edges that have flipped center pieces (avoid parity problems)
	for (int de = 0; de < 12; ++de)
	{
		//Move current edge so that its on the right side front face
		SetDestinationEdge(de, true);

		//Fix issue with the right center edge piece being backwards
		GetRightEdgeColors(Mid, r0, r1);

		for (int i = 0; i < 12; i++)
		{
			c0 = EdgeColorMap[2 * i];
			c1 = EdgeColorMap[2 * i + 1];

			if (r0 == c1 && r1 == c0)
			{
				//printf("a %i\n", de);
				Move(D, Mid, 1);
				FlipRightEdge();
				Move(D, Mid, -1);
				UnFlipRightEdge();
			}
		}



		SetDestinationEdge(de, false);
	}


	//For each of the 12 edges
	for (int de = 0; de < 12; ++de)
	{
		//Move current edge so that its on the right side front face
		SetDestinationEdge(de, true);

		//Get the two colors for this edge
		c0 = EdgeColorMap[2 * de];
		c1 = EdgeColorMap[2 * de + 1];

		//Loop through all other edges 
		for (int se = 0; se < 12; ++se)
		{
			//Skip this edge if its already been solved 
			if (EdgeState[se]) continue;

			//Move edge so its on the left side of the front face
			SetSourceEdge(se, true);

			//Fix issue with the right center edge piece being backwards
			GetRightEdgeColors(Mid, r0, r1);
			if (r0 == c1 && r1 == c0 && de < 11)
			{
				//printf("x %i\n", de);
				Move(D, Mid, 1);
				FlipRightEdge();
				Move(D, Mid, -1);
				UnFlipRightEdge();
			}

			do {
				found = false;

				//Step 1a 
				//Find pieces on the left that can be moved to the right
				mptr = 0;
				for (uint r = 1; r < R1; ++r)
				{
					GetLeftEdgeColors(r, l0, l1);
					GetRightEdgeColors(r, r0, r1);

					//Piece exists on the left?
					if ((l0 == c0 && l1 == c1) || (l0 == c1 && l1 == c0))
					{
						//No piece exists on the right?
						if (!((r0 == c0 && r1 == c1) || (r0 == c1 && r1 == c0)))
						{
							if (r != Mid) mstack[mptr++] = r;
						}
					}
				}

				//Step 1b
				//Move pieces from the left to the right
				if (mptr > 0)
				{
					found = true;
					for (uint i = 0; i < mptr; ++i)
					{
						if (mstack[i] < Mid)
						{
							Move(D, mstack[i], 1);
							Move(D, R1 - mstack[i], 1);
						}
					}
					FlipRightEdge();
					for (uint i = 0; i < mptr; ++i)
					{
						if (mstack[i] < Mid) 	Move(D, mstack[i], -1);
					}
					UnFlipRightEdge();
					for (uint i = 0; i < mptr; ++i)
					{
						if (mstack[i] < Mid) 	Move(D, R1 - mstack[i], -1);
					}



					for (uint i = 0; i < mptr; ++i)
					{
						if (mstack[i] >= Mid)
						{
							Move(D, mstack[i], 1);
							Move(D, R1 - mstack[i], 1);
						}
					}
					FlipRightEdge();
					for (uint i = 0; i < mptr; ++i)
					{
						if (mstack[i] >= Mid) 	Move(D, mstack[i], -1);
					}
					UnFlipRightEdge();
					for (uint i = 0; i < mptr; ++i)
					{
						if (mstack[i] >= Mid) 	Move(D, R1 - mstack[i], -1);
					}

				}

				//Step 2a
				//Find pieces on the left that can be moved to the right
				mptr = 0;
				for (uint r = 1; r < R1; ++r)
				{
					GetLeftEdgeColors(r, l0, l1);
					GetRightEdgeColors(r, r0, r1);

					//Piece exists on the left?
					if ((l0 == c0 && l1 == c1) || (l0 == c1 && l1 == c0))
					{
						//Also a piece exists on the right?
						if (((r0 == c0 && r1 == c1) || (r0 == c1 && r1 == c0)))
						{
							if (r != Mid) mstack[mptr++] = r;
						}
					}
				}

				//Step 2b
				//Move pieces from the left to the right
				if (mptr > 0)
				{
					found = true;
					FlipRightEdge();
					for (uint i = 0; i < mptr; ++i) { Move(D, mstack[i], 1); }
					UnFlipRightEdge();
					for (uint i = 0; i < mptr; ++i) { Move(D, mstack[i], -1); }
				}

				//Step 3
				//Move center edge pieces from left to right
				GetLeftEdgeColors(Mid, l0, l1);
				if (l0 == c0 && l1 == c1)
				{
					FlipLeftEdge();
					MoveCenterEdge(false);
					UnFlipLeftEdge();
				}

				if (l0 == c1 && l1 == c0) MoveCenterEdge(false);

			} while (found);  //Repeat the process if more additional pieces are brought into the edge after moving

			//Move edge back to its original location
			SetSourceEdge(se, false);
		}

		//Fix remaining parity issues for this edge
		for (uint r = 1; r < Mid; ++r)
		{
			GetRightEdgeColors(r, r0, r1);
			if (r0 == c1 && r1 == c0) FixParity(r);
		}

		//Move edge back to the correct face and orientation 
		SetDestinationEdge(de, false);

		//This edge is now solved
		EdgeState[de] = true;
	}

	delete[] mstack;
}

//Solve the edges for a even size cube
void Cube::SolveEdgesEven()
{
	byte c0, c1;
	byte r0, r1;
	byte l0, l1;

	uint* mstack = new uint[RowSize];	//Array to store potential edge pairings
	uint mptr;
	bool found;

	//Reset the edge solve states
	memset(EdgeState, 0, 12);

	//For each of the 12 edges
	for (int de = 0; de < 12; ++de)
	{
		//Move current edge so that its on the right side front face
		SetDestinationEdge(de, true);

		//Get the two colors for this edge
		c0 = EdgeColorMap[2 * de];
		c1 = EdgeColorMap[2 * de + 1];

		//Loop through all other edges 
		for (int se = 0; se < 12; ++se)
		{
			//Skip this edge if its already been solved 
			if (EdgeState[se]) continue;

			//Move edge so its on the left side of the front face
			SetSourceEdge(se, true);

			do {
				found = false;

				//Step 1a 
				//Find pieces on the left that can be moved to the right
				mptr = 0;
				for (uint r = 1; r < R1; ++r)
				{
					GetLeftEdgeColors(r, l0, l1);
					GetRightEdgeColors(r, r0, r1);

					//Piece exists on the left?
					if ((l0 == c0 && l1 == c1) || (l0 == c1 && l1 == c0))
					{
						//No piece exists on the right?
						if (!((r0 == c0 && r1 == c1) || (r0 == c1 && r1 == c0)))
						{
							mstack[mptr++] = r;
						}
					}
				}

				//Step 1b
				//Move pieces from the left to the right
				if (mptr > 0)
				{
					found = true;
					for (uint i = 0; i < mptr; ++i)
					{
						if (mstack[i] < Mid)
						{
							Move(D, mstack[i], 1);
							Move(D, R1 - mstack[i], 1);
						}
					}
					FlipRightEdge();
					for (uint i = 0; i < mptr; ++i)
					{
						if (mstack[i] < Mid) 	Move(D, mstack[i], -1);
					}
					UnFlipRightEdge();
					for (uint i = 0; i < mptr; ++i)
					{
						if (mstack[i] < Mid) 	Move(D, R1 - mstack[i], -1);
					}



					for (uint i = 0; i < mptr; ++i)
					{
						if (mstack[i] >= Mid)
						{
							Move(D, mstack[i], 1);
							Move(D, R1 - mstack[i], 1);
						}
					}
					FlipRightEdge();
					for (uint i = 0; i < mptr; ++i)
					{
						if (mstack[i] >= Mid) 	Move(D, mstack[i], -1);
					}
					UnFlipRightEdge();
					for (uint i = 0; i < mptr; ++i)
					{
						if (mstack[i] >= Mid) 	Move(D, R1 - mstack[i], -1);
					}

				}

				//Step 2a
				//Find pieces on the left that can be moved to the right
				mptr = 0;
				for (uint r = 1; r < R1; ++r)
				{
					GetLeftEdgeColors(r, l0, l1);
					GetRightEdgeColors(r, r0, r1);

					//Piece exists on the left?
					if ((l0 == c0 && l1 == c1) || (l0 == c1 && l1 == c0))
					{
						//Also a piece exists on the right?
						if (((r0 == c0 && r1 == c1) || (r0 == c1 && r1 == c0)))
						{
							mstack[mptr++] = r;
						}
					}
				}

				//Step 2b
				//Move pieces from the left to the right
				if (mptr > 0 && mptr < RowSize)
				{
					found = true;
					FlipRightEdge();
					for (uint i = 0; i < mptr; i++) { Move(D, mstack[i], 1); }
					UnFlipRightEdge();
					for (uint i = 0; i < mptr; i++) { Move(D, mstack[i], -1); }
				}

			} while (found);  //Repeat the process if more additional pieces are brought into the edge after moving

			//Move edge back to its original location
			SetSourceEdge(se, false);
		}

		//Fix remaining parity issues for this edge
		for (uint r = 1; r < Mid; ++r)
		{
			GetRightEdgeColors(r, r0, r1);
			if (r0 == c1 && r1 == c0) FixParity(r);
		}

		//Move edge back to the correct face and orientation 
		SetDestinationEdge(de, false);

		//this edge is now solved
		EdgeState[de] = true;
	}

	delete[] mstack;
}

//Flips the F-R edge
void Cube::FlipRightEdge()
{
	Move(R, 0, 1);
	UpdateEdgeRotation(R, 1);
	Move(U, 0, 1);
	UpdateEdgeRotation(U, 1);
	Move(R, 0, -1);
	UpdateEdgeRotation(R, -1);
	Move(F, 0, 1);
	UpdateEdgeRotation(F, 1);
	Move(R, 0, -1);
	UpdateEdgeRotation(R, -1);
	Move(F, 0, -1);
	UpdateEdgeRotation(F, -1);
	Move(R, 0, 1);
	UpdateEdgeRotation(R, 1);
}

//Un-Flips the F-R edge 
void Cube::UnFlipRightEdge()
{
	Move(R, 0, -1);
	UpdateEdgeRotation(R, -1);
	Move(F, 0, 1);
	UpdateEdgeRotation(F, 1);
	Move(R, 0, 1);
	UpdateEdgeRotation(R, 1);
	Move(F, 0, -1);
	UpdateEdgeRotation(F, -1);
	Move(R, 0, 1);
	UpdateEdgeRotation(R, 1);
	Move(U, 0, -1);
	UpdateEdgeRotation(U, -1);
	Move(R, 0, -1);
	UpdateEdgeRotation(R, -1);
}

//Flips the F-L edge
void Cube::FlipLeftEdge()
{
	Move(L, 0, -1);
	UpdateEdgeRotation(L, -1);
	Move(U, 0, -1);
	UpdateEdgeRotation(U, -1);
	Move(L, 0, 1);
	UpdateEdgeRotation(L, 1);
	Move(F, 0, -1);
	UpdateEdgeRotation(F, -1);
	Move(L, 0, 1);
	UpdateEdgeRotation(L, 1);
	Move(F, 0, 1);
	UpdateEdgeRotation(F, 1);
	Move(L, 0, -1);
	UpdateEdgeRotation(L, -1);
}

//Un Flips the F-L edge
void Cube::UnFlipLeftEdge()
{
	Move(L, 0, 1);
	UpdateEdgeRotation(L, 1);
	Move(F, 0, -1);
	UpdateEdgeRotation(F, -1);
	Move(L, 0, -1);
	UpdateEdgeRotation(L, -1);
	Move(F, 0, 1);
	UpdateEdgeRotation(F, 1);
	Move(L, 0, -1);
	UpdateEdgeRotation(L, -1);
	Move(U, 0, 1);
	UpdateEdgeRotation(U, 1);
	Move(L, 0, 1);
	UpdateEdgeRotation(L, 1);
}

//Move the front face center edge from the left side to the right side
void Cube::MoveCenterEdge(bool flipped)
{

	//Find unsolved center edge on the U face 0,1,2,3
	int q = -1;

	if (!EdgeState[6]) q = 1;
	if (!EdgeState[2]) q = 2;
	if (!EdgeState[5]) q = 3;
	if (!EdgeState[10]) q = 0;

	if (q >= 0)
	{
		if (q > 0)	Move(U, 0, -q);

		Move(L, Mid, 2);
		Move(F, 0, 1);
		Move(L, Mid, -1);
		Move(F, 0, 2);
		Move(L, Mid, 1);
		Move(F, 0, 1);
		Move(L, Mid, 2);

		if (q > 0)	Move(U, 0, q);

		return;
	}

	//Find unsolved center edge on the D face 0,1,2,3
	q = -1;

	if (!EdgeState[7]) q = 1;
	if (!EdgeState[0]) q = 2;
	if (!EdgeState[4]) q = 3;
	if (!EdgeState[8]) q = 0;

	if (q >= 0)
	{
		if (q > 0)	Move(D, 0, q);

		Move(L, Mid, 2);
		Move(F, 0, -1);
		Move(L, Mid, 1);
		Move(F, 0, 2);
		Move(L, Mid, -1);
		Move(F, 0, -1);
		Move(L, Mid, 2);

		if (q > 0)	Move(D, 0, -q);

		return;
	}

	if (!flipped)
	{
		Move(B, 0, 1);
		UpdateEdgeRotation(B, 1);
		MoveCenterEdge(true);
		Move(B, 0, -1);
		UpdateEdgeRotation(B, -1);
	}
}

//Get the values of an edge piece on the left side of the F face
void Cube::GetLeftEdgeColors(int row, byte& l0, byte& l1)
{
	l0 = faces[L].GetRC(row, R1);
	l1 = faces[F].GetRC(row, 0);
}

//Get the values of an edge piece on the right side of the F face
void Cube::GetRightEdgeColors(int row, byte& r0, byte& r1)
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
			Move(D, 0, -1); UpdateEdgeRotation(D, -1);
			Move(R, 0, 1);  UpdateEdgeRotation(R, 1);
			break;
		case 1: //B-L
			Move(B, 0, 2);  UpdateEdgeRotation(B, 2);
			Move(R, 0, 2);  UpdateEdgeRotation(R, 2);
			break;
		case 2://B-U
			Move(B, 0, -1);	 UpdateEdgeRotation(B, -1);
			Move(R, 0, 2); UpdateEdgeRotation(R, 2);
			break;
		case 3://B-R
			Move(R, 0, 2); UpdateEdgeRotation(R, 2);
			break;
		case 4://D-R
			Move(R, 0, 1); UpdateEdgeRotation(R, 1);
			break;
		case 5://U-R
			Move(R, 0, -1); UpdateEdgeRotation(R, -1);
			break;
		case 6://U-L
			Move(U, 0, 2); UpdateEdgeRotation(U, 2);
			Move(R, 0, -1); UpdateEdgeRotation(R, -1);
			break;
		case 7://D-L
			Move(D, 0, 2); UpdateEdgeRotation(D, 2);
			Move(R, 0, 1); UpdateEdgeRotation(R, 1);
			break;
		case 8://F-D
			Move(F, 0, -1); UpdateEdgeRotation(F, -1);
			break;
		case 9://F-R
			break;
		case 10://F-U
			Move(F, 0, 1); UpdateEdgeRotation(F, 1);
			break;
		case 11://F-L
			Move(F, 0, 2); UpdateEdgeRotation(F, 2);
			break;
		}
		return;
	}

	if (!set)
	{
		switch (edge)
		{
		case 0: //D-B
			Move(R, 0, -1); UpdateEdgeRotation(R, -1);
			Move(D, 0, 1); UpdateEdgeRotation(D, 1);
			break;
		case 1: //B-L
			Move(R, 0, 2); UpdateEdgeRotation(R, 2);
			Move(B, 0, 2); UpdateEdgeRotation(B, 2);
			break;
		case 2://B-U
			Move(R, 0, 2); UpdateEdgeRotation(R, 2);
			Move(B, 0, 1); UpdateEdgeRotation(B, 1);
			break;
		case 3://B-R
			Move(R, 0, 2); UpdateEdgeRotation(R, 2);
			break;
		case 4://D-R
			Move(R, 0, -1); UpdateEdgeRotation(R, -1);
			break;
		case 5://U-R
			Move(R, 0, 1); UpdateEdgeRotation(R, 1);
			break;
		case 6://U-L 
			Move(R, 0, 1); UpdateEdgeRotation(R, 1);
			Move(U, 0, 2); UpdateEdgeRotation(U, 2);
			break;
		case 7://D-L
			Move(R, 0, -1); UpdateEdgeRotation(R, -1);
			Move(D, 0, 2); UpdateEdgeRotation(D, 2);
			break;
		case 8://F-D
			Move(F, 0, 1); UpdateEdgeRotation(F, 1);
			break;
		case 9://F-R
			break;
		case 10://F-U
			Move(F, 0, -1); UpdateEdgeRotation(F, -1);
			break;
		case 11://F-L
			Move(F, 0, 2); UpdateEdgeRotation(F, 2);
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
			UpdateEdgeRotation(D, 1);
			Move(L, 0, -1);
			UpdateEdgeRotation(L, -1);
			break;
		case 1: //B-L
			Move(L, 0, 2);
			UpdateEdgeRotation(L, 2);
			break;
		case 2://B-U
			Move(U, 0, -1);
			UpdateEdgeRotation(U, -1);
			Move(L, 0, 1);
			UpdateEdgeRotation(L, 1);
			break;
		case 3://B-R
			Move(B, 0, 2);
			UpdateEdgeRotation(B, 2);
			Move(L, 0, 2);
			UpdateEdgeRotation(L, 2);
			break;
		case 4://D-R
			Move(D, 0, 2);
			UpdateEdgeRotation(D, 2);
			Move(L, 0, -1);
			UpdateEdgeRotation(L, -1);
			break;
		case 5://U-R
			Move(U, 0, 2);
			UpdateEdgeRotation(U, 2);
			Move(L, 0, 1);
			UpdateEdgeRotation(L, 1);
			break;
		case 6://U-L
			Move(L, 0, 1);
			UpdateEdgeRotation(L, 1);
			break;
		case 7://D-L
			Move(L, 0, -1);
			UpdateEdgeRotation(L, -1);
			break;
		case 8://F-D
			Move(D, 0, -1);
			UpdateEdgeRotation(D, -1);
			Move(L, 0, -1);
			UpdateEdgeRotation(L, -1);
			break;
		case 9://F-R 
			break;
		case 10://F-U
			Move(U, 0, 1);
			UpdateEdgeRotation(U, 1);
			Move(L, 0, 1);
			UpdateEdgeRotation(L, 1);
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
			Move(L, 0, 1); UpdateEdgeRotation(L, 1);
			Move(D, 0, -1); UpdateEdgeRotation(D, -1);
			break;
		case 1: //B-L 
			Move(L, 0, 2); UpdateEdgeRotation(L, 2);
			break;
		case 2://B-U
			Move(L, 0, -1); UpdateEdgeRotation(L, -1);
			Move(U, 0, 1); UpdateEdgeRotation(U, 1);
			break;
		case 3://B-R
			Move(L, 0, 2); UpdateEdgeRotation(L, 2);
			Move(B, 0, 2); UpdateEdgeRotation(B, 2);
			break;
		case 4://D-R
			Move(L, 0, 1); UpdateEdgeRotation(L, 1);
			Move(D, 0, 2); UpdateEdgeRotation(D, 2);
			break;
		case 5://U-R
			Move(L, 0, -1); UpdateEdgeRotation(L, -1);
			Move(U, 0, 2); UpdateEdgeRotation(U, 2);
			break;
		case 6://U-L
			Move(L, 0, -1); UpdateEdgeRotation(L, -1);
			break;
		case 7://D-L
			Move(L, 0, 1); UpdateEdgeRotation(L, 1);
			break;
		case 8://F-D
			Move(L, 0, 1); UpdateEdgeRotation(L, 1);
			Move(D, 0, 1); UpdateEdgeRotation(D, 1);
			break;
		case 9://F-R
			break;
		case 10://F-U
			Move(L, 0, -1); UpdateEdgeRotation(L, -1);
			Move(U, 0, -1); UpdateEdgeRotation(U, -1);
			break;
		case 11://F-L
			break;
		}
	}
}

//Keep track of the location of each edge as they are moved around 
//Prevents accidentally disrupting a solved edge
void Cube::UpdateEdgeRotation(byte faceid, int steps)
{
	const byte* e = EdgeRotMap[faceid];

	if (steps > 0)
	{
		for (int i = 0; i < steps; ++i)
		{
			bool tmp = EdgeState[e[3]];

			EdgeState[e[3]] = EdgeState[e[2]];
			EdgeState[e[2]] = EdgeState[e[1]];
			EdgeState[e[1]] = EdgeState[e[0]];
			EdgeState[e[0]] = tmp;
		}
	}

	if (steps < 0)
	{
		for (int i = 0; i < abs(steps); ++i)
		{
			bool tmp = EdgeState[e[0]];
			EdgeState[e[0]] = EdgeState[e[1]];
			EdgeState[e[1]] = EdgeState[e[2]];
			EdgeState[e[2]] = EdgeState[e[3]];
			EdgeState[e[3]] = tmp;
		}
	}

}

#pragma endregion

#pragma region Corners

void Cube::SolveCorners()
{
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

	Stage++;
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
void Cube::GetCorner(int cr, byte& c0, byte& c1, byte& c2)
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
	if (!SaveEnabled) return;

	printf("Saving cube state\n");

	//Keep track of total hours of processing
	Hours += CurrentProcessDuration();

	printf("Total duration = %.3f hours\n", Hours);

	std::ofstream out("cubestate.bin", std::ios::out | std::ios::binary);
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

	std::ifstream in("cubestate.bin", std::ios::out | std::ios::binary);
	in.read((char*)this, sizeof(Cube));
	in.close();

	faces = new Face[6];
	for (byte b = 0; b < 6; ++b)
	{
		faces[b].LoadFaceState(b);
	}

	printf("Done loading cube\n");

}

//Calculate the number of physical pieces 
uint64 Cube::PieceCount()
{
	if (RowSize <= 1) return 1;
	uint64 result = (uint64)RowSize; //size of a face
	result *= result;
	result *= 6;	// 6 faces
	result -= 16;	// Corner pieces were counted 3 times
	result -= ((uint64)RowSize - (uint64)2) * 12; //Edge pieces were counted 2 times
	return result;
}

void Cube::PrintStats()
{
	printf("\n");
	printf("Cube Size : %i\n", RowSize);
	printf("Total Tiles : %llu\n", ((uint64)RowSize * (uint64)RowSize * 6));
	printf("Total Pieces : %llu\n", PieceCount());
	printf("Total Moves : %llu\n", MoveCount);
	printf("Moves per piece : %f\n", (double)MoveCount / (double)PieceCount());

	if (Hours < 1.0)
	{
		double Minutes = Hours * 60.0;
		if (Minutes < 2.0)
		{
			double Seconds = Minutes * 60.0;
			printf("Total Seconds : %.7f\n", Seconds);
		}
		else
		{
			printf("Total Minutes : %.7f\n", Minutes);
		}
	}
	else
	{
		printf("Total Hours : %.7f\n", Hours);
	}


	if (SaveEnabled)
	{
		printf("Stage : %i\n", Stage);
		printf("Quadrant : %i\n", QState);
		printf("Itteration : %i\n\n", Itteration);
	}

	if (IsCubeSolved())
	{
		printf("Cube is solved!\n");
	}
	else
	{
		printf("Cube is NOT solved!\n");
	}
	printf("\n");
}

//Returns true if all faces are in the solved state
bool Cube::IsCubeSolved()
{
	for (int i = 0; i < 6; ++i)
	{
		if (!faces[i].IsFaceSolved()) return false;
	}

	return true;
}

#pragma endregion

void Cube::Reset()
{
	MoveCount = 0;
	MoveCounter = 0;
	FrameNumber = 0;
	Hours = 0.0;
	memset(EdgeState, 0, 12);
	Stage = 0;			//stage of the solve (used for recovering from a restart)
	QState = 0;			//current quadrant being solved
	Itteration = 0;		//itteration of the current stage 
	ProcessStartTime = std::chrono::high_resolution_clock::now(); //Reset process timer
}

Cube::Cube(int size)
{
	IsEven = false;
	Mid = 0;
	R1 = 0;
	RowSize = 0;
	faces = nullptr;
	SaveEnabled = false;
	Reset();
	Initalize(size);
}

Cube::Cube()
{
	//use this constructor when reloading the cube state
	IsEven = false;
	Mid = 0;
	R1 = 0;
	RowSize = 0;
	faces = nullptr;
	SaveEnabled = false;
	Reset();
}

Cube::~Cube()
{
	//Cleanup
	if (faces != nullptr) delete[] faces;
}
