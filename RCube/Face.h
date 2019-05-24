#pragma once
#include <iostream>
#include <cstdio>

typedef unsigned char byte;
typedef unsigned int  uint;
typedef unsigned long long uint64;

class Face
{
	
	uint R1;				// RowSize - 1
	uint MemRowSize;		// Memory row length (must be a power of 2) 32,64,128...65536
	uint BS;				// bit shift = log2(memsize)
	uint64 DataSize;			// size of data array in bytes = MemRowSize * MemRowSize
	byte *data;				// face data
	int orientation;		// Virtual orientation of this face [0,1,2,3] - 90 degree clockwise rotations
	uint PieceCount[6];		// Keep track of the number of each piece on this face (used for validating the integrity of the cube)

public:
	uint RowSize;			// cube row length 

	byte id; //this face number (0-5)

	inline void RotatefaceCW(int r);

	inline const byte GetRC(const uint r, const uint c) const;

	inline const byte GetRCQ(const uint r, const uint c, int q) const;

	inline const byte GetCoordR(const uint r, const uint c, int q) const;

	inline const byte GetCoordC(const uint r, const uint c, int q) const;

	inline void SetRC(const uint r, const uint c, const byte v);

	inline void SetRCQ(const int r, const int c, const byte v, int q);

	void Initialize(byte index, uint rsize, uint msize);

	bool VerifyCounts();

	void SaveFaceState();

	void LoadFaceState(byte faceid);

	void Paint(byte color);

	uint Count(byte color);
	
	void GetCounts(uint *);

	Face();

	~Face();
};

//Virtually rotates this face by q * 90 degrees (this does not affect other cube faces!)
inline void Face::RotatefaceCW(int q)
{
	orientation = (orientation + q) & 3;
}

//Gets the value of this face at coordinates r = row, c = column
inline const byte Face::GetRC(const uint r, const uint c) const
{
	switch (orientation)
	{
	case 0:
		return data[(r << BS) + c];
	case 1:
		return data[(c << BS) + (R1 - r) ];
	case 2:
		return data[(((R1 - r) << BS) + (R1 - c) )];
	case 3:
		return data[((R1 - c) << BS) + r ];
	default:
		return 0;
	}
}

//Gets the value of this face at coordinates r,c + an additional rotation q (q=90 cw turn)
inline const byte Face::GetRCQ(const uint r, const uint c, int q) const
{
	q = (orientation - q) & 3;

	switch (q)
	{
	case 0:
		return data[(r << BS) + c];
	case 1:
		return data[(c << BS) + (R1 - r)];
	case 2:
		return data[(((R1 - r) << BS) + (R1 - c))];
	case 3:
		return data[((R1 - c) << BS) + r];
	}

	return 0;

}

//Returns the Row number of coordinate (r,c) rotated by q
inline const byte Face::GetCoordR(const uint r, const uint c, int q) const
{
	switch ((-q)&3)
	{
	case 0:
		return r;
	case 1:
		return c;
	case 2:
		return R1 - r;
	case 3:
		return R1 - c;
	default:
		return r;
	}
}

//Returns the Column number of coordinate (r,c) rotated by q
inline const byte Face::GetCoordC(const uint r, const uint c, int q) const
{
	switch ((-q) & 3)
	{
	case 0:
		return c;
	case 1:
		return (R1 - r);
	case 2:
		return (R1 - c);
	case 3:
		return r;
	default:
		return c;
	}
}

//Sets the value of this face at coordinates r = row, c = column
inline void Face::SetRC(const uint r, const uint c, const byte v)
{
	switch (orientation)
	{
	case 0:
		data[(r << BS) + c] = v;
		return;
	case 1:
		data[(c << BS) + (R1 - r)] = v;
		 return;
	case 2:
		data[(((R1 - r) << BS) + (R1 - c))] = v;
		 return;
	case 3:
		data[((R1 - c) << BS) + r] = v;
		 return;
	}
}

//Sets the value of this face at coordinates r = row, c = column + an additional rotation q (q=90 cw turn)
inline void Face::SetRCQ(const int r, const int c, const byte v, int q)
{
	q = (orientation - q) & 3;

	switch (q)
	{
	case 0:
		data[(r << BS) + c] = v;
		return;
	case 1:
		data[(c << BS) + (R1 - r)] = v;
		return;
	case 2:
		data[(((R1 - r) << BS) + (R1 - c))] = v;
		return;
	case 3:
		data[((R1 - c) << BS) + r] = v;
		return;
	}
}

