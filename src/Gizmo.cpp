//
//  Gizmo.cpp
//  SceneGraph
//
//  Created by Simon Geilfus on 22/03/12.
//

#include "Gizmo.h"




GizmoRef Gizmo::create( ci::Vec2i viewportSize, bool autoRegisterEvents, float gizmoScale, float samplingDefinition ){
    GizmoRef gizmo              = GizmoRef( new Gizmo() );
    gizmo->mCurrentMode         = TRANSLATE;
    gizmo->mWindowSize          = ci::Rectf( 0, 0, viewportSize.x, viewportSize.y );
    
    ci::gl::Fbo::Format format;
    format.enableColorBuffer();
    format.setColorInternalFormat( GL_RGBA );
    format.setSamples( 0 );
    
    gizmo->mPositionFbo         = ci::gl::Fbo( viewportSize.x * samplingDefinition, viewportSize.y * samplingDefinition, format );
    gizmo->mCursorFbo           = ci::gl::Fbo( 5, 5, format );
    gizmo->mSelectedAxis        = -1;
    gizmo->mPosition            = ci::Vec3f( 0.0f, 0.0f, 0.0f );
    gizmo->mRotations           = ci::Quatf();
    gizmo->mScale               = ci::Vec3f( 1.0f, 1.0f, 1.0f );
    gizmo->mArcball             = ci::Arcball( viewportSize );
	gizmo->mSize				= gizmoScale;
    
	if( autoRegisterEvents ) gizmo->registerEvents();
	
    return gizmo;
}


void Gizmo::setMatrices( ci::CameraPersp cam ){
    
    mCurrentCam = cam;
    mProjection = cam.getProjectionMatrix();
    mModelView  = cam.getModelViewMatrix();
    
    // Render Gizmo positions to the Fbo
    mPositionFbo.bindFramebuffer();
    
    ci::gl::setMatricesWindowPersp( mPositionFbo.getSize() );
	ci::gl::setMatrices( cam );
    
    ci::gl::clear( ci::ColorA( 0.0f, 0.0f, 0.0f, 0.0f ) );
    
    ci::gl::pushModelView();
    
    // Mult by the unscaled matrix so we don't get non-uniform scales on our graphics
    ci::gl::multModelView( mUnscaledTransform );
    
    ci::gl::enableDepthRead();
    ci::gl::enableDepthWrite();
    
    // Scale the graphics so they look always the same size on the screen
    float scale = mSize * ( mTransform.getTranslate() - mCurrentCam.getEyePoint() ).length() / 200.0f;
    ci::gl::scale( scale, scale, scale );
    
	glLineWidth( 3.0f );
	
    // Draw Gizmo graphics
    switch( mCurrentMode ){
        case TRANSLATE: drawTranslate(); break;
        case ROTATE: drawRotate(); break;
        case SCALE: drawScale(); break;
    }
	
	glLineWidth( 1.0f );
    
    ci::gl::disableDepthRead();
    ci::gl::disableDepthWrite();
    
    ci::gl::popModelView();
    mPositionFbo.unbindFramebuffer();
}

void Gizmo::draw(){
    ci::gl::pushModelView();
    
    // Mult by the unscaled matrix so we don't get non-uniform scales on our graphics
    ci::gl::multModelView( mUnscaledTransform );
    
    // Scale the graphics so they look always the same size on the screen
    float scale = mSize * ( mTransform.getTranslate() - mCurrentCam.getEyePoint() ).length() / 200.0f;
    ci::gl::scale( scale, scale, scale );
    
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
    
    ci::gl::popModelView();
}

void Gizmo::transform(){
    // Create the transformation matrix, I guess some of the rotations problem are here
    mTransform.setToIdentity();
	mTransform.translate( mPosition );
    mTransform *= mRotations;
    mUnscaledTransform = mTransform;
    mTransform.scale( mScale );
}

void Gizmo::setTranslate( ci::Vec3f v ){ 
	mPosition = v; 
    transform();
}
void Gizmo::setRotate( ci::Quatf q ){ 
	mRotations = q; 
    transform();
}
void Gizmo::setScale( ci::Vec3f v ){ 
	mScale = v; 
    transform();
}


void Gizmo::setTransform( ci::Vec3f position, ci::Quatf rotations, ci::Vec3f scale ){
    mPosition   = position;
    mRotations  = rotations;
    mScale      = scale;
    transform();
}
void Gizmo::setTransform( ci::Matrix44f m ){
    mTransform = m;
    decompose();
}

ci::Vec3f Gizmo::getTranslate(){ 
	return mPosition; 
}
ci::Quatf Gizmo::getRotate(){ 
	return mRotations; 
}
ci::Vec3f Gizmo::getScale(){ 
	return mScale; 
}
ci::Matrix44f Gizmo::getTransform(){
    return mTransform;
}

void Gizmo::decompose (){
    // extract translation
    mPosition.x = mTransform.at(0, 3);
    mPosition.y = mTransform.at(1, 3);
    mPosition.z = mTransform.at(2, 3);
    
    // extract the rows of the matrix
    
    ci::Vec3f columns[3] = {
        mTransform.getColumn(0).xyz(),
        mTransform.getColumn(1).xyz(),
        mTransform.getColumn(2).xyz()
    };
    
    // extract the scaling factors
    mScale.x = columns[0].length();
    mScale.y = columns[1].length();
    mScale.z = columns[2].length();
    
    // and remove all scaling from the matrix
    if(mScale.x)
    {
        columns[0] /= mScale.x;
    }
    if(mScale.y)
    {
        columns[1] /= mScale.y;
    }
    if(mScale.z)
    {
        columns[2] /= mScale.z;
    }
    
    // build a 3x3 rotation matrix
    ci::Matrix33f m(columns[0].x,columns[1].x,columns[2].x,
                columns[0].y,columns[1].y,columns[2].y,
                columns[0].z,columns[1].z,columns[2].z, true);
    
    // and generate the rotation quaternion from it
    mRotations = ci::Quatf(m);
}

void Gizmo::setMode( int mode ){
    mCurrentMode = mode;
}


void Gizmo::registerEvents(){
    mCallbackIds.push_back( ci::app::App::get()->registerMouseDown( this, &Gizmo::mouseDown ) );
    mCallbackIds.push_back( ci::app::App::get()->registerMouseMove( this, &Gizmo::mouseMove ) );
    mCallbackIds.push_back( ci::app::App::get()->registerMouseDrag( this, &Gizmo::mouseDrag ) );
    mCallbackIds.push_back( ci::app::App::get()->registerResize( this, &Gizmo::resize ) );
}
void Gizmo::unregisterEvents(){
    if( mCallbackIds.size() ){
        ci::app::App::get()->unregisterMouseDown(	mCallbackIds[ 0 ] );
        ci::app::App::get()->unregisterMouseMove(	mCallbackIds[ 1 ] );
        ci::app::App::get()->unregisterMouseDrag(	mCallbackIds[ 2 ] );
        ci::app::App::get()->unregisterResize(      mCallbackIds[ 3 ] );
    }
}

bool Gizmo::mouseDown( ci::app::MouseEvent event ){
    
    
    // If rotating use Arcball instead of the raycasting trick
    if( mCurrentMode == ROTATE ){
        switch( mSelectedAxis ){
            case 0: mArcball.setConstraintAxis( mRotations * -ci::Vec3f::yAxis() ); break;
            case 1: mArcball.setConstraintAxis( mRotations * ci::Vec3f::xAxis() ); break;
            case 2: mArcball.setConstraintAxis( mRotations * ci::Vec3f::zAxis() ); break;
            default: mArcball.setNoConstraintAxis(); break;
        }
        mArcball.mouseDown( event.getPos() );
    }
    // Scale or rotate
    else{
        
        // Find the plane for the selected axis
        ci::Planef plane;
        switch( mSelectedAxis ){
            case 0: plane = ci::Planef( mPosition, ci::Vec3f::yAxis() ); break;
            case 1: plane = ci::Planef( mPosition, ci::Vec3f::zAxis() ); break;
            case 2: plane = ci::Planef( mPosition, ci::Vec3f::yAxis() ); break;
            default: return false;
        }
        
        // Cast a ray from the camera
        ci::Ray ray = mCurrentCam.generateRay( event.getPos().x / (float) mWindowSize.getWidth(), 1.0f - event.getPos().y / (float) mWindowSize.getHeight(), mWindowSize.getWidth() / (float) mWindowSize.getHeight() );
        
        // And check if there's an intersection with the plane
        float intersectionDistance;
        bool intersect = ray.calcPlaneIntersection( plane.getPoint(), plane.getNormal(), &intersectionDistance );
        
        // Use it to get the mouse position in 3D
        if( intersect ){
            ci::Vec3f intersection = ray.getOrigin() + ray.getDirection() * intersectionDistance;
            mMousePos = intersection;
        }
    }
    
    
    return false;
}
bool Gizmo::mouseMove( ci::app::MouseEvent event ){
    mSelectedAxis = samplePosition( (float) event.getPos().x / (float) ci::app::getWindowWidth() * (float) mPositionFbo.getWidth(), (float) event.getPos().y  / (float) ci::app::getWindowHeight() * (float) mPositionFbo.getHeight() );
    
    mCanRotate = false;
    if( mSelectedAxis != -1 || ( mSelectedAxis == -1 && mCurrentMode == ROTATE ) ){
        // Check if inside rotation center
        if( ( event.getPos() - mCurrentCam.worldToScreen( mPosition, mWindowSize.getWidth(), mWindowSize.getHeight() ) ).length() < 100.0f ){
            mCanRotate = true;
        }
    }
    return false;
}

bool Gizmo::mouseDrag( ci::app::MouseEvent event ){           
    
    // If rotating use Arcball instead of the raycasting trick
    if( mCurrentMode == ROTATE && mCanRotate ){
        mArcball.mouseDrag( event.getPos() );
        mRotations = mArcball.getQuat();
        transform();
    }
    
    // Scale or rotate
    else{
        
        // Find the plane and the current axis
        ci::Vec3f currentAxis;
        ci::Planef currentPlane;
        switch( mSelectedAxis ){
            case 0: currentAxis = ci::Vec3f::xAxis(); currentPlane = ci::Planef( ci::Vec3f::zero(), ci::Vec3f::yAxis() ); break;
            case 1: currentAxis = ci::Vec3f::yAxis(); currentPlane = ci::Planef( ci::Vec3f::zero(), ci::Vec3f::zAxis() ); break;
            case 2: currentAxis = ci::Vec3f::zAxis(); currentPlane = ci::Planef( ci::Vec3f::zero(), ci::Vec3f::yAxis() ); break;
            default: return false;
        }
        
        // Cast a ray from the camera
        float intersectionDistance;
        ci::Ray ray = mCurrentCam.generateRay( event.getPos().x / (float) mWindowSize.getWidth(), 1.0f - event.getPos().y / (float) mWindowSize.getHeight(), mWindowSize.getWidth() / (float) mWindowSize.getHeight() );
        
        // Transform the plane point and normal so it relfects our rotations
        bool intersect = ray.calcPlaneIntersection( mPosition + mRotations.toMatrix33() * currentPlane.getPoint(), mRotations.toMatrix33() * currentPlane.getNormal(), &intersectionDistance );
        
        // And check if there's an intersection with the plane
        if( intersect ){
            
            // Use that to move, rotate or scale 
            ci::Vec3f intersection = ray.getOrigin() + ray.getDirection() * intersectionDistance;
            ci::Vec3f diff = ( intersection - mMousePos );
            if( diff.length() < 50.0f ){ 
                diff *= currentAxis;
                
                if( mCurrentMode == TRANSLATE ){   
                    // Transform the translation to match the current rotations
                    mPosition -= mRotations.toMatrix33() * diff;
                }
                else if( mCurrentMode == SCALE ){
                    mScale += diff * 0.01f;
                }
                
                transform();
            }
            
            // Keep the last mouse position
            mMousePos = intersection;
        }            
    }
    
    return false;  
}

bool Gizmo::resize( ci::app::ResizeEvent event ){
    mWindowSize = ci::Rectf( 0, 0, event.getSize().x, event.getSize().y );
    return false;
}

Gizmo::Gizmo(){
}

void Gizmo::drawTranslate( ci::ColorA xColor, ci::ColorA yColor, ci::ColorA zColor ) {
    float axisLength = 30.0f;
    float headLength = 6.0f; 
    float headRadius = 1.5f;
    
    ci::gl::color( xColor );
    ci::gl::drawVector( ci::Vec3f::zero(), ci::Vec3f::xAxis() * axisLength, headLength, headRadius );
    ci::gl::color( yColor );
    ci::gl::drawVector( ci::Vec3f::zero(), ci::Vec3f::yAxis() * axisLength, headLength, headRadius );
    ci::gl::color( zColor );
    ci::gl::drawVector( ci::Vec3f::zero(), ci::Vec3f::zAxis() * axisLength, headLength, headRadius );
}
void Gizmo::drawRotate( ci::ColorA xColor, ci::ColorA yColor, ci::ColorA zColor ){
    float axisLength = 30.0f;
    float radius = 2.0f; 
    float slices = 30;
    
    glEnable( GL_CULL_FACE );
    glCullFace( GL_BACK );
    
    ci::gl::color( xColor );
    ci::gl::drawCylinder( axisLength, axisLength, radius, slices );
    
    ci::gl::color( yColor );
    ci::gl::pushModelView();
    ci::gl::rotate( ci::Vec3f::zAxis() * 90 );
    ci::gl::drawCylinder( axisLength, axisLength, radius, slices );
    ci::gl::popModelView();
    
    ci::gl::color( zColor );
    ci::gl::pushModelView();
    ci::gl::rotate( ci::Vec3f::xAxis() * 90 );
    ci::gl::drawCylinder( axisLength, axisLength, radius, slices );
    ci::gl::popModelView();
    
    glDisable( GL_CULL_FACE );
    
    ci::gl::pushMatrices();
    ci::gl::color( 0.3f, 0.3f, 0.3f );
    ci::gl::setMatricesWindow( ci::Vec2i( mWindowSize.getWidth(), mWindowSize.getHeight() ) );
    ci::gl::drawStrokedCircle( mCurrentCam.worldToScreen( mPosition, mWindowSize.getWidth(), mWindowSize.getHeight() ), 100 );
    ci::gl::popMatrices();
    
}
void Gizmo::drawScale( ci::ColorA xColor, ci::ColorA yColor, ci::ColorA zColor ){
    float axisLength = 30.0f;
    ci::Vec3f handleSize = ci::Vec3f( 3.0f, 3.0f, 3.0f );
    
    ci::gl::color( xColor );
    ci::gl::drawLine( ci::Vec3f::zero(), ci::Vec3f::xAxis() * axisLength );
    ci::gl::drawCube( ci::Vec3f::xAxis() * axisLength, handleSize );
    ci::gl::color( yColor );
    ci::gl::drawLine( ci::Vec3f::zero(), ci::Vec3f::yAxis() * axisLength );
    ci::gl::drawCube( ci::Vec3f::yAxis() * axisLength, handleSize );
    ci::gl::color( zColor );
    ci::gl::drawLine( ci::Vec3f::zero(), ci::Vec3f::zAxis() * axisLength );
    ci::gl::drawCube( ci::Vec3f::zAxis() * axisLength, handleSize ); 
}


int Gizmo::samplePosition( int x, int y ){
    
    y  = mPositionFbo.getHeight() - y;
    
    // Copy Cursor Neighbors to the cursor Fbo
    
    mPositionFbo.blitTo( mCursorFbo, ci::Area( x-5, y-5, x+5, y+5), mCursorFbo.getBounds());
    
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


ci::ColorA Gizmo::RED = ci::ColorA( 1.0f, 0.0f, 0.0f, 1.0f);
ci::ColorA Gizmo::GREEN = ci::ColorA( 0.0f, 1.0f, 0.0f, 1.0f);
ci::ColorA Gizmo::BLUE = ci::ColorA( 0.0f, 0.0f, 1.0f, 1.0f);
ci::ColorA Gizmo::YELLOW = ci::ColorA( 1.0f, 1.0f, 0.0f, 1.0f);
