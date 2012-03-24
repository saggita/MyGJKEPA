#ifndef __BITMAP_H_INCLUDED__
#define __BITMAP_H_INCLUDED__

class BmpUtils
{
	public:
		enum
		{
			FILEHEADERSIZE = 14,
			INFOHEADERSIZE = 40,
			HEADERSIZE = (FILEHEADERSIZE+INFOHEADERSIZE),
		};
		struct Rgb{
			unsigned char b;
			unsigned char g;
			unsigned char r;
		};

		struct Image{
			Image():data(0){}
			Image(int width, int height){allocate(width, height);}
			~Image();
			void allocate(int width, int height);
			
			unsigned int height;
			unsigned int width;
			Rgb* data;
		};

		static
		Image *read(char *filename);
		static
		int write(char *filename, Image *img);
};

#include <AdlGraphics/BmpUtils.inl>

#endif /*__BITMAP_H_INCLUDED__*/
