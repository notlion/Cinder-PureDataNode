#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "Resources.h"

#include "audio2/audio.h"
#include "audio2/Debug.h"

#include "PdNode.h"

using namespace ci;
using namespace ci::app;
using namespace std;

using namespace audio2;

class PdTestApp : public AppNative {
  public:
	void setup();
	void mouseDown( MouseEvent event );	
	void update();
	void draw();

	ContextRef mContext;
	PdNodeRef mPdNode;

};

void PdTestApp::setup()
{
	mContext = Context::instance()->createContext();

	mPdNode = make_shared<PdNode>();
	mPdNode->connect( mContext->getRoot() );

	LOG_V << "----------- Graph configuration: (before) --------------" << endl;
	printGraph( mContext );

	mContext->initialize();

	LOG_V << "----------- Graph configuration: (after) --------------" << endl;
	console() << "Graph configuration: (after)" << endl;
	printGraph( mContext );


	PatchRef patch = mPdNode->loadPatch( loadResource( RES_TEST_PD_PATCH ) );

	LOG_V << "loaded patch: " << patch->filename() << endl;

	mContext->start();
}

void PdTestApp::mouseDown( MouseEvent event )
{
	mPdNode->setEnabled( ! mPdNode->isEnabled() );
}

void PdTestApp::update()
{
}

void PdTestApp::draw()
{
	gl::clear();
}

CINDER_APP_NATIVE( PdTestApp, RendererGl )
