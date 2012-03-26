//
//  Gizmo.cpp
//  SceneGraph
//
//  Created by Simon Geilfus on 22/03/12.
//

#include "Gizmo.h"

namespace cinder {
    
    
    GizmoRef Gizmo::Create( Vec2i viewportSize ){
        GizmoRef gizmo              = GizmoRef( new Gizmo() );
        gizmo->mCurrentMode         = TRANSLATE;
        gizmo->mWindowSize          = Rectf( 0, 0, viewportSize.x, viewportSize.y );;
        
        gl::Fbo::Format format;
        format.enableColorBuffer();
        format.setColorInternalFormat( GL_RGBA );
        format.setSamples( 0 );
        
        gizmo->mPositionFbo         = gl::Fbo( viewportSize.x / 2.0f, viewportSize.y / 2.0f, format );
        gizmo->mCursorFbo           = gl::Fbo( 10, 10, format );
        gizmo->mSelectedAxis        = -1;
        gizmo->mPosition            = Vec3f( 0.0f, 0.0f, 0.0f );
        gizmo->mRotations           = Quatf();
        gizmo->mScale               = Vec3f( 1.0f, 1.0f, 1.0f );
        
        return gizmo;
    }
    
    
    void Gizmo::setMatrices( CameraPersp cam ){
        
        mCurrentCam = cam;
        mProjection = cam.getProjectionMatrix();
        mModelView  = cam.getModelViewMatrix();
        
        // Render Gizmo to the position Fbo
        mPositionFbo.bindFramebuffer();
        
        gl::setMatricesWindowPersp( mPositionFbo.getSize() );
        
        glMatrixMode( GL_PROJECTION );
        glLoadMatrixf( mProjection.m );
        
        gl::clear( ColorA( 0.0f, 0.0f, 0.0f, 0.0f ) );
        
        gl::pushModelView();
        
        glMatrixMode( GL_MODELVIEW );
        glLoadMatrixf( mModelView.m );
        
        // Mult by the unscaled matrix so we don't get non-uniform scales on our graphics
        gl::multModelView( mUnscaledTransform );
        
        gl::enableDepthRead();
        gl::enableDepthWrite();
        
        // Scale the graphics so they look always the same size on the screen
        float scale = ( mTransform.getTranslate() - mCurrentCam.getEyePoint() ).length() / 200.0f;
        gl::scale( scale, scale, scale );
        
        // Draw Gizmo graphics
        switch( mCurrentMode ){
            case TRANSLATE: drawTranslate(); break;
            case ROTATE: drawRotate(); break;
            case SCALE: drawScale(); break;
        }
        
        gl::disableDepthRead();
        gl::disableDepthWrite();
        
        gl::popModelView();
        mPositionFbo.unbindFramebuffer();
    }
    
    void Gizmo::draw(){
        gl::pushModelView();
        
        // Mult by the unscaled matrix so we don't get non-uniform scales on our graphics
        gl::multModelView( mUnscaledTransform );
        
        // Scale the graphics so they look always the same size on the screen
        float scale = ( mTransform.getTranslate() - mCurrentCam.getEyePoint() ).length() / 200.0f;
        gl::scale( scale, scale, scale );
        
        // Draw Gizmo graphics and highlight selected axis
        switch( mCurrentMode ){
            case TRANSLATE: 
                switch( mSelectedAxis ){
                    case -1:    drawTranslate( RED, GREEN, BLUE ); break;
                    case 0:     drawTranslate( YELLOW, GREEN, BLUE ); break;
                    case 1:     drawTranslate( RED, YELLOW, BLUE ); break;
                    case 2:     drawTranslate( RED, GREEN, YELLOW ); break;
                }
                break;
            case ROTATE:
                switch( mSelectedAxis ){
                    case -1:    drawRotate( RED, GREEN, BLUE ); break;
                    case 0:     drawRotate( YELLOW, GREEN, BLUE ); break;
                    case 1:     drawRotate( RED, YELLOW, BLUE ); break;
                    case 2:     drawRotate( RED, GREEN, YELLOW ); break;
                }
                break;
            case SCALE: switch( mSelectedAxis ){
                    case -1:    drawScale( RED, GREEN, BLUE ); break;
                    case 0:     drawScale( YELLOW, GREEN, BLUE ); break;
                    case 1:     drawScale( RED, YELLOW, BLUE ); break;
                    case 2:     drawScale( RED, GREEN, YELLOW ); break;
                }
                break;
        }
        
        gl::drawCoordinateFrame();
        gl::popModelView();
    }
    
    void Gizmo::transform(){
        // Create the transformation matrix, I guess some of the rotations problem are here
        // WRONG ?
        mTransform.setToIdentity();
        mTransform.translate( mPosition );
        mTransform *= mRotations.toMatrix44();
        mUnscaledTransform = mTransform;
        mTransform.scale( mScale );
    }
    void Gizmo::setTransform( Vec3f position, Quatf rotations, Vec3f scale ){
        mPosition   = position;
        mRotations  = rotations;
        mScale      = scale;
        transform();
    }
    void Gizmo::setTransform( Matrix44f m ){
        mTransform = m;
    }
    Matrix44f Gizmo::getTransform(){
        return mTransform;
    }
    void Gizmo::setMode( int mode ){
        mCurrentMode = mode;
    }
    
    
    void Gizmo::registerEvents(){
		mCallbackIds.push_back( app::App::get()->registerMouseDown( this, &Gizmo::mouseDown ) );
		mCallbackIds.push_back( app::App::get()->registerMouseUp( this, &Gizmo::mouseUp ) );
		mCallbackIds.push_back( app::App::get()->registerMouseMove( this, &Gizmo::mouseMove ) );
		mCallbackIds.push_back( app::App::get()->registerMouseDrag( this, &Gizmo::mouseDrag ) );
    }
    void Gizmo::unregisterEvents(){
		if( mCallbackIds.size() ){
			app::App::get()->unregisterMouseDown(	mCallbackIds[ 0 ] );
			app::App::get()->unregisterMouseUp(     mCallbackIds[ 1 ] );
			app::App::get()->unregisterMouseMove(	mCallbackIds[ 2 ] );
			app::App::get()->unregisterMouseDrag(	mCallbackIds[ 3 ] );
        }
    }
    
    bool Gizmo::mouseDown( app::MouseEvent event ){
        
        // Find the plane for the selected axis
        Planef plane;
        switch( mSelectedAxis ){
            case 0: plane = Planef( Vec3f::zero(), Vec3f::yAxis() ); break;
            case 1: plane = Planef( Vec3f::zero(), Vec3f::zAxis() ); break;
            case 2: plane = Planef( Vec3f::zero(), Vec3f::yAxis() ); break;
        }
        
        // Cast a ray from the camera
        Ray ray = mCurrentCam.generateRay( event.getPos().x / (float) mWindowSize.getWidth(), 1.0f - event.getPos().y / (float) mWindowSize.getHeight(), mWindowSize.getWidth() / (float) mWindowSize.getHeight() );
        
        // And check if there's an intersection with the plane
        float intersectionDistance;
        bool intersect = ray.calcPlaneIntersection( plane.getPoint(), plane.getNormal(), &intersectionDistance );
        
        // Use it to get the mouse position in 3D
        if( intersect ){
            Vec3f intersection = ray.getOrigin() + ray.getDirection() * intersectionDistance;
            mMousePos = intersection;
        }
        
        return false;
    }
    bool Gizmo::mouseUp( app::MouseEvent event ){
        return false;
    }
    bool Gizmo::mouseMove( app::MouseEvent event ){
        mSelectedAxis = samplePosition( (float) event.getPos().x / (float) app::getWindowWidth() * (float) mPositionFbo.getWidth(), (float) event.getPos().y  / (float) app::getWindowHeight() * (float) mPositionFbo.getHeight() );
        return false;
    }

    bool Gizmo::mouseDrag( app::MouseEvent event ){           
        
        // Find the plane and the current axis
        Vec3f currentAxis;
        Planef currentPlane;
        switch( mSelectedAxis ){
            case 0: currentAxis = Vec3f::xAxis(); currentPlane = Planef( Vec3f::zero(), Vec3f::yAxis() ); break;
            case 1: currentAxis = Vec3f::yAxis(); currentPlane = Planef( Vec3f::zero(), Vec3f::zAxis() ); break;
            case 2: currentAxis = Vec3f::zAxis(); currentPlane = Planef( Vec3f::zero(), Vec3f::yAxis() ); break;
            default: return false;
        }
        
        // Cast a ray from the camera
        float intersectionDistance;
        Ray ray = mCurrentCam.generateRay( event.getPos().x / (float) mWindowSize.getWidth(), 1.0f - event.getPos().y / (float) mWindowSize.getHeight(), mWindowSize.getWidth() / (float) mWindowSize.getHeight() );
        bool intersect = ray.calcPlaneIntersection( currentPlane.getPoint(), currentPlane.getNormal(), &intersectionDistance );
        
        // And check if there's an intersection with the plane
        if( intersect ){
            
            // Use that to move, rotate or scale 
            Vec3f intersection = ray.getOrigin() + ray.getDirection() * intersectionDistance;
            Vec3f diff = ( intersection - mMousePos );
            if( diff.length() < 50.0f ){ 
                diff *= currentAxis;
                
                if( mCurrentMode == TRANSLATE ){   
                    mPosition += diff;
                }
                else if( mCurrentMode == ROTATE ){
                    diff *= 0.005f;
                    mRotations *= Quatf( currentAxis, diff.x + diff.y + diff.z ) ;
                }
                else if( mCurrentMode == SCALE ){
                    mScale += diff * 0.01f;
                }
                
                transform();
            }
            
            // Keep the last mouse position
            mMousePos = intersection;
        }
        
        
        return false;  
    }
    
    
    Gizmo::Gizmo(){
    }
    
    void Gizmo::drawTranslate( ColorA xColor, ColorA yColor, ColorA zColor ) {
        float axisLength = 30.0f;
        float headLength = 6.0f; 
        float headRadius = 1.5f;
        
        gl::color( xColor );
        gl::drawVector( Vec3f::zero(), Vec3f::xAxis() * axisLength, headLength, headRadius );
        gl::color( yColor );
        gl::drawVector( Vec3f::zero(), Vec3f::yAxis() * axisLength, headLength, headRadius );
        gl::color( zColor );
        gl::drawVector( Vec3f::zero(), Vec3f::zAxis() * axisLength, headLength, headRadius );
    }
    void Gizmo::drawRotate( ColorA xColor, ColorA yColor, ColorA zColor ){
        float axisLength = 30.0f;
        float radius = 2.0f; 
        float slices = 30;
        
        glEnable( GL_CULL_FACE );
        glCullFace( GL_BACK );
        
        gl::color( yColor );
        gl::drawCylinder( axisLength, axisLength, radius, slices );
        
        gl::color( xColor );
        gl::pushModelView();
        gl::rotate( Vec3f::zAxis() * 90 );
        gl::drawCylinder( axisLength, axisLength, radius, slices );
        gl::popModelView();
        
        gl::color( zColor );
        gl::pushModelView();
        gl::rotate( Vec3f::xAxis() * 90 );
        gl::drawCylinder( axisLength, axisLength, radius, slices );
        gl::popModelView();
        
        glDisable( GL_CULL_FACE );
        
    }
    void Gizmo::drawScale( ColorA xColor, ColorA yColor, ColorA zColor ){
        float axisLength = 30.0f;
        Vec3f handleSize = Vec3f( 3.0f, 3.0f, 3.0f );
        
        gl::color( xColor );
        gl::drawLine( Vec3f::zero(), Vec3f::xAxis() * axisLength );
        gl::drawCube( Vec3f::xAxis() * axisLength, handleSize );
        gl::color( yColor );
        gl::drawLine( Vec3f::zero(), Vec3f::yAxis() * axisLength );
        gl::drawCube( Vec3f::yAxis() * axisLength, handleSize );
        gl::color( zColor );
        gl::drawLine( Vec3f::zero(), Vec3f::zAxis() * axisLength );
        gl::drawCube( Vec3f::zAxis() * axisLength, handleSize ); 
    }
    
    
    int Gizmo::samplePosition( int x, int y ){
        
        y  = mPositionFbo.getHeight() - y;
        
        // Copy Cursor Neighbors to the cursor Fbo
        
        mPositionFbo.blitTo( mCursorFbo, Area( x-5, y-5, x+5, y+5), mCursorFbo.getBounds());
        
        mCursorFbo.bindFramebuffer();
        
        GLubyte buffer[400];
        glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
        glReadPixels(0, 0, mCursorFbo.getWidth(), mCursorFbo.getHeight(), GL_RGBA, GL_UNSIGNED_BYTE, (void*)buffer);
        
        mCursorFbo.unbindFramebuffer();
        
        
        // Sample the area and count the occurences of red, green and blue
        
        unsigned int total  = (mCursorFbo.getWidth() * mCursorFbo.getHeight());
        unsigned int color, reds = 0, greens = 0, blues = 0;
        unsigned int red    = 0xff0000;
        unsigned int green  = 0x00ff00;
        unsigned int blue   = 0x0000ff; 
    
        
        for(size_t i=0;i<total;++i) {
            color = charToInt( buffer[(i*4)+0], buffer[(i*4)+1], buffer[(i*4)+2] );
            if( color == red ) reds++;
            else if( color == green ) greens++;
            else if( color == blue ) blues++;
        }
        
        // Return the selected axis
        
        int axis = -1;
        if( reds + greens + blues > 0 ) {
            axis = ( reds > blues && reds > greens ) ? 0 : ( greens > blues && greens > reds ) ? 1 : 2;
        }
        
        return axis;
    }
   
    
    ColorA Gizmo::RED = ColorA( 1.0f, 0.0f, 0.0f, 1.0f);
    ColorA Gizmo::GREEN = ColorA( 0.0f, 1.0f, 0.0f, 1.0f);
    ColorA Gizmo::BLUE = ColorA( 0.0f, 0.0f, 1.0f, 1.0f);
    ColorA Gizmo::YELLOW = ColorA( 1.0f, 1.0f, 0.0f, 1.0f);
}