/**
 *  @file oglplus/text/bitmap_glyph/renderer.ipp
 *  @brief Implementation of Bitmap-font-based text renderer
 *
 *  @author Matus Chochlik
 *
 *  Copyright 2010-2013 Matus Chochlik. Distributed under the Boost
 *  Software License, Version 1.0. (See accompanying file
 *  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 */

namespace oglplus {
namespace text {

OGLPLUS_LIB_FUNC
BitmapGlyphRenderer::BitmapGlyphRenderer(
	BitmapGlyphRenderingBase& parent,
	const GeometryShader& layout_transform_shader,
	const GeometryShader& glyph_transform_shader,
	const FragmentShader& pixel_color_shader
): _parent(parent)
 , _program(ObjectDesc("BitmapGlyphRenderer"))
 , _bitmap_sampler(_program, "oglpBitmap")
 , _metric_sampler(_program, "oglpMetric")
 , _pg_map_sampler(_program, "oglpPageMap")
 , _layout_width(_program, "oglpLayoutWidth")
 , _layout_width_active(false)
 , _prev_font_essence(nullptr)
 , _prev_layout_storage(nullptr)
{
	VertexShader vs(ObjectDesc("BitmapGlyphRenderer - Vertex"));
	vs.Source(StrLit(
		"#version 330\n"

		"uniform uint GlyphsPerPage;"

		"uniform sampler2DRect oglpMetric;"
		"uniform usamplerBuffer oglpPageMap;"

		"layout (location = 0) in uint CodePoint;"
		"layout (location = 1) in float XOffset;"

		"out vec4 vertLogData;"
		"out vec4 vertInkData;"
		"out vec4 vertTexData;"
		"out float vertXOffset;"
		"out float vertFrame;"

		"void main(void)"
		"{"
		"	int goffs = int(CodePoint % GlyphsPerPage)*3;"
		"	int page =  int(CodePoint / GlyphsPerPage);"
		"	int frame = int(texelFetch(oglpPageMap,page).r);"

		"	vertLogData = texelFetch("
		"		oglpMetric,"
		"		ivec2(goffs+0, frame)"
		"	);"
		"	vertInkData = texelFetch("
		"		oglpMetric,"
		"		ivec2(goffs+1, frame)"
		"	);"
		"	vertTexData = texelFetch("
		"		oglpMetric,"
		"		ivec2(goffs+2, frame)"
		"	);"
		"	vertXOffset = XOffset;"
		"	vertFrame = float(frame);"
		"}"
	));
	vs.Compile();
	_program.AttachShader(vs);

	_program.AttachShader(layout_transform_shader);
	_program.AttachShader(glyph_transform_shader);

	GeometryShader gs(ObjectDesc("BitmapGlyphRenderer - Geometry"));
	gs.Source(StrLit(
		"#version 330\n"
		"layout (points) in;"
		"layout (triangle_strip, max_vertices = 6) out;"

		"vec3 TransformGlyph("
		"	vec4 LogMetrics,"
		"	vec4 InkMetrics,"
		"	vec2 Position,"
		"	float XOffset,"
		"	float LayoutWidth,"
		"	int Index"
		");"

		"vec4 TransformLayout(vec3 GlyphPosition);"

		"in vec4 vertLogData[1];"
		"in vec4 vertInkData[1];"
		"in vec4 vertTexData[1];"
		"in float vertXOffset[1];"
		"in float vertFrame[1];"

		"out vec4 geomGlyphPos;"
		"out vec4 geomGlyphCoord;"
		"out vec3 geomTexCoord;"

		"uniform float oglpLayoutWidth;"

		"void make_vertex(vec2 Position, vec2 GlyphCoord, vec2 TexCoord)"
		"{"
		"	geomGlyphPos = vec4(TransformGlyph("
		"		vertLogData[0],"
		"		vertInkData[0],"
		"		Position,"
		"		vertXOffset[0],"
		"		oglpLayoutWidth,"
		"		int(gl_PrimitiveIDIn)"
		"	), vertXOffset[0]);"
		"	gl_Position = TransformLayout(geomGlyphPos.xyz);"
		"	geomGlyphCoord = vec4(Position, GlyphCoord);"
		"	geomTexCoord = vec3(TexCoord, vertFrame[0]);"
		"	EmitVertex();"
		"}"

		"void main(void)"
		"{"
		//      left bearing
		"       float lb = vertInkData[0].x;"
		//      right bearing
		"       float rb = vertInkData[0].y;"
		//      ascender
		"       float as = vertInkData[0].z;"
		//      descender
		"       float ds = vertInkData[0].w;"
		//      height
		"       float ht = as + ds;"
		//      glyph origin in texture space
		"       vec2  to = vertTexData[0].xy;"
		//      glyph width in texture space
		"       float tw = vertTexData[0].z;"
		//      glyph height in texture space
		"       float th = vertTexData[0].w;"
		//      glyph ascent in texture space
		"       float ta = th * (as / ht);"
		//      glyph descent in texture space
		"       float td = th * (ds / ht);"

		"       make_vertex(vec2(rb,-ds), vec2(1.0,-1.0), to+vec2( tw, -td));"
		"       make_vertex(vec2(lb,-ds), vec2(0.0,-1.0), to+vec2(0.0, -td));"
		"       make_vertex(vec2(rb,0.0), vec2(1.0, 0.0), to+vec2( tw, 0.0));"
		"       make_vertex(vec2(lb,0.0), vec2(0.0, 0.0), to+vec2(0.0, 0.0));"
		"       make_vertex(vec2(rb, as), vec2(1.0, 1.0), to+vec2( tw,  ta));"
		"       make_vertex(vec2(lb, as), vec2(0.0, 1.0), to+vec2(0.0,  ta));"
		"       EndPrimitive();"
		"}"
	));
	gs.Compile();
	_program.AttachShader(gs);

	_program.AttachShader(pixel_color_shader);

	FragmentShader fs(ObjectDesc("BitmapGlyphRenderer - Fragment"));
	fs.Source(StrLit(
		"#version 330\n"
		"uniform sampler2DArray oglpBitmap;"

		"uniform float oglpLayoutWidth;"

		"in vec4 geomGlyphPos;"
		"in vec4 geomGlyphCoord;"
		"in vec3 geomTexCoord;"

		"out vec4 fragColor;"

		"vec4 PixelColor("
		"	vec4 TexelColor,"
		"	vec3 GlyphPosition,"
		"	float GlyphXOffset,"
		"	vec2 GlyphExtent,"
		"	vec2 GlyphCoord,"
		"	float LayoutWidth"
		");"

		"void main(void)"
		"{"
		"       fragColor = PixelColor("
		"		texture(oglpBitmap, geomTexCoord),"
		"		geomGlyphPos.xyz,"
		"		geomGlyphPos.w,"
		"		geomGlyphCoord.xy,"
		"		geomGlyphCoord.zw,"
		"		oglpLayoutWidth"
		"	);"
		"}"

	));
	fs.Compile();
	_program.AttachShader(fs);

	_program.Link();
	ProgramUniform<GLuint>(_program, "GlyphsPerPage").Set(
		BitmapGlyphGlyphsPerPage(_parent)
	);
	_layout_width_active = _layout_width.IsActive();
}

OGLPLUS_LIB_FUNC
BitmapGlyphDefaultRenderer::BitmapGlyphDefaultRenderer(
	BitmapGlyphRenderingBase& parent,
	const FragmentShader& pixel_color_shader
): DefaultRendererTpl<BitmapGlyphRenderer>(
	parent,
	pixel_color_shader,
	GeometryShader(
		ObjectDesc("BitmapGlyphRenderer - Layout transform"),
		StrLit("#version 330\n"
		"uniform mat4 "
		"	oglpProjectionMatrix,"
		"	oglpCameraMatrix,"
		"	oglpLayoutMatrix;"
		"mat4 Matrix = "
		"	oglpProjectionMatrix*"
		"	oglpCameraMatrix*"
		"	oglpLayoutMatrix;"

		"vec4 TransformLayout(vec3 GlyphPosition)"
		"{"
		"	return Matrix * vec4(GlyphPosition, 1.0);"
		"}")
	),
	GeometryShader(
		ObjectDesc("BitmapGlyphRenderer - Glyph transform"),
		StrLit("#version 330\n"
		"uniform float oglpAlignCoef;"
		"uniform float oglpDirCoef;"

		"float oglpAlignCoef2 = 0.5*oglpDirCoef-oglpAlignCoef;"
		"float oglpDirCoef2 = min(oglpDirCoef, 0.0);"

		"vec3 TransformGlyph("
		"	vec4 LogMetrics,"
		"	vec4 InkMetrics,"
		"	vec2 Pos,"
		"	float XOffs,"
		"	float LayoutWidth,"
		"	int Idx"
		")"
		"{"
		"	float LogWidth = LogMetrics.y - LogMetrics.x;"
		"	XOffs = oglpDirCoef * XOffs+"
		"		oglpDirCoef2* LogWidth-"
		"		oglpAlignCoef2*LayoutWidth;"
		"	return vec3("
		"		Pos.x+XOffs,"
		"		Pos.y,"
		"		0.0"
		"	);"
		"}")
	)
)
{ }

} // namespace text
} // namespace oglplus

