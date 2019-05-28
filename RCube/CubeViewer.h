#pragma once


#include "Cube.h"
#include "TinyPngOut.hpp" //https://www.nayuki.io/page/tiny-png-output



class CubeViewer
{

	//Collor palette
	const static uint8_t palette[6][3];

public:

	void ExportFaceDiagram(Face& face,std::string FileName, int ImageWidth, bool IncludeGridlines);

};

