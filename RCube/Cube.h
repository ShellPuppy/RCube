#pragma once

#include <chrono>
#include "Face.h"
#include "CubeViewer.h"
#include <string>

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

	int FrameNumber;	//Number to use when exporting a frame
	int MoveCounter;	//Number of moves since the last frame export
	int MovesPerFrame;	//Number of moves per exported frame


#pragma endregion

	uint RowSize;	//size of cube side
	
	Face *faces;	//array of 6 faces

	bool SaveEnabled;	//allow saving the cube state

	void Initalize(uint rsize);

	void Cleanup();

	inline void ExportFrame();

	inline void Move(byte f, int d, int q);

	void Reset();

	void Scramble(int);

	void Solve();	

#pragma region Solving Centers
	
	void SolveCenters();

	void AlignTrueCenters();

	bool IsOpposite(byte src, byte dst);

	void PushCenterPieces(byte src, byte dst, byte color);

	void OptomizedMovement(uint* mstack, int stkptr, uint r, byte src, byte dst, int sq, int dq);

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

inline void Cube::ExportFrame()
{
	std::string name = std::to_string(FrameNumber);
	for (byte i = 0; i < 6; i++)
	{
		std::string filename = "face" + std::to_string(i) + "//F" + std::to_string(i) + "_" + std::string(6 - name.length(), '0') + name + ".png";
		std::cout << filename << std::endl;
		CubeViewer::ExportFaceDiagram(faces[i], filename, 1000, false);
	}
	FrameNumber++;
}

//Rotates a Face at Depth d by q steps
inline void Cube::Move(const byte face, const  int depth, const int q)
{
	//Keep track of move counts
	MoveCount++;
	
	MoveCounter++;
	
	if (MovesPerFrame > 0 && MoveCounter >= MovesPerFrame)
	{
		MoveCounter = 0;
		//ExportFrame();
	}

	switch (face)
	{
	case 0: //F
		RotateZ(depth, q);
		return;
	case 1: //R
		RotateX(R1 - depth, q);
		return;
	case 2: //B
		RotateZ(R1 - depth, -q);
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
inline void Cube::RotateX(const uint index, const int step)
{
	if (index == 0)  faces[3].RotatefaceCW(-step);
	if (index == R1) faces[1].RotatefaceCW(step);

	byte b[4];
	byte* f0, * f4, * f2, * f5;
	uint p0, p4, p2, p5;
	int d0, d4, d2, d5;
	int i0, i1, i2, i3;

	f0 = faces[0].data;
	f4 = faces[4].data;
	f2 = faces[2].data;
	f5 = faces[5].data;

	p0 = faces[0].GetPos(0, index);
	p4 = faces[4].GetPos(0, index);
	p2 = faces[2].GetPos(R1, R1 - index);
	p5 = faces[5].GetPos(0, index);

	d0 = faces[0].GetDelta(3);
	d4 = faces[4].GetDelta(3);
	d2 = faces[2].GetDelta(1);
	d5 = faces[5].GetDelta(3);

	i0 = (0 - (step & 3)) & 3;
	i1 = (1 - (step & 3)) & 3;
	i2 = (2 - (step & 3)) & 3;
	i3 = (3 - (step & 3)) & 3;

	uint i = 0;

	while (i < RowSize)
	{
		b[0] = f0[p0];
		b[1] = f4[p4];
		b[2] = f2[p2];
		b[3] = f5[p5];

		f0[p0] = b[i0];
		f4[p4] = b[i1];
		f2[p2] = b[i2];
		f5[p5] = b[i3];

		p0 += d0;
		p4 += d4;
		p2 += d2;
		p5 += d5;

		i++;
	}

}

//Rotates a slice in the X-Y plane  (Top and Bottom faces) by (1,2,3,-1,-2,-3)
inline void Cube::RotateY(const uint index, const int step)
{
	if (index == 0)  faces[5].RotatefaceCW(step);
	if (index == R1) faces[4].RotatefaceCW(-step);

	byte b[4];
	byte* f0, * f1, * f2, * f3;
	uint p0, p1, p2, p3;
	int d0, d1, d2, d3;
	int i0, i1, i2, i3;

	f0 = faces[0].data;
	f1 = faces[1].data;
	f2 = faces[2].data;
	f3 = faces[3].data;

	p0 = faces[0].GetPos(index, 0);
	p1 = faces[1].GetPos(index, 0);
	p2 = faces[2].GetPos(index, 0);
	p3 = faces[3].GetPos(index, 0);

	d0 = faces[0].GetDelta(0);
	d1 = faces[1].GetDelta(0);
	d2 = faces[2].GetDelta(0);
	d3 = faces[3].GetDelta(0);

	i0 = (0 - (step & 3)) & 3;
	i1 = (1 - (step & 3)) & 3;
	i2 = (2 - (step & 3)) & 3;
	i3 = (3 - (step & 3)) & 3;

	uint i = 0;

	while (i < RowSize)
	{
		b[0] = f0[p0];
		b[1] = f1[p1];
		b[2] = f2[p2];
		b[3] = f3[p3];

		f0[p0] = b[i0];
		f1[p1] = b[i1];
		f2[p2] = b[i2];
		f3[p3] = b[i3];

		p0 += d0;
		p1 += d1;
		p2 += d2;
		p3 += d3;

		i++;
	}

}

//Rotates a slice in the X-Z plane (Front and Back) by (1,2,3,-1,-2,-3)
inline void Cube::RotateZ(const uint index, const int step)
{
	if (index == 0)  faces[0].RotatefaceCW(step);
	if (index == R1) faces[2].RotatefaceCW(-step);

	byte b[4];
	byte* f1, * f5, * f3, * f4;
	uint p1, p5, p3, p4;
	int d1, d5, d3, d4;
	int i0, i1, i2, i3;

	f1 = faces[1].data;
	f5 = faces[5].data;
	f3 = faces[3].data;
	f4 = faces[4].data;

	p1 = faces[1].GetPos(0, index);
	p5 = faces[5].GetPos(R1 - index, 0);
	p3 = faces[3].GetPos(R1, R1 - index);
	p4 = faces[4].GetPos(index, R1);

	d1 = faces[1].GetDelta(3);
	d5 = faces[5].GetDelta(0);
	d3 = faces[3].GetDelta(1);
	d4 = faces[4].GetDelta(2);

	i0 = (0 - (step & 3)) & 3;
	i1 = (1 - (step & 3)) & 3;
	i2 = (2 - (step & 3)) & 3;
	i3 = (3 - (step & 3)) & 3;

	uint i = 0;

	while (i < RowSize)
	{
		b[0] = f1[p1];
		b[1] = f5[p5];
		b[2] = f3[p3];
		b[3] = f4[p4];

		f1[p1] = b[i0];
		f5[p5] = b[i1];
		f3[p3] = b[i2];
		f4[p4] = b[i3];

		p1 += d1;
		p5 += d5;
		p3 += d3;
		p4 += d4;

		i++;
	}

}


//Returns the total number of hours since solve was started or restarted
inline double Cube::CurrentProcessDuration()
{

	std::chrono::duration<double, std::ratio<3600>> ProcessTime = std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - ProcessStartTime);
	
	return ProcessTime.count();

}
