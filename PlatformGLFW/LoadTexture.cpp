#include "StdAfx.h"
#include "Render.h"

// get texture template definition
#include "Texture.h"

//========================================================================
// TGA file header information
//========================================================================

struct TGAHeader
{
	int idlen;                 // 1 byte
	int cmaptype;              // 1 byte
	int imagetype;             // 1 byte
	int cmapfirstidx;          // 2 bytes
	int cmaplen;               // 2 bytes
	int cmapentrysize;         // 1 byte
	int xorigin;               // 2 bytes
	int yorigin;               // 2 bytes
	int width;                 // 2 bytes
	int height;                // 2 bytes
	int bitsperpixel;          // 1 byte
	int imageinfo;             // 1 byte
	int _alphabits;            // (derived from imageinfo)
	int _origin;               // (derived from imageinfo)
};

#define _TGA_CMAPTYPE_NONE      0
#define _TGA_CMAPTYPE_PRESENT   1

#define _TGA_IMAGETYPE_NONE     0
#define _TGA_IMAGETYPE_CMAP     1
#define _TGA_IMAGETYPE_TC       2
#define _TGA_IMAGETYPE_GRAY     3
#define _TGA_IMAGETYPE_CMAP_RLE 9
#define _TGA_IMAGETYPE_TC_RLE   10
#define _TGA_IMAGETYPE_GRAY_RLE 11

#define _TGA_IMAGEINFO_ALPHA_MASK   0x0f
#define _TGA_IMAGEINFO_ALPHA_SHIFT  0
#define _TGA_IMAGEINFO_ORIGIN_MASK  0x30
#define _TGA_IMAGEINFO_ORIGIN_SHIFT 4

#define _TGA_ORIGIN_BL 0
#define _TGA_ORIGIN_BR 1
#define _TGA_ORIGIN_UL 2
#define _TGA_ORIGIN_UR 3


//========================================================================
// Read TGA file header (and check that it is valid)
//========================================================================

static int ReadTGAHeader(FILE *s, TGAHeader *h)
{
	unsigned char buf[ 18 ];
	int pos;

	// Read TGA file header from file
	pos = ftell(s);
	fread(buf, 18, 1, s);

	// Interpret header (endian independent parsing)
	h->idlen         = (int) buf[0];
	h->cmaptype      = (int) buf[1];
	h->imagetype     = (int) buf[2];
	h->cmapfirstidx  = (int) buf[3] | (((int) buf[4]) << 8);
	h->cmaplen       = (int) buf[5] | (((int) buf[6]) << 8);
	h->cmapentrysize = (int) buf[7];
	h->xorigin       = (int) buf[8] | (((int) buf[9]) << 8);
	h->yorigin       = (int) buf[10] | (((int) buf[11]) << 8);
	h->width         = (int) buf[12] | (((int) buf[13]) << 8);
	h->height        = (int) buf[14] | (((int) buf[15]) << 8);
	h->bitsperpixel  = (int) buf[16];
	h->imageinfo     = (int) buf[17];

	// Extract alphabits and origin information
	h->_alphabits = (int) (h->imageinfo & _TGA_IMAGEINFO_ALPHA_MASK) >>
		_TGA_IMAGEINFO_ALPHA_SHIFT;
	h->_origin    = (int) (h->imageinfo & _TGA_IMAGEINFO_ORIGIN_MASK) >>
		_TGA_IMAGEINFO_ORIGIN_SHIFT;

	// Validate TGA header (is this a TGA file?)
	if ((h->cmaptype == 0 || h->cmaptype == 1) &&
		((h->imagetype >= 1 && h->imagetype <= 3) ||
		(h->imagetype >= 9 && h->imagetype <= 11)) &&
		(h->bitsperpixel == 8 || h->bitsperpixel == 24 ||
		h->bitsperpixel == 32))
	{
		// Skip the ID field
		fseek(s, h->idlen, SEEK_CUR);

		// Indicate that the TGA header was valid
		return GL_TRUE;
	}
	else
	{
		// Restore file position
		fseek(s, pos, SEEK_SET);

		// Indicate that the TGA header was invalid
		return GL_FALSE;
	}
}

//========================================================================
// Read Run-Length Encoded data
//========================================================================

static void ReadTGA_RLE(unsigned char *buf, int size, int bpp, FILE *s)
{
	int repcount, bytes, k, n;
	unsigned char pixel[ 4 ];
	char c;

	// Dummy check
	if (bpp > 4)
	{
		return;
	}

	while(size > 0)
	{
		// Get repetition count
		fread(&c, 1, 1, s);
		repcount = (unsigned int) c;
		bytes = ((repcount & 127) + 1) * bpp;
		if (size < bytes)
		{
			bytes = size;
		}

		// Run-Length packet?
		if (repcount & 128)
		{
			fread(pixel, 1, bpp, s);
			for(n = 0; n < (repcount & 127) + 1; n ++)
			{
				for(k = 0; k < bpp; k ++)
				{
					*buf ++ = pixel[ k ];
				}
			}
		}
		else
		{
			// It's a Raw packet
			fread(buf, 1, bytes, s);
			buf += bytes;
		}

		size -= bytes;
	}
}


//========================================================================
// Read a TGA image from a file
//========================================================================

unsigned char *ReadTGA(FILE *s, int &width, int &height, int &components)
{
	TGAHeader h;
	unsigned char *cmap, *pix, tmp, *src, *dst;
	int cmapsize, pixsize, pixsize2;
	int bpp, bpp2, k, m, n, swapx, swapy;

	// Read TGA header
	if (!ReadTGAHeader(s, &h))
	{
		return NULL;
	}

	// Is there a colormap?
	cmapsize = (h.cmaptype == _TGA_CMAPTYPE_PRESENT ? 1 : 0) * h.cmaplen *
		((h.cmapentrysize+7) / 8);
	if (cmapsize > 0)
	{
		// Is it a colormap that we can handle?
		if ((h.cmapentrysize != 24 && h.cmapentrysize != 32) ||
			h.cmaplen == 0 || h.cmaplen > 256)
		{
			return NULL;
		}

		// Allocate memory for colormap
		cmap = (unsigned char *) malloc(cmapsize);
		if (cmap == NULL)
		{
			return NULL;
		}

		// Read colormap from file
		fread(cmap, 1, cmapsize, s);
	}
	else
	{
		cmap = NULL;
	}

	// Size of pixel data
	pixsize = h.width * h.height * ((h.bitsperpixel + 7) / 8);

	// Bytes per pixel (pixel data - unexpanded)
	bpp = (h.bitsperpixel + 7) / 8;

	// Bytes per pixel (expanded pixels - not colormap indeces)
	if (cmap)
	{
		bpp2 = (h.cmapentrysize + 7) / 8;
	}
	else
	{
		bpp2 = bpp;
	}

	// For colormaped images, the RGB/RGBA image data may use more memory
	// than the stored pixel data
	pixsize2 = h.width * h.height * bpp2;

	// Allocate memory for pixel data
	pix = (unsigned char *) malloc(pixsize2);
	if (pix == NULL)
	{
		if (cmap)
		{
			free(cmap);
		}
		return NULL;
	}

	// Read pixel data from file
	if (h.imagetype >= _TGA_IMAGETYPE_CMAP_RLE)
	{
		ReadTGA_RLE(pix, pixsize, bpp, s);
	}
	else
	{
		fread(pix, 1, pixsize, s);
	}

	// If the image origin is not what we want, re-arrange the pixels
	switch(h._origin)
	{
	default:
	case _TGA_ORIGIN_UL:
		swapx = 0;
		swapy = 1;
		break;

	case _TGA_ORIGIN_BL:
		swapx = 0;
		swapy = 0;
		break;

	case _TGA_ORIGIN_UR:
		swapx = 1;
		swapy = 1;
		break;

	case _TGA_ORIGIN_BR:
		swapx = 1;
		swapy = 0;
		break;
	}
	if (swapy)
	{
		src = pix;
		dst = &pix[ (h.height-1)*h.width*bpp ];
		for(n = 0; n < h.height/2; n ++)
		{
			for(m = 0; m < h.width ; m ++)
			{
				for(k = 0; k < bpp; k ++)
				{
					tmp     = *src;
					*src ++ = *dst;
					*dst ++ = tmp;
				}
			}
			dst -= 2*h.width*bpp;
		}
	}
	if (swapx)
	{
		src = pix;
		dst = &pix[ (h.width-1)*bpp ];
		for(n = 0; n < h.height; n ++)
		{
			for(m = 0; m < h.width/2 ; m ++)
			{
				for(k = 0; k < bpp; k ++)
				{
					tmp     = *src;
					*src ++ = *dst;
					*dst ++ = tmp;
				}
				dst -= 2*bpp;
			}
			src += ((h.width+1)/2)*bpp;
			dst += ((3*h.width+1)/2)*bpp;
		}
	}

	// Convert BGR/BGRA to RGB/RGBA, and optionally colormap indeces to
	// RGB/RGBA values
	if (cmap)
	{
		// Convert colormap pixel format (BGR -> RGB or BGRA -> RGBA)
		if (bpp2 == 3 || bpp2 == 4)
		{
			for(n = 0; n < h.cmaplen; n ++)
			{
				tmp                = cmap[ n*bpp2 ];
				cmap[ n*bpp2 ]     = cmap[ n*bpp2 + 2 ];
				cmap[ n*bpp2 + 2 ] = tmp;
			}
		}

		// Convert pixel data to RGB/RGBA data
		for(m = h.width * h.height - 1; m >= 0; m --)
		{
			n = pix[ m ];
			for(k = 0; k < bpp2; k ++)
			{
				pix[ m*bpp2 + k ] = cmap[ n*bpp2 + k ];
			}
		}

		// Free memory for colormap (it's not needed anymore)
		free(cmap);
	}
	else
	{
		// Convert image pixel format (BGR -> RGB or BGRA -> RGBA)
		if (bpp2 == 3 || bpp2 == 4)
		{
			src = pix;
			dst = &pix[ 2 ];
			for(n = 0; n < h.height * h.width; n ++)
			{
				tmp  = *src;
				*src = *dst;
				*dst = tmp;
				src += bpp2;
				dst += bpp2;
			}
		}
	}

	// return values
	width = h.width;
	height = h.height;
	components = bpp2;
	return pix;
}


namespace Platform
{
	bool LoadTexture(TextureTemplate &aTexture, const char *aName)
	{
		// read image
		FILE *file = fopen(aName, "rb");
		if (!file)
			return false;

		int width, height, components;
		unsigned char *data = ReadTGA(file, width, height, components);
		if (!data)
		{
			fclose(file);
			return false;
		}

		// texture dimensions
		aTexture.mWidth = width;
		aTexture.mHeight = height;

		// texture format
		switch (components)
		{
		case 1:
			aTexture.mFormat = GL_R8;			// GL_ALPHA;
			aTexture.mInternalFormat = GL_R;	// GL_ALPHA8;
			break;
		case 2:
			aTexture.mFormat = GL_RG8;
			aTexture.mInternalFormat = GL_RG;
			break;
		case 3:
			aTexture.mFormat = GL_RGB8;
			aTexture.mInternalFormat = GL_RGB;
			break;
		case 4:
			aTexture.mFormat = GL_RGBA8;
			aTexture.mInternalFormat = GL_RGBA;
			break;
		}

		// texture data
		aTexture.mPixels = data;

		// close file
		fclose(file);

		// success!
		return true;
	}
}