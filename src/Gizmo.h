//
//  Gizmo.h
//  SceneGraph
//
//  Created by Simon Geilfus on 22/03/12.
//
//  Fbo.blitTo trick, charToInt and area color sampling taken from 
//  Paul Houx 3D picking sample  :
//  http://forum.libcinder.org/topic/fast-object-picking-using-multiple-render-targets
//
//  Decompose matrix method from Assimp library:
//  http://assimp.sourceforge.net/
//

#pragma once

#include "cinder/app/App.h"
#include "cinder/gl/Fbo.h"
#include "cinder/gl/Vbo.h"
#include "cinder/Vector.h"
#include "cinder/Camera.h"
#include "cinder/Matrix.h"
#include "cinder/CinderMath.h"
#include "cinder/Plane.h"
#include "cinder/Arcball.h"


typedef std::shared_ptr< class Gizmo > GizmoRef;

class Gizmo {
public:
    
    static GizmoRef create( ci::Vec2i viewportSize, bool autoRegisterEvents = true, float gizmoScale = 1.0f, float samplingDefinition = 0.5f );
    
    enum {
        TRANSLATE,
        ROTATE,
        SCALE
    };
    
    void setMatrices( ci::CameraPersp cam );
    
    void draw();
    
    void setTranslate( ci::Vec3f v );
    void setRotate( ci::Quatf q );
    void setScale( ci::Vec3f v );
    void setTransform( ci::Vec3f position, ci::Quatf rotations, ci::Vec3f scale );
    void setTransform( ci::Matrix44f m );
    
    ci::Vec3f       getTranslate();
    ci::Quatf       getRotate();
    ci::Vec3f       getScale();
    ci::Matrix44f   getTransform();
    
    void setMode( int mode );
    
    void registerEvents();
    void unregisterEvents();
    
    bool mouseDown( ci::app::MouseEvent event );
    bool mouseMove( ci::app::MouseEvent event );
    bool mouseDrag( ci::app::MouseEvent event );
    bool resize( ci::app::ResizeEvent event );
    
protected:
    
    Gizmo();
    
    void drawTranslate( ci::ColorA xColor = RED, ci::ColorA yColor = GREEN, ci::ColorA zColor = BLUE ) ;
    void drawRotate( ci::ColorA xColor = RED, ci::ColorA yColor = GREEN, ci::ColorA zColor = BLUE );
    void drawScale( ci::ColorA xColor = RED, ci::ColorA yColor = GREEN, ci::ColorA zColor = BLUE );
    
    int samplePosition( int x, int y ); 
    
    static unsigned int charToInt( unsigned char r, unsigned char g, unsigned char b ){
        return b + (g << 8) + (r << 16);
    };
    
    void transform();
    void decompose();
    
    
    static ci::ColorA RED, GREEN, BLUE, YELLOW;
    
    
    ci::gl::Fbo     mPositionFbo;
    ci::gl::Fbo     mCursorFbo;
    
    ci::Vec3f       mPosition;
    ci::Quatf       mRotations;
    ci::Vec3f       mScale;
    
    ci::Arcball     mArcball;
    
    ci::Matrix44f   mTransform;
    ci::Matrix44f   mUnscaledTransform;
    
    ci::CameraPersp mCurrentCam;
    ci::Matrix44f   mModelView;
    ci::Matrix44f   mProjection;
    ci::Rectf       mWindowSize;
    ci::Area        mViewport;
    
    int             mCurrentMode;
    int             mSelectedAxis;
    ci::Vec3f       mMousePos;
	
	float			mSize;
    
    bool            mCanRotate;
    
    std::vector< ci::CallbackId >	mCallbackIds;
    
};
