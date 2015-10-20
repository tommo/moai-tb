#include "MOAITBCanvas.h"
#include "tb_widgets.h"
#include "tb_window.h"
#include "tb_widgets_common.h"
#include "tb_node_tree.h"
#include "tb_widgets_reader.h"

#include "TBRendererMOAI.h"

//----------------------------------------------------------------//
// Glue
//----------------------------------------------------------------//

int MOAITBCanvas::_getRootWidget ( lua_State* L ) {
	MOAI_LUA_SETUP ( MOAITBCanvas, "U" )
	MOAITBWidget::PushTBWidgetOrNil( state, self->mRootWidget );
	return 1;
}

int MOAITBCanvas::_setSize ( lua_State* L ) {
	MOAI_LUA_SETUP ( MOAITBCanvas, "UNN" )
	
	float width  = state.GetValue < float >( 2, 0.0f );
	float height = state.GetValue < float >( 3, 0.0f );
	self->SetSize( width, height );
	
	return 0;
}

int MOAITBCanvas::_doStep ( lua_State* L ) {
	MOAI_LUA_SETUP ( MOAITBCanvas, "U" )
	float step = state.GetValue< float >( 2, 0.0f );
	self->OnUpdate( step );
	return 0;
}

//----------------------------------------------------------------//
//
//----------------------------------------------------------------//
MOAITBCanvas::MOAITBCanvas () {
	RTTI_BEGIN
		RTTI_EXTEND ( MOAIGraphicsProp )
		RTTI_EXTEND ( MOAIAction )
	RTTI_END
	this->mWidth  = 100.0;
	this->mHeight = 100.0;
	this->mRootWidget = new TBWidget();
	// this->SetDependentMember < MOAIShader >(
	// 	this->mShader, 
	// 	&MOAIShaderMgr::Get().GetShader( MOAIShaderMgr::DECK2D_SHADER )
	// );
}

//----------------------------------------------------------------//
MOAITBCanvas::~MOAITBCanvas () {
	this->mRootWidget->DeleteAllChildren();
	delete this->mRootWidget;
}

//----------------------------------------------------------------//
void MOAITBCanvas::RegisterLuaClass ( MOAILuaState& state ) {

	MOAIGraphicsProp::RegisterLuaClass ( state );
	MOAIAction::RegisterLuaClass ( state );
}

//----------------------------------------------------------------//
void MOAITBCanvas::RegisterLuaFuncs ( MOAILuaState& state ) {
	
	MOAIGraphicsProp::RegisterLuaFuncs ( state );
	MOAIAction::RegisterLuaFuncs ( state );
	
	luaL_Reg regTable [] = {
		{ "getRootWidget",            _getRootWidget        },
		{ "setSize",                  _setSize              },
		{ "doStep",                   _doStep               },
		{ NULL, NULL }
	};
	
	luaL_register ( state, 0, regTable );
}

//----------------------------------------------------------------//
u32 MOAITBCanvas::OnGetModelBounds ( ZLBox& bounds ) {
	//TODO
	return BOUNDS_GLOBAL;
}


//----------------------------------------------------------------//
void MOAITBCanvas::OnUpdate ( double step ) {
	this->mRootWidget->InvokeProcessStates();
	this->mRootWidget->InvokeProcess();
	TBMessageHandler::ProcessMessages();
	this->ScheduleUpdate ();
}

//----------------------------------------------------------------//
bool MOAITBCanvas::IsDone () {

	return false;
}

//----------------------------------------------------------------//
void MOAITBCanvas::Draw ( int subPrimID, float lod ) {
	UNUSED ( subPrimID );

	if ( !this->IsVisible ( lod ) ) return;
	TBRendererMOAI* renderer = static_cast< TBRendererMOAI* >( g_renderer );
	this->LoadGfxState ();
	this->LoadVertexTransform ();

	renderer->PushCanvas( this );
	renderer->BeginPaint( this->mWidth, this->mHeight );
	this->mRootWidget->InvokePaint(TBWidget::PaintProps());
	renderer->EndPaint();
	renderer->PopCanvas( this );
}

//----------------------------------------------------------------//
void MOAITBCanvas::SetSize( float w, float h ) {
	this->mWidth  = w;
	this->mHeight = h;
	this->mRootWidget->SetRect( TBRect( 0.0, 0.0, w, h ) );
}



void MOAITBCanvas::BuildLocalToWorldMtx ( ZLAffine3D& localToWorldMtx ) {
	
	localToWorldMtx.ScRoTr (
		this->mScale.mX,
		-this->mScale.mY,
		this->mScale.mZ,
		ClampEuler ( this->mRot.mX ) * ( float )D2R,
		ClampEuler ( this->mRot.mY ) * ( float )D2R,
		ClampEuler ( this->mRot.mZ ) * ( float )D2R,
		this->mLoc.mX,
		this->mLoc.mY,
		this->mLoc.mZ
	);
	
	ZLAffine3D shear;
	shear.Shear ( this->mShearYX, this->mShearZX, this->mShearXY, this->mShearZY, this->mShearXZ, this->mShearYZ );
	localToWorldMtx.Prepend ( shear );
	
	if (( this->mPiv.mX != 0.0f ) || ( this->mPiv.mY != 0.0f ) || ( this->mPiv.mZ != 0.0f )) {
		
		ZLAffine3D pivot;
		pivot.Translate ( -this->mPiv.mX, -this->mPiv.mY, -this->mPiv.mZ );
		localToWorldMtx.Prepend ( pivot );
	}

}


void MOAITBCanvas::RestoreRender() {
	this->LoadGfxState ();
	this->LoadVertexTransform ();
}