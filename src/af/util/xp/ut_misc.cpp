/* AbiSource Program Utilities
 * Copyright (C) 1998 AbiSource, Inc.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  
 * 02111-1307, USA.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>

#include "ut_vector.h"
#include "ut_string_class.h"
#include "ut_types.h"
#include "ut_misc.h"
#include "ut_assert.h"
#include "ut_string.h"
#include "ut_debugmsg.h"
/*****************************************************************/
/*****************************************************************/

UT_Rect::UT_Rect()
{
	left = top = height = width = 0;
}

UT_Rect::UT_Rect(UT_sint32 iLeft, UT_sint32 iTop, UT_sint32 iWidth, UT_sint32 iHeight)
{
	left = iLeft;
	top = iTop;
	width = iWidth;
	height = iHeight;
}

UT_Rect::UT_Rect(const UT_Rect & r)
{
	left = r.left;
	top = r.top;
	width = r.width;
	height = r.height;
}


UT_Rect::UT_Rect(const UT_Rect * r)
{
	left = r->left;
	top = r->top;
	width = r->width;
	height = r->height;
}

bool UT_Rect::containsPoint(UT_sint32 x, UT_sint32 y) const
{
	// return true iff the given (x,y) is inside the rectangle.

	if ((x < left) || (x >= left+width))
		return false;
	if ((y < top) || (y >= top+height))
		return false;

	return true;
}

void UT_Rect::set(int iLeft, int iTop, int iWidth, int iHeight)
{
	left = iLeft;
	top = iTop;
	width = iWidth;
	height = iHeight;
}

/*!
 * This method makes a union of the current rectangle with the one in the 
 * parameter list. This rectangle is the smallest one that covers both
 * rectangles.
 */
void UT_Rect::unionRect(const UT_Rect * pRect)
{
	UT_sint32 fx1,fx2,fy1,fy2;
	fx1 = UT_MIN(left,pRect->left);
	fx2 = UT_MAX(left+width,pRect->left + pRect->width);
	fy1 = UT_MIN(top,pRect->top);
	fy2 = UT_MAX(top+height,pRect->top + pRect->height);
	left = fx1;
	width = fx2 - fx1;
	top = fy1;
	height = fy2 - fy1;
}

bool UT_Rect::intersectsRect(const UT_Rect * pRect) const
{
	// return true if this rectangle and pRect intersect.

#define R_RIGHT(pr)		(((pr)->left)+((pr)->width))
#define R_BOTTOM(pr)	(((pr)->top)+((pr)->height))
	
	if (R_RIGHT(pRect) < left)
		return false;

	if (R_RIGHT(this) < pRect->left)
		return false;

	if (R_BOTTOM(pRect) < top)
		return false;

	if (R_BOTTOM(this) < pRect->top)
		return false;

	return true;

#undef R_RIGHT
#undef R_BOTTOM
}

/*****************************************************************/
/*****************************************************************/

/* These are the colors defined in the SVG standard (I haven't checked the final recommendation for changes)
 */
struct colorToRGBMapping
{
  char * m_name;

  unsigned char m_red;
  unsigned char m_green;
  unsigned char m_blue;
};

static struct colorToRGBMapping s_Colors[] =
{
	{ "aliceblue",			240, 248, 255 },
	{ "antiquewhite",		250, 235, 215 },
	{ "aqua",			  0, 255, 255 },
	{ "aquamarine",			127, 255, 212 },
	{ "azure",			240, 255, 255 },
	{ "beige",			245, 245, 220 },
	{ "bisque",			255, 228, 196 },
	{ "black",			  0,   0,   0 },
	{ "blanchedalmond",		255, 235, 205 },
	{ "blue",			  0,   0, 255 },
	{ "blueviolet",			138,  43, 226 },
	{ "brown",			165,  42,  42 },
	{ "burlywood",			222, 184, 135 },
	{ "cadetblue",			 95, 158, 160 },
	{ "chartreuse",			127, 255,   0 },
	{ "chocolate",			210, 105,  30 },
	{ "coral",			255, 127,  80 },
	{ "cornflowerblue",		100, 149, 237 },
	{ "cornsilk",			255, 248, 220 },
	{ "crimson",			220,  20,  60 },
	{ "cyan",			  0, 255, 255 },
	{ "darkblue",			  0,   0, 139 },
	{ "darkcyan",			  0, 139, 139 },
	{ "darkgoldenrod",		184, 134,  11 },
	{ "darkgray",			169, 169, 169 },
	{ "darkgreen",			  0, 100,   0 },
	{ "darkgrey",			169, 169, 169 },
	{ "darkkhaki",			189, 183, 107 },
	{ "darkmagenta",		139,   0, 139 },
	{ "darkolivegreen",		 85, 107,  47 },
	{ "darkorange",			255, 140,   0 },
	{ "darkorchid",			153,  50, 204 },
	{ "darkred",			139,   0,   0 },
	{ "darksalmon",			233, 150, 122 },
	{ "darkseagreen",		143, 188, 143 },
	{ "darkslateblue",		 72,  61, 139 },
	{ "darkslategray",		 47,  79,  79 },
	{ "darkslategrey",		 47,  79,  79 },
	{ "darkturquoise",		  0, 206, 209 },
	{ "darkviolet",			148,   0, 211 },
	{ "deeppink",			255,  20, 147 },
	{ "deepskyblue",		  0, 191, 255 },
	{ "dimgray",			105, 105, 105 },
	{ "dimgrey",			105, 105, 105 },
	{ "dodgerblue",			 30, 144, 255 },
	{ "firebrick",			178,  34,  34 },
	{ "floralwhite",		255, 250, 240 },
	{ "forestgreen",		 34, 139,  34 },
	{ "fuchsia",			255,   0, 255 },
	{ "gainsboro",			220, 220, 220 },
	{ "ghostwhite",			248, 248, 255 },
	{ "gold",			255, 215,   0 },
	{ "goldenrod",			218, 165,  32 },
	{ "gray",			128, 128, 128 },
	{ "grey",			128, 128, 128 },
	{ "green",			  0, 128,   0 },
	{ "greenyellow",		173, 255,  47 },
	{ "honeydew",			240, 255, 240 },
	{ "hotpink",			255, 105, 180 },
	{ "indianred",			205,  92,  92 },
	{ "indigo",			 75,   0, 130 },
	{ "ivory",			255, 255, 240 },
	{ "khaki",			240, 230, 140 },
	{ "lavender",			230, 230, 250 },
	{ "lavenderblush",		255, 240, 245 },
	{ "lawngreen",			124, 252,   0 },
	{ "lemonchiffon",		255, 250, 205 },
	{ "lightblue",			173, 216, 230 },
	{ "lightcoral",			240, 128, 128 },
	{ "lightcyan",			224, 255, 255 },
	{ "lightgoldenrodyellow",	250, 250, 210 },
	{ "lightgray",			211, 211, 211 },
	{ "lightgreen",			144, 238, 144 },
	{ "lightgrey",			211, 211, 211 },
	{ "lightpink",			255, 182, 193 },
	{ "lightsalmon",		255, 160, 122 },
	{ "lightseagreen",		 32, 178, 170 },
	{ "lightskyblue",		135, 206, 250 },
	{ "lightslategray",		119, 136, 153 },
	{ "lightslategrey",		119, 136, 153 },
	{ "lightsteelblue",		176, 196, 222 },
	{ "lightyellow",		255, 255, 224 },
	{ "lime",			  0, 255,   0 },
	{ "limegreen",			 50, 205,  50 },
	{ "linen",			250, 240, 230 },
	{ "magenta",			255,   0, 255 },
	{ "maroon",			128,   0,   0 },
	{ "mediumaquamarine",		102, 205, 170 },
	{ "mediumblue",			  0,   0, 205 },
	{ "mediumorchid",		186,  85, 211 },
	{ "mediumpurple",		147, 112, 219 },
	{ "mediumseagreen",		 60, 179, 113 },
	{ "mediumslateblue",		123, 104, 238 },
	{ "mediumspringgreen",		  0, 250, 154 },
	{ "mediumturquoise",		 72, 209, 204 },
	{ "mediumvioletred",		199,  21, 133 },
	{ "midnightblue",		 25,  25, 112 },
	{ "mintcream",			245, 255, 250 },
	{ "mistyrose",			255, 228, 225 },
	{ "moccasin",			255, 228, 181 },
	{ "navajowhite",		255, 222, 173 },
	{ "navy",			  0,   0, 128 },
	{ "oldlace",			253, 245, 230 },
	{ "olive",			128, 128,   0 },
	{ "olivedrab",			107, 142,  35 },
	{ "orange",			255, 165,   0 },
	{ "orangered",			255,  69,   0 },
	{ "orchid",			218, 112, 214 },
	{ "palegoldenrod",		238, 232, 170 },
	{ "palegreen",			152, 251, 152 },
	{ "paleturquoise",		175, 238, 238 },
	{ "palevioletred",		219, 112, 147 },
	{ "papayawhip",			255, 239, 213 },
	{ "peachpuff",			255, 218, 185 },
	{ "peru",			205, 133,  63 },
	{ "pink",			255, 192, 203 },
	{ "plum",			221, 160, 221 },
	{ "powderblue",			176, 224, 230 },
	{ "purple",			128,   0, 128 },
	{ "red",			255,   0,   0 },
	{ "rosybrown",			188, 143, 143 },
	{ "royalblue",			 65, 105, 225 },
	{ "saddlebrown",		139,  69,  19 },
	{ "salmon",			250, 128, 114 },
	{ "sandybrown",			244, 164,  96 },
	{ "seagreen",			 46, 139,  87 },
	{ "seashell",			255, 245, 238 },
	{ "sienna",			160,  82,  45 },
	{ "silver",			192, 192, 192 },
	{ "skyblue",			135, 206, 235 },
	{ "slateblue",			106,  90, 205 },
	{ "slategray",			112, 128, 144 },
	{ "slategrey",			112, 128, 144 },
	{ "snow",			255, 250, 250 },
	{ "springgreen",		  0, 255, 127 },
	{ "steelblue",			 70, 130, 180 },
	{ "tan",			210, 180, 140 },
	{ "teal",			  0, 128, 128 },
	{ "thistle",			216, 191, 216 },
	{ "tomato",			255,  99,  71 },
	{ "turquoise",			 64, 224, 208 },
	{ "violet",			238, 130, 238 },
	{ "wheat",			245, 222, 179 },
	{ "white",			255, 255, 255 },
	{ "whitesmoke",			245, 245, 245 },
	{ "yellow",			255, 255,   0 },
	{ "yellowgreen",		154, 205,  50 }
};

#ifdef __MRC__
extern "C" {
#endif

static int color_compare (const void * a, const void * b)
{
  const char * name = (const char *) a;
  const colorToRGBMapping * id = (const colorToRGBMapping *) b;
		
  return UT_stricmp (name, id->m_name);
}

#ifdef __MRC__
};
#endif

static bool s_mapNameToColor (const char * name, char * buf7er)
{
  static const char hexval[16] = { '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f' };

  size_t length = sizeof (s_Colors) / sizeof (s_Colors[0]);

  if ((name == 0) || (buf7er == 0)) return false; // Bad programmer! Bad!

  if (name[0] == '#')
    {
      bool isValid = true;
      for (int i = 0; i < 6; i++)
	{
	  switch (name[i+1])
	    {
	    case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
	    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
	      buf7er[i] = name[i+1];
	      break;
	    case 'A': buf7er[i] = 'a'; break;
	    case 'B': buf7er[i] = 'b'; break;
	    case 'C': buf7er[i] = 'c'; break;
	    case 'D': buf7er[i] = 'd'; break;
	    case 'E': buf7er[i] = 'e'; break;
	    case 'F': buf7er[i] = 'f'; break;
	    default:
	      isValid = false;
	      break;
	    }
	  if (!isValid) break;
	}
      if (isValid)
	buf7er[6] = 0;
      else
	buf7er[0] = 0;

      return isValid;
    }

  colorToRGBMapping * id = (colorToRGBMapping *) bsearch (name, s_Colors, (int) length, sizeof (colorToRGBMapping), color_compare);

  if (id == 0)
    {
      buf7er[0] = 0;
      return false;
    }

  buf7er[0] = hexval[(id->m_red   >> 4) & 0x0f];
  buf7er[1] = hexval[ id->m_red         & 0x0f];
  buf7er[2] = hexval[(id->m_green >> 4) & 0x0f];
  buf7er[3] = hexval[ id->m_green       & 0x0f];
  buf7er[4] = hexval[(id->m_blue  >> 4) & 0x0f];
  buf7er[5] = hexval[ id->m_blue        & 0x0f];
  buf7er[6] = 0;

  return true;
}

static int x_hexDigit(char c)
{
	if ((c>='0') && (c<='9'))
	{
		return c-'0';
	}

	if ((c>='a') && (c<='f'))
	{
		return c - 'a' + 10;
	}

	if ((c>='A') && (c<='F'))
	{
		return c - 'A' + 10;
	}

	UT_ASSERT(UT_SHOULD_NOT_HAPPEN);

	return 0;
}

UT_RGBColor::UT_RGBColor()
{
	m_red = 0;
	m_grn = 0;
	m_blu = 0;
}

UT_RGBColor::UT_RGBColor(unsigned char red, unsigned char grn, unsigned char blu)
{
	m_red = red;
	m_grn = grn;
	m_blu = blu;
}

void UT_setColor(UT_RGBColor & col, unsigned char r, unsigned char g, unsigned char b)
{
	col.m_red = r;
	col.m_grn = g;
	col.m_blu = b;
}

void UT_parseColor(const char *p, UT_RGBColor& c)
{
        UT_uint32 len = strlen (p);

	char returned_color[7] = "";

	if ( s_mapNameToColor ( p, returned_color ) )
	  {
	    goto ParseHex;
	  }
	else if ( 6 == len )
	  {
	    strncpy ( returned_color, p, 6 ) ;
	    goto ParseHex;
	  }

	UT_DEBUGMSG(("String = %s \n",p));
	UT_ASSERT(UT_NOT_IMPLEMENTED);

	return;

 ParseHex:
	c.m_red = x_hexDigit(returned_color[0]) * 16 + x_hexDigit(returned_color[1]);
	c.m_grn = x_hexDigit(returned_color[2]) * 16 + x_hexDigit(returned_color[3]);
	c.m_blu = x_hexDigit(returned_color[4]) * 16 + x_hexDigit(returned_color[5]);

	return;
}

#ifdef WIN32
#define ut_PATHSEP	'\\'
#else
#define ut_PATHSEP '/'
#endif

const char * UT_pathSuffix(const char * path)
{
	// TODO This needs to be moved to platform code.
	
	if (!path)
		return NULL;

	// This algorithm is pretty simple.  We search backwards for
	// a dot, and if the dot happens AFTER the last slash (if there
	// is a slash), we consider the stuff beyond the dot (in
	// the forward direction) the extension.
	const char * dotpos = strrchr(path, '.');
	const char * slashpos = strrchr(path, ut_PATHSEP);

	if (slashpos)
	{
		// There is a slash, we have a file like "/foo/bar/mox.txt"
		// or "/foo/bar/mox" or "/foo.dir/bar/mox.txt" or
		// "/foo.dir/bar/mox".

		// If there is a dot, and it's after the slash, the extension
		// is to the right.  If the dot is before the slash, there is
		// no extension.  If there's no dot, there's no extension.
		if (dotpos)
			if (dotpos > slashpos)
				return dotpos;
			else
				return NULL;
		else
			return NULL;
	}
	else
	{
		// There is no slash, we have a path like "file" or "file.txt".

		// If there's a dot, return
		if (dotpos)
			return dotpos;
		else
			return NULL;
	}
}

typedef struct {
  UT_UCSChar low;
  UT_UCSChar high; 
} ucs_range;

static ucs_range s_word_delim[] = 
{
  // we will include all control chars from Latin1
  {0x0001, 0x0021},
  {0x0023, 0x0026},
  {0x0028, 0x002f},
  {0x003a, 0x0040},
  {0x005b, 0x0060},
  {0x007b, 0x007e},
  {0x00a1, 0x00a7},
  {0x00a9, 0x00b3},
  {0x00b5, 0x00b7},
  {0x00b9, 0x00bf},
  {0x00f7, 0x00f7}
};

static bool s_find_delim(UT_UCSChar c)
{
    for(UT_uint32 i = 0; i < NrElements(s_word_delim); i++)
	{
		if(c < s_word_delim[i].low)
			return false;

		if(c <= s_word_delim[i].high)
			return true;
	}
	return false;
}

bool UT_isWordDelimiter(UT_UCSChar currentChar, UT_UCSChar followChar, UT_UCSChar prevChar)
{
#if 1
    switch(currentChar)
	{
		case '"': //in some languages this can be in the middle of a word (Hebrew)
		case '\'':	// we want quotes inside words for contractions
		case UCS_LDBLQUOTE:    // smart quote, open double /* wjc */
		case UCS_RDBLQUOTE:    // smart quote, close double /* wjc */
		case UCS_LQUOTE:    // smart quote, open single  /* wjc */
		case UCS_RQUOTE:	// we want quotes inside words for contractions
			if (UT_UCS_isalpha(followChar) && UT_UCS_isalpha(prevChar))
			  {
				  return false;
			  }
			else
			  {
				  return true;
			  }
		case UCS_ABI_OBJECT:
			return false;

		default:
			return s_find_delim(currentChar);
	}

#else
	/*
		TODO we need a more systematic way to handle this, instead 
		TODO of just randomly adding more whitespace & punctuation
		TODO on an as-discovered basis
	*/
	switch (currentChar)
	{
		case ' ':
		case ',':
		case '.':
		case '-':
		case '_':
		case '(':
		case ')':
		case '[':
		case ']':
		case '{':
		case '}':
		case '<':
		case '>':
		case '*':
		case '/':
		case '+':
		case '=':
		case '#':
		case '$':
		case ';':
		case ':':
		case '!':
		case '?':
		case UCS_TAB:	// tab
		case UCS_LF:	// line break
		case UCS_VTAB:	// column break
		case UCS_FF:	// page break
		case 0x00a1:    // upside-down exclamation mark

		/* various currency symbols */
		case 0x00a2:
		case 0x00a3:
		case 0x00a4:
		case 0x00a5:

		/* other symbols */
		case 0x00a6:
		case 0x00a7:
		case 0x00a9:
		case 0x00ab:
		case 0x00ae:
		case 0x00b0:
		case 0x00b1:

		return true;
		case '"': //in some languages this can be in the middle of a word (Hebrew)
		case '\'':	// we want quotes inside words for contractions
		case UCS_LDBLQUOTE:    // smart quote, open double /* wjc */
		case UCS_RDBLQUOTE:    // smart quote, close double /* wjc */
		case UCS_LQUOTE:    // smart quote, open single  /* wjc */
		case UCS_RQUOTE:	// we want quotes inside words for contractions
			if (UT_UCS_isalpha(followChar))
			  {
				  return false;
			  }
			else
			  {
				  return true;
			  }
		case UCS_ABI_OBJECT:
		default:
			return false;
	}
#endif
}

const XML_Char* UT_getAttribute(const XML_Char* name, const XML_Char** atts)
{
	UT_ASSERT(atts);

	const XML_Char** p = atts;

	while (*p)
	{
		if (0 == strcmp((const char*)p[0], (const char*)name))
			break;
		p += 2;
	}

	if (*p)
		return p[1];
	else
		return NULL;
}

//////////////////////////////////////////////////////////////////

UT_sint32 signedLoWord(UT_uint32 dw)
{
	// return low word as a signed quantity

	unsigned short u16 = (unsigned short)(dw & 0xffff);
	signed short   s16 = *(signed short *)&u16;
	UT_sint32      s32 = s16;

	return s32;
}

UT_sint32 signedHiWord(UT_uint32 dw)
{
	// return high word as a signed quantity

	unsigned short u16 = (unsigned short)((dw >> 16) & 0xffff);
	signed short   s16 = *(signed short *)&u16;
	UT_sint32      s32 = s16;

	return s32;
}

//////////////////////////////////////////////////////////////////

/*!
 * simplesplit splits the referring string along the character 'separator', 
 * removing the separator character, and placing the resulting strings in a 
 * UT_Vector. If 'max' is specified, this is done max times - the max + 1
 * string in the vector will contain the remainder of the original string 
 * (str).
 */
UT_Vector * simpleSplit (const UT_String & str, char separator,
						 size_t max)
{
	UT_Vector * utvResult = new UT_Vector();
	UT_String* utsEntry;
	UT_uint32 start = 0;

	for(size_t j = 0; (max == 0 || j < max) && start < str.size(); j++)
	{
		utsEntry = new UT_String;

		for (; (str[start] != separator || j == max - 1) && start < str.size(); start++)
			*utsEntry += str[start];

		start++;						// skipping over the separator character
										// itself
		if (utsEntry->empty())
			delete utsEntry;
		else
			utvResult->addItem(utsEntry);
	}

	return utvResult;
}

/**
 * It search the next space (blank space, tab, etc.) in the string
 * str, starting at offset.
 *
 * @param str is the string where we will look for spaces.
 * @param offset is the offset where we will start our search.
 * @returns offset of the next space.
 */
UT_uint32 find_next_space(const UT_String& str, UT_uint32 offset)
{
	size_t max = str.size();
	for (++offset; offset < max; ++offset)
		if (isspace(str[offset]))
			break;

	return offset;
}

/**
 * It warps the string str in various lines, taking care that no line
 * goes beyond the column col.
 *
 * @param str is the string to warp.
 * @param col_max is the column that no character in any line should
 *                pass (except if we can not cut this line anywhere).
 */
void warpString(UT_String& str, size_t col_max)
{
	size_t max = str.size();

	for (UT_uint32 i = 0; i < max;)
	{
		UT_uint32 j = i, old_j;

		do {
			old_j = j;
			j = find_next_space(str, j);

			if (j < max && str[j] == '\n')
				i = j;
		} while (j - i < col_max && j < max);

		if (j >= max)
			return;

		if (old_j == i) // no spaces in the whole line!
		{
			str[j] = '\n';
			i = j;
		}
		else
		{
			str[old_j] = '\n';
			i = old_j;
		}
	}
}

/*!
 * Strips off the first numeric part of string and returns it as a uint32.
 * ie. "Numbered Heading 5" would return 5.
 */
UT_uint32 UT_HeadingDepth(const char * szHeadingName)
{
	UT_String sz;
	UT_uint32 i = 0;
	bool bFound = false;
	bool bStop = false;
	for(i=0; i< strlen(szHeadingName) && !bStop ; i++)
	{
		if(szHeadingName[i] >= '0' && szHeadingName[i] <= '9')
		{
			sz += szHeadingName[i];
			bFound = true;
		}
		else if(bFound)
		{
			bStop = true;
			break;
		}
	}
	i = (UT_uint32) atoi(sz.c_str());
	return i;
}
