//
//  Gizmo.h
//  SceneGraph
//
//  Created by Simon Geilfus on 22/03/12.
//
//  fbo.blitTo trick, charToInt and area color sampling taken from 
//  Paul Houx 3D picking sample  :
//  http://forum.libcinder.org/topic/fast-object-picking-using-multiple-render-targets
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


namespace cinder {
    
    typedef std::shared_ptr< class Gizmo > GizmoRef;
    
    class Gizmo {
    public:
        
        static GizmoRef Create( Vec2i viewportSize );
        
        enum {
            TRANSLATE,
            ROTATE,
            SCALE
        };
        
        void setMatrices( CameraPersp cam );
        
        void draw();
        
        void setTranslate( Vec3f v );
        void setRotate( Quatf q );
        void setScale( Vec3f v );
        void setTransform( Vec3f position, Quatf rotations, Vec3f scale );
        void setTransform( Matrix44f m );
        
        Vec3f       getTranslate(){ return mPosition; }
        Quatf       getRotate(){ return mRotations; }
        Vec3f       getScale(){ return mScale; }
        Matrix44f   getTransform();
        
        void setMode( int mode );
        
        void registerEvents();
        void unregisterEvents();
        
        bool mouseDown( app::MouseEvent event );
        bool mouseUp( app::MouseEvent event );
        bool mouseMove( app::MouseEvent event );
        bool mouseDrag( app::MouseEvent event );
        
        
    protected:
        
        Gizmo();
                
        void drawTranslate( ColorA xColor = RED, ColorA yColor = GREEN, ColorA zColor = BLUE ) ;
        void drawRotate( ColorA xColor = RED, ColorA yColor = GREEN, ColorA zColor = BLUE );
        void drawScale( ColorA xColor = RED, ColorA yColor = GREEN, ColorA zColor = BLUE );
        
        int samplePosition( int x, int y ); 
        
        static unsigned int charToInt( unsigned char r, unsigned char g, unsigned char b ){
            return b + (g << 8) + (r << 16);
        };
        
        void transform();
        
        
        static ColorA RED, GREEN, BLUE, YELLOW;
        
        
        gl::Fbo     mPositionFbo;
        gl::Fbo     mCursorFbo;
        
        Vec3f       mPosition;
        Quatf       mRotations;
        Vec3f       mRotations2;
        Vec3f       mScale;
        
        Matrix44f   mTransform;
        Matrix44f   mUnscaledTransform;
        
        CameraPersp mCurrentCam;
        Matrix44f   mModelView;
        Matrix44f   mProjection;
        Rectf       mWindowSize;
        Area        mViewport;
        
        int         mCurrentMode;
        int         mSelectedAxis;
        Vec3f       mMousePos;
        
		std::vector< ci::CallbackId >	mCallbackIds;
        
    };
}