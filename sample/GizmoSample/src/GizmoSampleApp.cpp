#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/MayaCamUI.h"

#include "Gizmo.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class GizmoSampleApp : public AppBasic {
  public:
	void setup();
	void update();
	void draw();
    
	void mouseDown( MouseEvent event );	
	void mouseDrag( MouseEvent event );	
	void keyDown( KeyEvent event );	
    
    GizmoRef        mGizmo;
	MayaCamUI       mCamUI;
};

void GizmoSampleApp::setup()
{
    // Create a reference to our gizmo object 
    mGizmo = Gizmo::create( getWindowSize() );    
    
    // Create the cam interface
    CameraPersp cam;
    cam.setEyePoint( Vec3f( 0.0f, 300.0f, 500.0f ) );
	cam.setPerspective(50, getWindowWidth() / (float) getWindowHeight(), 1, 10000 );
	cam.setCenterOfInterestPoint( Vec3f::zero() );
	mCamUI.setCurrentCam( cam );
}

void GizmoSampleApp::update()
{
    // Update the gizmo with the current camera matrices
    mGizmo->setMatrices( mCamUI.getCamera() );
}

void GizmoSampleApp::draw()
{
	gl::clear( Color( 0, 0, 0 ) ); 
    
    gl::setMatricesWindowPersp( getWindowSize() );
    gl::setMatrices( mCamUI.getCamera() );
    gl::enableDepthRead();
    gl::enableDepthWrite();
    
    // Draw GizmoUI
    
    mGizmo->draw();
    
    
    // Move and draw a cube with it
    
    gl::pushModelView();
    gl::multModelView( mGizmo->getTransform() );
    gl::color( 1.0f, 1.0f, 1.0f );
    gl::drawStrokedCube( Vec3f::zero(), Vec3f( 100.0f, 100.0f, 100.0f ));
    gl::popModelView();
    
    
    // Draw XZ Plane

    float cellSize = 50.0f;
    gl::color( 0.1f, 0.1f, 0.1f );
    gl::enableWireframe();
    glBegin( GL_QUADS );
    for ( int x = -1000.0f; x < 1000.0f; x += cellSize ) {
        for ( int z = -1000.0f; z < 1000.0f; z += cellSize ) {
            glVertex3f( x - cellSize, 0.0f, z - cellSize );
            glVertex3f( x           , 0.0f, z - cellSize );
            glVertex3f( x           , 0.0f, z );
            glVertex3f( x - cellSize, 0.0f, z );
        }
    }
    glEnd();
    gl::disableWireframe();
}



void GizmoSampleApp::mouseDown( MouseEvent event ){
	if( event.isAltDown() )
		mCamUI.mouseDown( event.getPos() );
}

void GizmoSampleApp::mouseDrag( MouseEvent event ){
	if( event.isAltDown() )
		mCamUI.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
}

void GizmoSampleApp::keyDown( KeyEvent event ){
    if( event.getChar() == '1' ) mGizmo->setMode( Gizmo::TRANSLATE );
    else if( event.getChar() == '2' ) mGizmo->setMode( Gizmo::ROTATE );
    else if( event.getChar() == '3' ) mGizmo->setMode( Gizmo::SCALE );
    else if( event.getChar() == 'o' ) {
        CameraPersp centered = mCamUI.getCamera();
        centered.setCenterOfInterestPoint( mGizmo->getTranslate() );
        mCamUI.setCurrentCam( centered );
    }
}
CINDER_APP_BASIC( GizmoSampleApp, RendererGl )
