﻿#ifndef __GMGLGLYPHMANAGER_H__
#define __GMGLGLYPHMANAGER_H__
#include "common.h"
#include "gmdatacore/glyph/glyphmanager.h"
BEGIN_NS

class GMGLGlyphTexture;
struct GMGLGlyphManagerPrivate
{
	GMint cursor_u, cursor_v;
	GMfloat maxHeight;
	GMGLGlyphTexture* texture;
};

class GMGLGlyphManager : public GlyphManager
{
	DEFINE_PRIVATE(GMGLGlyphManager);

public:
	enum
	{
		FONT_SIZE = 24,
		RESOLUTION = 150,
		CANVAS_WIDTH = 1024,
		CANVAS_HEIGHT = 1024,
	};

public:
	GMGLGlyphManager();
	~GMGLGlyphManager();

public:
	virtual ITexture* glyphTexture() override;

protected:
	virtual const GlyphInfo& createChar(GMWChar c) override;
};

END_NS
#endif