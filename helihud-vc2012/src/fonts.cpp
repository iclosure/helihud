#if IBM
#define _CRT_SECURE_NO_WARNINGS 1
#include <windows.h>
#define _USE_MATH_DEFINES
#endif
#if LIN
#include <GL/gl.h>
#include <GL/glu.h>
#define FALSE 0
#define TRUE 1
#else
#define TRUE 1
#define FALSE 0
#if APL
#include <Carbon/Carbon.h>
#endif
#if __GNUC__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif
#endif
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fonts.h"
#include "drawing.h"
#include "data_access.h"
#include "XPLMDisplay.h"
#include "XPLMGraphics.h"
#include "XPLMUtilities.h"
#include "BitmapSupport.h"

#if APL
int ConvertPath(const char * inPath, char * outPath, int outPathMaxLen)
{
        CFStringRef inStr = CFStringCreateWithCString(kCFAllocatorDefault, inPath ,kCFStringEncodingMacRoman);
        if (inStr == NULL)
                return -1;
        CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, inStr, kCFURLHFSPathStyle,0);
        CFStringRef outStr = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
        if (!CFStringGetCString(outStr, outPath, outPathMaxLen, kCFURLPOSIXPathStyle))
                return -1;
        CFRelease(outStr);
        CFRelease(url);
        CFRelease(inStr);
        return 0;
}
#endif

HUDFontProperties fontBig = {
                              "courier.bmp",  //fileName
                              ' ', // firstChar
                              0, //texId
                              0, //dispListBase
                              10, //rowsCount
                              10, //colsCount
                              25, //charWidth
                              25, //charHeight
                              -6, //spacing
                              256 //texSize
                            };

HUDFontProperties fontSmall = {
                                "ArialSmall.bmp",  //fileName
                                ' ', // firstChar
                                0, //texId
                                0, //dispListBase
                                6, //rowsCount
                                16, //colsCount
                                16, //charWidth
                                19, //charHeight
                                -2, //spacing
                                256 //texSize
                              };

void InitFonts( void ) {
  LoadFontTextures();
  CreateHUDFont(&fontBig);
  CreateHUDFont(&fontSmall);
}

void CreateHUDFont(HUDFontProperties *f) {
  if ((f->dispListBase = glGenLists((f->colsCount)*(f->rowsCount))) == 0)
    return;
  float texCharWidth = 1.0f /(float ) f->colsCount * (f->colsCount*(f->charWidth)/(float) f->texSize);
  float texCharHeight = 1.0f /(float ) f->rowsCount * (f->rowsCount*f->charHeight/(float) f->texSize);
  for (int i = 0; i < f->colsCount*f->rowsCount; i++)
  {
    int col;
    int row;
    float lx;
    float ly;
    row = i / f->colsCount;
    col = i % f->colsCount;
    lx = col * texCharWidth;  // chars at font ends at 250th pixel
    ly = 1 - ((row + 1) * texCharHeight);
    glNewList(f->dispListBase + i, GL_COMPILE);
    glBegin(GL_QUADS);
    glTexCoord2f(lx+texCharWidth, ly);
    glVertex2f(f->charWidth, 0); // Bottom Right Of The Texture and Quad
    glTexCoord2f(lx, ly);
    glVertex2f(0, 0); // Bottom Left Of The Texture and Quad
    glTexCoord2f(lx, ly+texCharHeight);
    glVertex2f(0, f->charHeight); // Top Left Of The Texture and Quad
    glTexCoord2f(lx+texCharWidth, ly+texCharHeight);
    glVertex2f(f->charWidth, f->charHeight); // Top Right Of The Texture and Quad
    glEnd();
    glEndList();
  }
  debugLog("loaded font %s, base %d.\n", f->fileName, f->dispListBase);
}

void DrawHUDText(const char *pValue, HUDFontProperties* f, int pX, int pY, char pAllign) {
  XPLMBindTexture2d(f->texId, 0);
  int lLen = strlen(pValue);
  if (pAllign >= 0)
  { // center+right
    int textWidth = getTextWidth(f, lLen);
    if (pAllign == 0) // center
      pX -= (textWidth / 2);
    else
      pX -= textWidth; // right
  }
  glTranslatef(pX, pY, 0);
  for (int i = 0; i < lLen; i++)
  {
    glCallList(f->dispListBase + pValue[i]- f->firstChar);
    //glCallList(106);
    glTranslatef(f->charWidth + f->spacing, 0, 0);
  }
  glTranslatef(-pX-(f->charWidth + f->spacing)*lLen, -pY, 0);
}

void DrawHUDNumber(int pValue, HUDFontProperties* f, int pDigits, int pX, int pY, char pAllign) {
  char buffer[20];
  char lFormat[10];
  int lPos = 0;
  lFormat[lPos++] = '%';
  if (pDigits < 0)
  {
    pDigits = -pDigits;
    lFormat[lPos++] = '0';
  }
  if (pDigits > 0)
    lFormat[lPos++] = '0' + pDigits;
  lFormat[lPos++] = 'd';
  lFormat[lPos++] = 0;
  sprintf(buffer, lFormat, pValue);
  DrawHUDText(buffer, f, pX, pY, pAllign);
}

int getTextWidth(HUDFontProperties *f, int numberOfChars) {
  return(numberOfChars * f->charWidth + (numberOfChars + 1) * f->spacing);
}

/// Loads all our textures
void LoadFontTextures(void)
{
  if (!LoadHUDFontTexture(&fontBig))
    debugLog("Font texture failed to load\n");
  if (!LoadHUDFontTexture(&fontSmall))
    debugLog("Font texture failed to load\n");
}

HUDFontProperties *GetBigFont(void) {
  return &fontBig;
}

HUDFontProperties *GetSmallFont(void) {
  return &fontSmall;
}

/// Loads one texture
int LoadHUDFontTexture(HUDFontProperties *f)
{
  int Status=FALSE;
  char TextureFileName[255];
  void *pImageData = 0;
  int sWidth, sHeight;
  IMAGEDATA sImageData;
#if APL && __MACH__

  char TextureFileName2[255];
  int Result = 0;
#endif

  /// Need to get the actual texture path
  /// and append the filename to it.
  strcpy(TextureFileName, getHudConfig()->pluginPath);
  strcat(TextureFileName, f->fileName);

#if APL && __MACH__

  Result = ConvertPath(TextureFileName, TextureFileName2, sizeof(TextureFileName));
  if (Result == 0)
    strcpy(TextureFileName, TextureFileName2);
  else
    XPLMDebugString("Helihud - Unable to convert path\n");
#endif

  /// Get the bitmap from the file
  if (BitmapLoader(TextureFileName, &sImageData, 4))
  {
    Status=TRUE;

    //SwapRedBlue(&sImageData); // code was changed to convert RGB bmp file
	// to 1-channel texture (luminance). So if we are using one channel only, 
	// no need to swap RGB. Actually call above would crash xpl now,
	// because it expects 3 bytes for each pixel so touches un-allocated memory
	// for last 2 pixels
    pImageData = sImageData.pData;

    /// Do the opengl stuff using XPLM functions for a friendly Xplane existence.
    sWidth=sImageData.Width;
    sHeight=sImageData.Height;
    XPLMGenerateTextureNumbers(&(f->texId), 1);
    XPLMBindTexture2d(f->texId, 0);
    //gluBuild2DMipmaps(GL_TEXTURE_2D, 3, sWidth, sHeight, GL_RGB, GL_UNSIGNED_BYTE, pImageData);
	//gluBuild2DMipmaps(GL_TEXTURE_2D, 1, sWidth, sHeight, GL_LUMINANCE, GL_UNSIGNED_BYTE, pImageData);
	gluBuild2DMipmaps(GL_TEXTURE_2D, 4, sWidth, sHeight, GL_RGBA, GL_UNSIGNED_BYTE, pImageData);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
  }
  if (pImageData != NULL)
    free(pImageData);

  return Status;
}

void SetGLText( void ) {
  XPLMSetGraphicsState(0/*Fog*/, 1/*TexUnits*/, 0/*Lighting*/, 0/*AlphaTesting*/, 1/*AlphaBlending*/, 0/*DepthTesting*/, 0/*DepthWriting*/);
  //glBlendFunc(GL_ONE, GL_ONE);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}




