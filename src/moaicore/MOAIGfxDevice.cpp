// Copyright (c) 2010-2011 Zipline Games, Inc. All Rights Reserved.
// http://getmoai.com

#include "pch.h"

#include <moaicore/MOAIGfxDevice.h>
#include <moaicore/MOAIGfxDevice.h>
#include <moaicore/MOAIShader.h>
#include <moaicore/MOAIShaderMgr.h>
#include <moaicore/MOAITexture.h>
#include <moaicore/MOAIVertexFormat.h>
#include <moaicore/MOAIVertexFormatMgr.h>
#include <moaicore/MOAIViewport.h>

//================================================================//
// local
//================================================================//

//----------------------------------------------------------------//
// TODO: doxygen
int MOAIGfxDevice::_isProgrammable ( lua_State* L ) {

	MOAIGfxDevice& gfxDevice = MOAIGfxDevice::Get ();
	lua_pushboolean ( L, gfxDevice.IsProgrammable ());
	
	return 1;
}

//================================================================//
// MOAIGfxDevice
//================================================================//

//----------------------------------------------------------------//
void MOAIGfxDevice::BeginPrim () {

	if ( this->mPrimSize ) {

		u32 primBytes = this->mPrimSize * this->mVertexFormat->GetVertexSize ();

		this->mMaxPrims = ( u32 )( this->mSize / primBytes );
		this->mPrimTop = this->mTop + primBytes;
	}
}

//----------------------------------------------------------------//
void MOAIGfxDevice::BeginPrim ( u32 primType ) {

	this->SetPrimType ( primType );
	this->BeginPrim ();
}

//----------------------------------------------------------------//
void MOAIGfxDevice::Clear () {

	if ( this->mBuffer ) {
		free ( this->mBuffer );
		this->mBuffer = 0;
		this->mSize = 0;
		this->mTop = 0;
	}
}

//----------------------------------------------------------------//
void MOAIGfxDevice::ClearColorBuffer ( u32 color ) {

	USColorVec colorVec;
	colorVec.SetRGBA ( color );
	
	glClearColor ( colorVec.mR, colorVec.mG, colorVec.mB, 1.0f );
	glClear ( GL_COLOR_BUFFER_BIT );
}

//----------------------------------------------------------------//
void MOAIGfxDevice::ClearErrors () {

	while ( glGetError () != GL_NO_ERROR );
}

//----------------------------------------------------------------//
u32 MOAIGfxDevice::CountErrors () const {

	u32 count = 0;
	while ( glGetError () != GL_NO_ERROR ) count++;
	return count;
}

//----------------------------------------------------------------//
void MOAIGfxDevice::DetectContext () {

	#ifdef __GLEW_H__
		glewInit ();
	#endif

	const GLubyte* driverVersion = glGetString ( GL_VERSION );
	
	STLString version = ( cc8* )driverVersion;
	version.to_lower ();
	
	STLString gles = "opengl es";
	
	if ( version.find ( gles ) != version.npos ) {
		this->mIsES = true;
		version = version.substr ( gles.length ());
		
		size_t space = version.find ( ' ' );
		if ( space != version.npos ) {
			version = version.substr ( space + 1 );
		}
	}
	else {
		this->mIsES = false;
	}
	
	version = version.substr ( 0, 3 );
	
	this->mMajorVersion = version.at ( 0 ) - '0';
	this->mMinorVersion = version.at ( 2 ) - '0';
	
	this->mIsProgrammable = ( this->mMajorVersion >= 2 );
}

//----------------------------------------------------------------//
void MOAIGfxDevice::DrawPrims () {

	if ( this->mVertexFormat ) {

		u32 vertexSize = this->mVertexFormat->GetVertexSize ();

		if ( this->mPrimCount && vertexSize ) {
		
			u32 count = this->mPrimSize ? this->mPrimCount * this->mPrimSize : ( u32 )( this->mTop / vertexSize );
			glDrawArrays ( this->mPrimType, 0, count );
		}
	}
}

//----------------------------------------------------------------//
void MOAIGfxDevice::DrawPrims ( const MOAIVertexFormat& format, GLenum primType, void* buffer, u32 size ) {

	if ( !( buffer && size )) return;
	
	this->Flush ();
	this->SetVertexFormat ();
	
	// load the software render state
	//glColor4f ( this->mPenColor.mR, this->mPenColor.mG, this->mPenColor.mB, this->mPenColor.mA );
	
	// TODO
	//glMatrixMode ( GL_MODELVIEW );
	//MOAIGfxDevice::LoadMatrix ( this->mModelToWorldMtx );
	//
	//glMatrixMode ( GL_TEXTURE );
	//MOAIGfxDevice::LoadMatrix ( this->mUVTransform );
	
	// draw the prims
	u32 nVerts = ( u32 )( size / format.GetVertexSize ());
	if ( nVerts ) {
		
		if ( this->mIsProgrammable ) {
		
			format.BindProgrammable ( buffer );
			glDrawArrays ( primType, 0, nVerts );
			format.UnbindProgrammable ();
		}
		else {
		
			format.BindFixed ( buffer );
			glDrawArrays ( primType, 0, nVerts );
			format.UnbindFixed ();
		}
	}
	
	// reset
	//glColor4f ( 1.0f, 1.0f, 1.0f, 1.0f );
	
	//glMatrixMode ( GL_MODELVIEW );
	//glLoadIdentity ();
	
	//glMatrixMode ( GL_TEXTURE );
	// glLoadIdentity ();
}

//----------------------------------------------------------------//
void MOAIGfxDevice::EndPrim () {

	if ( this->mPrimSize ) {
		this->mTop = this->mPrimTop;
	}
	++this->mPrimCount;
	
	if (( this->mPrimSize == 0 ) || ( this->mPrimCount >= this->mMaxPrims )) {
		this->Flush ();
	}
}

//----------------------------------------------------------------//
void MOAIGfxDevice::Flush () {

	this->DrawPrims ();

	this->mTop = 0;
	this->mPrimTop = 0;
	this->mPrimCount = 0;
}

//----------------------------------------------------------------//
cc8* MOAIGfxDevice::GetErrorString ( int error ) const {

	switch ( error ) {
		case GL_INVALID_ENUM:		return "GL_INVALID_ENUM";
		case GL_INVALID_VALUE:		return "GL_INVALID_VALUE";
		case GL_INVALID_OPERATION:	return "GL_INVALID_OPERATION";
		case GL_STACK_OVERFLOW:		return "GL_STACK_OVERFLOW";
		case GL_STACK_UNDERFLOW:	return "GL_STACK_UNDERFLOW";
		case GL_OUT_OF_MEMORY:		return "GL_OUT_OF_MEMORY";
	}
	return "";
}

//----------------------------------------------------------------//
u32 MOAIGfxDevice::GetHeight () const {

	return this->mHeight;
}

//----------------------------------------------------------------//
USMatrix3D MOAIGfxDevice::GetModelToWorldMtx () const {

	return this->mVertexTransforms [ VTX_WORLD_TRANSFORM ];
}

//----------------------------------------------------------------//
USMatrix3D MOAIGfxDevice::GetModelToWndMtx () const {

	USMatrix3D modelToWnd = this->GetModelToWorldMtx ();
	modelToWnd.Append ( this->GetWorldToWndMtx ());
	return modelToWnd;
}

//----------------------------------------------------------------//
USColorVec MOAIGfxDevice::GetPenColor () const {

	return this->mPenColor;
}

//----------------------------------------------------------------//
USRect MOAIGfxDevice::GetRect () const {

	USRect rect;
	rect.mXMin = 0;
	rect.mYMin = 0;
	rect.mXMax = ( float )this->mWidth;
	rect.mYMax = ( float )this->mHeight;
	
	return rect;
}

//----------------------------------------------------------------//
USMatrix3D MOAIGfxDevice::GetUVTransform () const {

	return this->mUVTransform;
}

//----------------------------------------------------------------//
USMatrix3D MOAIGfxDevice::GetVertexTransform ( u32 id ) const {

	return this->mVertexTransforms [ id ];
}

//----------------------------------------------------------------//
USMatrix3D MOAIGfxDevice::GetViewProjMtx () const {

	USMatrix3D mtx = this->mVertexTransforms [ VTX_VIEW_TRANSFORM ];
	mtx.Append ( this->mVertexTransforms [ VTX_PROJ_TRANSFORM ]);
	return mtx;
}

//----------------------------------------------------------------//
USQuad MOAIGfxDevice::GetViewQuad () const {

	USQuad quad;

	USMatrix3D invMtx;
	invMtx.Inverse ( this->GetViewProjMtx ());
	
	quad.mV [ 0 ].Init ( -1.0f, 1.0f );
	quad.mV [ 1 ].Init ( 1.0f, 1.0f );
	quad.mV [ 2 ].Init ( 1.0f, -1.0f );
	quad.mV [ 3 ].Init ( -1.0f, -1.0f );
	
	invMtx.TransformQuad ( quad.mV );
	return quad;
}

//----------------------------------------------------------------//
USRect MOAIGfxDevice::GetViewRect () const {

	return this->mViewRect;
}

//----------------------------------------------------------------//
u32 MOAIGfxDevice::GetWidth () const {

	return this->mWidth;
}

//----------------------------------------------------------------//
USMatrix3D MOAIGfxDevice::GetWndToModelMtx () const {

	USMatrix3D wndToModel;
	wndToModel.Inverse ( this->GetModelToWndMtx ());
	return wndToModel;
}

//----------------------------------------------------------------//
USMatrix3D MOAIGfxDevice::GetWndToWorldMtx () const {

	USMatrix3D wndToWorld;
	USMatrix3D mtx;

	USRect rect = this->GetViewRect ();
	
	float hWidth = rect.Width () * 0.5f;
	float hHeight = rect.Height () * 0.5f;

	// Inv Wnd
	wndToWorld.Translate ( -hWidth - rect.mXMin, -hHeight - rect.mYMin, 0.0f );
		
	mtx.Scale (( 1.0f / hWidth ), -( 1.0f / hHeight ), 1.0f );
	wndToWorld.Append ( mtx );
	
	// inv viewproj
	mtx = this->GetViewProjMtx ();
	mtx.Inverse ();
	wndToWorld.Append ( mtx );
	
	return wndToWorld;
}

//----------------------------------------------------------------//
USMatrix3D MOAIGfxDevice::GetWorldToModelMtx () const {
	
	USMatrix3D worldToModel;
	worldToModel.Inverse ( this->mVertexTransforms [ VTX_WORLD_TRANSFORM ]);
	return worldToModel;
}

//----------------------------------------------------------------//
USMatrix3D MOAIGfxDevice::GetWorldToWndMtx ( float xScale, float yScale ) const {

	USMatrix3D worldToWnd;
	USMatrix3D mtx;

	USRect rect = this->GetViewRect ();
	
	float hWidth = rect.Width () * 0.5f;
	float hHeight = rect.Height () * 0.5f;

	// viewproj
	worldToWnd = this->GetViewProjMtx ();
	
	// wnd
	mtx.Scale ( hWidth * xScale, hHeight * yScale, 1.0f );
	worldToWnd.Append ( mtx );
		
	mtx.Translate ( hWidth + rect.mXMin, hHeight + rect.mYMin, 0.0f );
	worldToWnd.Append ( mtx );
	
	return worldToWnd;
}

//----------------------------------------------------------------//
void MOAIGfxDevice::GpuLoadMatrix ( const USMatrix3D& mtx ) const {

	glLoadMatrixf ( mtx.m );
}

//----------------------------------------------------------------//
void MOAIGfxDevice::GpuMultMatrix ( const USMatrix3D& mtx ) const {

	glMultMatrixf ( mtx.m );
}

//----------------------------------------------------------------//
bool MOAIGfxDevice::IsOpenGLES () {

	return this->mIsES;
}

//----------------------------------------------------------------//
bool MOAIGfxDevice::IsProgrammable () {

	return this->mIsProgrammable;
}

//----------------------------------------------------------------//
MOAIGfxDevice::MOAIGfxDevice () :
	mVertexFormat ( 0 ),
	mBuffer ( 0 ),
	mSize ( 0 ),
	mTop ( 0 ),
	mPrimTop ( 0 ),
	mPrimType ( GL_POINTS ),
	mPrimSize ( 0 ),
	mPrimCount ( 0 ),
	mMaxPrims ( 0 ),
	mTexture ( 0 ),
	mShader ( 0 ),
	mPackedColor ( 0xffffffff ),
	mPenWidth ( 1.0f ),
	mPointSize ( 1.0f ),
	mBlendEnabled ( 0 ),
	mWidth ( 0 ),
	mHeight ( 0 ),
	mUVMtxInput ( UV_STAGE_MODEL ),
	mUVMtxOutput ( UV_STAGE_MODEL ),
	mVertexMtxInput ( VTX_STAGE_MODEL ),
	mVertexMtxOutput ( VTX_STAGE_MODEL ),
	mCpuVertexTransform ( false ),
	mCpuUVTransform ( false ),
	mIsES ( false ),
	mMajorVersion ( 0 ),
	mMinorVersion ( 0 ),
	mIsProgrammable ( false ) {
	
	RTTI_SINGLE ( MOAIGfxDevice )
	
	this->Reserve ( DEFAULT_BUFFER_SIZE );
	
	for ( u32 i = 0; i < TOTAL_VTX_TRANSFORMS; ++i ) {
		this->mVertexTransforms [ i ].Ident ();
	}
	this->mUVTransform.Ident ();
	this->mCpuVertexTransformMtx.Ident ();
	
	this->mPenColor.Set ( 1.0f, 1.0f, 1.0f, 1.0f );
	this->mViewRect.Init ( 0.0f, 0.0f, 0.0f, 0.0f );
	this->mScissorRect.Init ( 0.0f, 0.0f, 0.0f, 0.0f );
}

//----------------------------------------------------------------//
MOAIGfxDevice::~MOAIGfxDevice () {

	this->Clear ();
}

//----------------------------------------------------------------//
u32 MOAIGfxDevice::PrintErrors () {

	u32 count = 0;
	for ( int error = glGetError (); error != GL_NO_ERROR; error = glGetError (), ++count ) {
		printf ( "OPENGL ERROR: %s\n", this->GetErrorString ( error ));
	}
	return count;
}

//----------------------------------------------------------------//
void MOAIGfxDevice::RegisterLuaClass ( USLuaState& state ) {

	luaL_Reg regTable [] = {
		{ "isProgrammable",				_isProgrammable },
		{ NULL, NULL }
	};

	luaL_register( state, 0, regTable );
}

//----------------------------------------------------------------//
void MOAIGfxDevice::Reserve ( u32 size ) {

	this->mSize = size;
	this->mTop = 0;
	this->mBuffer = malloc ( size );
}

//----------------------------------------------------------------//
void MOAIGfxDevice::Reset () {

	this->mTop = 0;
	this->mPrimCount = 0;

	// turn off texture
	glDisable ( GL_TEXTURE_2D );
	this->mTexture = 0;
	
	// turn off blending
	glDisable ( GL_BLEND );
	this->mBlendEnabled = false;
	
	// clear the vertex format
	this->SetVertexFormat ();

	// clear the shader
	this->mShader = 0;
	
	// disable backface culling
	glDisable ( GL_CULL_FACE );
	
	// reset the pen width
	this->mPenWidth = 1.0f;
	glLineWidth (( GLfloat )this->mPenWidth );
	
	// reset the point size
	this->mPointSize = 1.0f;
	
	// reset the scissor rect
	MOAIGfxDevice& device = MOAIGfxDevice::Get ();
	USRect scissorRect = device.GetRect ();
	glScissor (( int )scissorRect.mXMin, ( int )scissorRect.mYMin, ( int )scissorRect.Width (), ( int )scissorRect.Height ());
	this->mScissorRect = scissorRect;
	
	for ( u32 i = 0; i < TOTAL_VTX_TRANSFORMS; ++i ) {
		this->mVertexTransforms [ i ].Ident ();
	}
	this->mUVTransform.Ident ();
	this->mCpuVertexTransformMtx.Ident ();
	
	this->mVertexMtxInput = VTX_STAGE_MODEL;
	this->mVertexMtxOutput = VTX_STAGE_MODEL;
	
	// fixed function reset
	if ( !this->IsProgrammable ()) {
		
		// load identity matrix
		glMatrixMode ( GL_MODELVIEW );
		glLoadIdentity ();
		
		glMatrixMode ( GL_PROJECTION );
		glLoadIdentity ();
		
		glMatrixMode ( GL_TEXTURE );
		glLoadIdentity ();
		
		// reset the current vertex color
		glColor4f ( 1.0f, 1.0f, 1.0f, 1.0f );
		
		// reset the point size
		glPointSize (( GLfloat )this->mPointSize );
	}
}

//----------------------------------------------------------------//
void MOAIGfxDevice::SetBlendMode () {

	if ( this->mBlendEnabled ) {
		this->Flush ();
		glDisable ( GL_BLEND );
		this->mBlendEnabled = false;
	}
}

//----------------------------------------------------------------//
void MOAIGfxDevice::SetBlendMode ( const USBlendMode& blendMode ) {

	if ( !( this->mBlendEnabled && this->mBlendMode.IsSame ( blendMode ))) {
		this->Flush ();
		glBlendFunc ( blendMode.mSourceFactor, blendMode.mDestFactor );
		glEnable ( GL_BLEND );
		this->mBlendEnabled = true;
		this->mBlendMode = blendMode;
	}
}

//----------------------------------------------------------------//
void MOAIGfxDevice::SetBlendMode ( int srcFactor, int dstFactor ) {

	USBlendMode blendMode;
	blendMode.SetBlend ( srcFactor, dstFactor );
	
	this->SetBlendMode ( blendMode );
}

//----------------------------------------------------------------//
void MOAIGfxDevice::SetFrameBuffer ( MOAITexture* frameBuffer ) {

	this->Flush ();
	if ( frameBuffer && frameBuffer->IsFrameBuffer ()) {
		frameBuffer->BindFrameBuffer ();
	}
	else {
		glBindFramebuffer ( GL_FRAMEBUFFER, 0 );
	}
}

//----------------------------------------------------------------//
void MOAIGfxDevice::SetPenColor ( u32 color ) {

	this->mPenColor.SetRGBA ( color );
	this->mPackedColor = color;
}

//----------------------------------------------------------------//
void MOAIGfxDevice::SetPenColor ( const USColorVec& colorVec ) {

	this->mPenColor = colorVec;
	this->mPackedColor = this->mPenColor.PackRGBA ();
}

//----------------------------------------------------------------//
void MOAIGfxDevice::SetPenColor ( float r, float g, float b, float a ) {

	this->mPenColor.Set ( r, g, b, a );
	this->mPackedColor = this->mPenColor.PackRGBA ();
}

//----------------------------------------------------------------//
void MOAIGfxDevice::SetPenWidth ( float penWidth ) {

	if ( this->mPenWidth != penWidth ) {
		this->Flush ();
		this->mPenWidth = penWidth;
		glLineWidth (( GLfloat )penWidth );
	}
}

//----------------------------------------------------------------//
void MOAIGfxDevice::SetPointSize ( float pointSize ) {

	if ( this->mPointSize != pointSize ) {
		this->Flush ();
		this->mPointSize = pointSize;
		glPointSize (( GLfloat )pointSize );
	}
}

//----------------------------------------------------------------//
void MOAIGfxDevice::SetPrimType ( u32 primType ) {

	if ( this->mPrimType != primType ) {

		this->Flush ();
		this->mPrimType = primType;

		switch ( primType ) {
		
			case GL_POINTS:
				this->mPrimSize = 1;
				break;
			
			case GL_LINES:
				this->mPrimSize = 2;
				break;
			
			case GL_TRIANGLES:
				this->mPrimSize = 3;
				break;
			
			case GL_LINE_LOOP:
			case GL_LINE_STRIP:
			case GL_TRIANGLE_FAN:
			case GL_TRIANGLE_STRIP:
			default:
				this->mPrimSize = 0;
				break;
		}
	}
}

//----------------------------------------------------------------//
void MOAIGfxDevice::SetScissorRect () {

	this->SetScissorRect ( this->GetRect ());
}

//----------------------------------------------------------------//
void MOAIGfxDevice::SetScissorRect ( const USRect& rect ) {
	
	USRect& current = this->mScissorRect;
	
	if (	( current.mXMin != rect.mXMin ) ||
			( current.mYMin != rect.mYMin ) ||
			( current.mXMax != rect.mXMax ) ||
			( current.mYMax != rect.mYMax )) {
	
		this->Flush ();
		glScissor (( int )rect.mXMin, ( int )rect.mYMin, ( int )rect.Width (), ( int )rect.Height ());
		this->mScissorRect = rect;
	}
}

//----------------------------------------------------------------//
void MOAIGfxDevice::SetScreenSpace ( MOAIViewport& viewport ) {
	UNUSED ( viewport );

	// TODO

	//glMatrixMode ( GL_MODELVIEW );
	//glLoadIdentity ();
	//
	//USMatrix3D wndToNorm;
	//viewport.GetWndToNormMtx ( wndToNorm );
	//
	//glMatrixMode ( GL_PROJECTION );
	//MOAIGfxDevice::LoadMatrix ( wndToNorm );
}

//----------------------------------------------------------------//
void MOAIGfxDevice::SetShader ( MOAIShader* shader ) {

	if ( this->mShader != shader ) {
	
		this->Flush ();
		this->mShader = shader;
		
		if ( shader ) {
			shader->Bind ();
		}
	}
}

//----------------------------------------------------------------//
void MOAIGfxDevice::SetShaderPreset ( u32 preset ) {

	MOAIShaderMgr::Get ().BindShader ( preset );
}

//----------------------------------------------------------------//
void MOAIGfxDevice::SetSize ( u32 width, u32 height ) {

	this->mWidth = width;
	this->mHeight = height;
}

//----------------------------------------------------------------//
bool MOAIGfxDevice::SetTexture ( MOAITexture* texture ) {
	
	if ( this->mTexture == texture ) {
		return texture ? true : false;
	}
	
	this->Flush ();
	
	if ( texture ) {
		if ( !this->mTexture ) {
			glEnable ( GL_TEXTURE_2D );
		}
		this->mTexture = texture;
		return texture->Bind ();
	}

	if ( this->mTexture ) {
		glDisable ( GL_TEXTURE_2D );
		this->mTexture = 0;
	}
	return false;
}

//----------------------------------------------------------------//
void MOAIGfxDevice::SetUVMtxMode ( u32 input, u32 output ) {

	if (( this->mUVMtxInput != input ) || ( this->mUVMtxOutput != output )) {
		
		this->mUVMtxInput = input;
		this->mUVMtxOutput = output;
		
		this->UpdateUVMtx ();
	}
}

//----------------------------------------------------------------//
void MOAIGfxDevice::SetUVTransform () {

	USMatrix3D mtx;
	mtx.Ident ();
	this->SetUVTransform ( mtx );
}

//----------------------------------------------------------------//
void MOAIGfxDevice::SetUVTransform ( const USAffine2D& transform ) {

	this->mUVTransform.Init ( transform );
	this->UpdateUVMtx ();
}

//----------------------------------------------------------------//
void MOAIGfxDevice::SetUVTransform ( const USMatrix3D& transform ) {

	this->mUVTransform = transform;
	this->UpdateUVMtx ();
}

//----------------------------------------------------------------//
void MOAIGfxDevice::SetVertexFormat () {

	this->Flush ();
	
	if ( this->mVertexFormat ) {
		
		if ( this->mIsProgrammable ) {
			this->mVertexFormat->UnbindProgrammable ();
		}
		else {
			this->mVertexFormat->UnbindFixed ();
		}
	}
	this->mVertexFormat = 0;
}

//----------------------------------------------------------------//
void MOAIGfxDevice::SetVertexFormat ( const MOAIVertexFormat& format ) {

	if ( this->mVertexFormat != &format ) {

		this->SetVertexFormat ();
		this->mVertexFormat = &format;

		if ( this->mIsProgrammable ) {
			this->mVertexFormat->BindProgrammable ( this->mBuffer );
		}
		else {
			this->mVertexFormat->BindFixed ( this->mBuffer );
		}
	}
}

//----------------------------------------------------------------//
void MOAIGfxDevice::SetVertexMtxMode ( u32 input, u32 output ) {

	if (( this->mVertexMtxInput != input ) || ( this->mVertexMtxOutput != output )) {

		this->mVertexMtxInput = input;
		this->mVertexMtxOutput = output;
		
		this->UpdateCpuVertexMtx ();
		this->UpdateGpuVertexMtx ();
	}
}

//----------------------------------------------------------------//
void MOAIGfxDevice::SetVertexPreset ( u32 preset ) {

	this->SetVertexFormat ( MOAIVertexFormatMgr::Get ().GetPreset ( preset ));
}

//----------------------------------------------------------------//
void MOAIGfxDevice::SetVertexTransform ( u32 id ) {

	USMatrix3D mtx;
	mtx.Ident ();
	this->SetVertexTransform ( id, mtx );
}

//----------------------------------------------------------------//
void MOAIGfxDevice::SetVertexTransform ( u32 id, const USAffine2D& transform ) {

	USMatrix3D mtx;
	mtx.Init ( transform );
	this->SetVertexTransform ( id, mtx );
}

//----------------------------------------------------------------//
void MOAIGfxDevice::SetVertexTransform ( u32 id, const USMatrix3D& transform ) {

	if ( !this->mVertexTransforms [ id ].IsSame ( transform )) {

		this->mVertexTransforms [ id ] = transform;
		
		// check to see if this is a CPU or GPU matrix and update accordingly
		if ( id < this->mVertexMtxOutput ) {
			this->UpdateCpuVertexMtx ();
		}
		else {
			this->UpdateGpuVertexMtx ();
		}
		
		if ( this->mShader ) {
			this->mShader->UpdatePipelineTransforms (
				this->mVertexTransforms [ VTX_WORLD_TRANSFORM ],
				this->mVertexTransforms [ VTX_VIEW_TRANSFORM ],
				this->mVertexTransforms [ VTX_PROJ_TRANSFORM ]
			);
		}
	}
}

//----------------------------------------------------------------//
void MOAIGfxDevice::SetViewport () {

	float width = ( float )this->mWidth;
	float height = ( float )this->mHeight;

	MOAIViewport viewport;
	viewport.Init ( 0.0f, 0.0f, width, height );
	viewport.SetScale ( width, -height );
	viewport.SetOffset ( -1.0f, 1.0f );
	
	this->SetViewport ( viewport );
}

//----------------------------------------------------------------//
void MOAIGfxDevice::SetViewport ( MOAIViewport& viewport ) {

	// set us up the viewport
	USRect rect = viewport.GetRect ();
	
	GLint x = ( GLint )rect.mXMin;
	GLint y = ( GLint )rect.mYMin;
	
	GLsizei w = ( GLsizei )( rect.Width () + 0.5f );
	GLsizei h = ( GLsizei )( rect.Height () + 0.5f );
	
	glViewport ( x, y, w, h );

	this->mViewRect = rect;
}

//----------------------------------------------------------------//
void MOAIGfxDevice::UpdateCpuVertexMtx () {

	u32 start = this->mVertexMtxInput;
	u32 finish = this->mVertexMtxOutput;
	
	this->mCpuVertexTransformMtx.Ident ();
	
	for ( u32 i = start; i < finish; ++i ) {
		this->mCpuVertexTransformMtx.Append ( this->mVertexTransforms [ i ]);
	}
	this->mCpuVertexTransform = !this->mCpuVertexTransformMtx.IsIdent ();
}

//----------------------------------------------------------------//
void MOAIGfxDevice::UpdateGpuVertexMtx () {

	if ( this->IsProgrammable ()) return;

	this->Flush ();

	// update the gpu matrices
	switch ( this->mVertexMtxOutput ) {
		
		case VTX_STAGE_MODEL:
		
			glMatrixMode ( GL_MODELVIEW );
			this->GpuLoadMatrix ( this->mVertexTransforms [ VTX_WORLD_TRANSFORM ]);
			this->GpuMultMatrix ( this->mVertexTransforms [ VTX_VIEW_TRANSFORM ]);
		
			glMatrixMode ( GL_PROJECTION );
			this->GpuLoadMatrix ( this->mVertexTransforms [ VTX_PROJ_TRANSFORM ]);
			
			break;
			
		case VTX_STAGE_WORLD:
			
			glMatrixMode ( GL_MODELVIEW );
			this->GpuLoadMatrix ( this->mVertexTransforms [ VTX_VIEW_TRANSFORM ]);
			
			glMatrixMode ( GL_PROJECTION );
			this->GpuLoadMatrix ( this->mVertexTransforms [ VTX_PROJ_TRANSFORM ]);
		
			break;
			
		case VTX_STAGE_VIEW:
			
			glMatrixMode ( GL_MODELVIEW );
			glLoadIdentity ();
			
			glMatrixMode ( GL_PROJECTION );
			this->GpuLoadMatrix ( this->mVertexTransforms [ VTX_PROJ_TRANSFORM ]);
			
			break;
		
		case VTX_STAGE_PROJ:
		
			glMatrixMode ( GL_MODELVIEW );
			glLoadIdentity ();
			
			glMatrixMode ( GL_PROJECTION );
			glLoadIdentity ();
		
			break;
	}
}

//----------------------------------------------------------------//
void MOAIGfxDevice::UpdateUVMtx () {

	if ( this->mUVMtxOutput == UV_STAGE_TEXTURE ) {
		
		this->mCpuUVTransform = !this->mUVTransform.IsIdent ();
	}
	else {
		
		this->mCpuUVTransform = false;
		
		// flush and load gl UV transform
		this->Flush ();
		glMatrixMode ( GL_TEXTURE );
		this->GpuLoadMatrix ( this->mUVTransform );
	}
}

//----------------------------------------------------------------//
void MOAIGfxDevice::WriteQuad ( USVec2D* vtx, USVec2D* uv ) {

	if ( this->mCpuVertexTransform ) {
		this->mCpuVertexTransformMtx.TransformQuad ( vtx );
	}
	
	if ( this->mCpuUVTransform ) {
		this->mUVTransform.TransformQuad ( uv );
	}
	
	this->BeginPrim ();
	
		this->Write ( vtx[ 3 ]);
		this->Write ( uv [ 3 ]);
		this->WritePenColor4b ();
		
		this->Write ( vtx[ 1 ]);
		this->Write ( uv [ 1 ]);
		this->WritePenColor4b ();	
	
		this->Write ( vtx[ 0 ]);
		this->Write ( uv [ 0 ]);
		this->WritePenColor4b ();
		
	this->EndPrim ();
	
	this->BeginPrim ();
	
		this->Write ( vtx[ 3 ]);
		this->Write ( uv [ 3 ]);
		this->WritePenColor4b ();	
	
		this->Write ( vtx[ 2 ]);
		this->Write ( uv [ 2 ]);
		this->WritePenColor4b ();	
	
		this->Write ( vtx[ 1 ]);
		this->Write ( uv [ 1 ]);
		this->WritePenColor4b ();
		
	this->EndPrim ();
}
