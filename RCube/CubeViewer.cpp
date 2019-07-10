#include <fstream>
#include "CubeViewer.h"
#include <math.h>

//Color Palette 
const uint8_t CubeViewer::palette[6][3] =
{
	{0x00,0xFF,0x00}, //Green
	{0xFF,0x00,0x00}, //Red
	{0x00,0x00,0xFF}, //Blue
	{0xFF,0x80,0x40}, //Orange
	{0xFF,0xFF,0xFF}, //White
	{0xFF,0xFF,0x00}  //Yellow
};

void CubeViewer::ExportFaceDiagram(Face& face, std::string FileName, int ImageWidth, bool IncludeGridlines)
{
	if (ImageWidth <= 0) return;

	int pixelcount = ImageWidth * ImageWidth;	//number of pixels in the output image
	int datacount = 3 * pixelcount;	//Number of bytes in the output image

	uint8_t* pixels = new uint8_t[datacount];

	//Disable gridlines if the image is too small
	if ((uint)ImageWidth <= face.RowSize * 2) IncludeGridlines = false;

	//Compute scale between cube size and image size
	double wp = ((double)face.RowSize) / ((double)ImageWidth);

	double intpart;
	double fpx, fpy;
	uint px, py;
	double linewidth = .02;
	int colorid;
	int iptr = 0;

	for (int y = ImageWidth-1; y >=0 ; --y)
	{
		for (int x = 0; x < ImageWidth; ++x)
		{
			iptr = 3*((ImageWidth - y - 1) * ImageWidth + x);

			//Compute face coordinates from image coordinates
			py = (int)(y * wp);
			px = (int)(x * wp);

			//Get the colorid from the face
			colorid = face.GetRC(py, px);
			
			pixels[iptr] = palette[colorid][0];
			pixels[iptr+1] = palette[colorid][1];
			pixels[iptr+2] = palette[colorid][2];

			if (IncludeGridlines)
			{
				//Figure out if the pixel hits a grid line
				fpx = modf((x * wp), &intpart);
				fpy = modf((y * wp), &intpart);
				if (((fpx <= linewidth) || (fpx >= (1 - linewidth)) || (fpy <= linewidth) || (fpy >= (1 - linewidth))))
				{
					pixels[iptr] = 0x00; 
					pixels[iptr + 1] = 0x00;
					pixels[iptr + 2] = 0x00;
				}
			}

		}
	}



	//Write .png file
	std::ofstream out(FileName, std::ios::binary);

	TinyPngOut pngout(static_cast<uint32_t>(ImageWidth), static_cast<uint32_t>(ImageWidth), out);

	pngout.write(pixels, static_cast<size_t>(pixelcount));


	delete[] pixels;

}
