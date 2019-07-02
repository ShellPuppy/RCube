#pragma once

#include <chrono>
#include "Face.h"

typedef unsigned long long uint64;

class Cube
{
	const byte F = 0x00;	//Front
	const byte R = 0x01;	//Right
	const byte B = 0x02;	//Back
	const byte L = 0x03;	//Left
	const byte U = 0x04;	//Up
	const byte D = 0x05;	//Down

	uint R1;		//Rowsize - 1
	uint Mid;		//Midpoint 
	bool IsEven;	//Is this an even layered cube

	int Stage;		//stage of the solve (used for recovering from a restart)
	int QState;		//current quadrant being solved
	uint Itteration;//itteration of the current stage 
	bool EdgeState[12];//Current solve state of each egde

	inline void RotateX(uint c, int step);
	inline void RotateY(uint c, int step);
	inline void RotateZ(uint c, int step);
	
	const static byte cmap[30][6];		//Parameters for center commutators 
	const static byte EdgeColorMap[24];	//Pairs of edge colors
	const static byte corners[8][3];	//Corner color definitions
	const static byte EdgeRotMap[6][4];	//Edge rotation map 
public:

#pragma region Stats and Info

	std::chrono::high_resolution_clock::time_point ProcessStartTime;

	uint64 MoveCount;	//Total number of moves made ( rotations of 2 or 3 are counted as 1 )
	double Hours;		//Total processing hours

	inline double CurrentProcessDuration();

	uint64 PieceCount();	//Number of physical pieces in a cube

#pragma endregion

	uint RowSize;	//size of cube side
	
	Face *faces;	//array of 6 faces

	bool SaveEnabled;	//allow saving the cube state

	void Initalize(uint rsize);

	void Cleanup();

	inline void Move(byte f, int d, int q);

	void Reset();

	void Scramble(int);

	void Solve();	

#pragma region Solving Centers
	
	void SolveCenters();

	void AlignTrueCenters();

	bool IsOpposite(byte src, byte dst);

	void PushCenterPieces(byte src, byte dst, byte color);

	void OptomizeTopCenter();

	int FindCommutatorMap(byte src, byte dst);

#pragma endregion

#pragma region Solving Edges

	void SolveEdgesOdd();

	void SolveEdgesEven();

	void FlipRightEdge();

	void UnFlipRightEdge();

	void FlipLeftEdge();

	void UnFlipLeftEdge();

	void MoveCenterEdge(bool flipped);

	void GetLeftEdgeColors(int row, byte & l1, byte & l2);

	void GetRightEdgeColors(int row, byte & l1, byte & l2);

	void FixParity(int row);

	void SetDestinationEdge(int e, bool set);
	
	void SetSourceEdge(int edge, bool set);

	void UpdateEdgeRotation(byte faceid, int steps);

#pragma endregion	

#pragma region Solving Corners
	void SolveCorners();

	void FlipCorners();

	void GetCorner(int cr, byte & c0, byte & c1, byte & c2);

	bool IsCorner(int cr, byte c0, byte c1, byte c2);

	int FindCorner(int cr);
#pragma endregion

#pragma region Cube State

	void SaveCubeState();
	void LoadCubeState();
	void PrintStats();

	bool IsCubeSolved();

#pragma endregion

	Cube(int);
	Cube();
	~Cube();
};

//Rotates a Face at Depth d by q steps
inline void Cube::Move(const byte face, const  int depth, const int q)
{
	//Keep track of move counts
	MoveCount++;

	switch (face)
	{
	case 0: //F
		RotateZ(depth, -q);
		return;
	case 1: //R
		RotateX(R1 - depth, q);
		return;
	case 2: //B
		RotateZ(R1 - depth, q);
		return;
	case 3: //L
		RotateX(depth, -q);
		return;
	case 4: //U
		RotateY(R1 - depth, -q);
		return;
	case 5: //D
		RotateY(depth, q);
		return;
	}
}

//Rotates a slice in the Y-Z plane (Left and Right faces) by (1,2,3,-1,-2,-3)
inline void Cube::RotateX(uint index, int step)
{
	if (index == 0)  faces[3].RotatefaceCW(-step);
	if (index == R1) faces[1].RotatefaceCW(step);

	byte buffer[4];
	for (uint i = 0; i < RowSize; ++i)
	{
		buffer[0] = faces[0].GetRC(i, index);
		buffer[1] = faces[4].GetRC(i, index);
		buffer[2] = faces[2].GetRC(R1 - i, R1 - index);
		buffer[3] = faces[5].GetRC(i, index);

		faces[0].SetRC(i, index,			buffer[(0 - (step & 3)) & 3]);
		faces[4].SetRC(i, index,			buffer[(1 - (step & 3)) & 3]);
		faces[2].SetRC(R1 - i, R1 - index,	buffer[(2 - (step & 3)) & 3]);
		faces[5].SetRC(i, index,			buffer[(3 - (step & 3)) & 3]);
	}
}

//Rotates a slice in the X-Y plane  (Top and Bottom faces) by (1,2,3,-1,-2,-3)
inline void Cube::RotateY(uint index, int step)
{
	if (index == 0)  faces[5].RotatefaceCW(step);
	if (index == R1) faces[4].RotatefaceCW(-step);

	byte buffer[4];
	for (uint i = 0; i < RowSize; ++i)
	{
		buffer[0] = faces[0].GetRC(index, i);
		buffer[1] = faces[1].GetRC(index, i);
		buffer[2] = faces[2].GetRC(index, i);
		buffer[3] = faces[3].GetRC(index, i);

		faces[0].SetRC(index, i, buffer[(0 - (step & 3)) & 3]);
		faces[1].SetRC(index, i, buffer[(1 - (step & 3)) & 3]);
		faces[2].SetRC(index, i, buffer[(2 - (step & 3)) & 3]);
		faces[3].SetRC(index, i, buffer[(3 - (step & 3)) & 3]);
	}
}

//Rotates a slice in the X-Z plane (Front and Back) by (1,2,3,-1,-2,-3)
inline void Cube::RotateZ(uint index, int step)
{
	if (index == 0)  faces[0].RotatefaceCW(-step);
	if (index == R1) faces[2].RotatefaceCW(step);

	int b1 = (0 - (step & 3)) & 3;
	byte buffer[4];

	for (uint i = 0; i < RowSize; ++i)
	{
		buffer[0] = faces[1].GetRC(i, index);
		buffer[1] = faces[4].GetRC(index, R1 - i);
		buffer[2] = faces[3].GetRC(R1 - i, R1 - index);
		buffer[3] = faces[5].GetRC(R1 - index, i);

		faces[1].SetRC(i, index, buffer[b1]);
		faces[4].SetRC(index, R1 - i, buffer[(b1 + 1) & 3]);
		faces[3].SetRC(R1 - i, R1 - index, buffer[(b1 + 2) & 3]);
		faces[5].SetRC(R1 - index, i, buffer[(b1 + 3) & 3]);
	}
}

//Returns the total number of hours since solve was started or restarted
inline double Cube::CurrentProcessDuration()
{

	std::chrono::duration<double, std::ratio<3600>> ProcessTime = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - ProcessStartTime);
	
	return ProcessTime.count();

}
