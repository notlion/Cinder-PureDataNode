#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"

#include "audio2/audio.h"

#include "cpp/PdBase.hpp"

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

	pd::PdBase mPd;
};

void PdTestApp::setup()
{
	mContext = Context::instance()->createContext();
}

void PdTestApp::mouseDown( MouseEvent event )
{
}

void PdTestApp::update()
{
}

void PdTestApp::draw()
{
	gl::clear();
}

CINDER_APP_NATIVE( PdTestApp, RendererGl )
