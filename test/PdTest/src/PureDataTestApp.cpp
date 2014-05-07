#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "Resources.h"

#include "cinder/audio2/Context.h"
#include "cinder/audio2/Source.h"
#include "cinder/audio2/SamplePlayer.h"
#include "cinder/audio2/Debug.h"
#include "cinder\params\Params.h"

#include "PureDataNode.h"
using namespace ci;
using namespace ci::app;
using namespace std;

class libpdTestApp : public AppNative {
public:
	void setup();
	void draw();
	void setupBasic();
	void setupFileInput();
	void touchOsc(Vec2i pos);

	params::InterfaceGlRef	mParams;

	cipd::PureDataNodeRef	mPureDataNode;
	cipd::PatchRef			mPatch;

	audio2::SourceFileRef	mSourceFile;
	audio2::BufferPlayerRef	mPlayerNode;

	string msg;
};

void libpdTestApp::setupFileInput(){
	mPureDataNode->closePatch(mPatch);
	mPureDataNode->disconnectAll();
	
	mSourceFile = audio2::load(loadAsset("DaisyBell1895.ogg"));
	auto ctx = audio2::master();
	mPlayerNode = ctx->makeNode(new audio2::BufferPlayer(mSourceFile->loadBuffer()));
	mPlayerNode->setLoopEnabled();
	mPlayerNode->start();
	CI_LOG_V("BufferPlayerNode frames: " << mPlayerNode->getNumFrames());
	
	mPatch->clear();
	mPatch = mPureDataNode->loadPatch(loadAsset("input.pd"));
	CI_LOG_V("loaded patch: " << mPatch->filename());

	mPlayerNode >> mPureDataNode >> ctx->getOutput();

	msg = "Ogg file playing from Audio2, routed to Pure-data patch, \n"
		"and then to output.\n"
		"Song: Unknown singer - Daisy bell (1895)\n"
		"https://archive.org/details/DaisyBell1895\n\n"
		"See setupFileInput() and 'input.pd' for more info";
}

void libpdTestApp::setupBasic()
{
	mPureDataNode->closePatch(mPatch);
	mPureDataNode->disconnectAll();
	if (mPureDataNode->isInitialized())
	{
		mPatch->clear();
	}
#if defined (CINDER_MSW)
	mPatch = mPureDataNode->loadPatch(loadAsset("basic.pd"));
#else
	mPatch = mPureDataNode->loadPatch(loadResource(RES_BASIC_PD_PATCH));
#endif
	CI_LOG_V("loaded patch: " << mPatch->filename());
	
	mPureDataNode >> audio2::master()->getOutput();
	getWindow()->getSignalMouseMove().connect([this](MouseEvent &event) { touchOsc(event.getPos()); });

	msg = "Basic output from Pure-data to master output through Audio2\n"
		"Cinder is sending data to patch via 'recive' pins\n"
		"Move your mouse to change sound!\n\n"
		"See setupBasic() and 'basic.pd' for more info!";
}

void libpdTestApp::touchOsc(Vec2i pos){
	mPureDataNode->sendFloat("freq", pos.x);
	mPureDataNode->sendFloat("amp", (float)pos.y / (float)getWindowHeight());
}

void libpdTestApp::setup()
{
	auto ctx = audio2::master();
	mPureDataNode = ctx->makeNode(new cipd::PureDataNode(audio2::Node::Format().autoEnable())); 
	
	setupBasic();
	
	audio2::master()->enable();
	audio2::master()->printGraph();

	mParams = params::InterfaceGl::create(getWindow(), "libpd sample", toPixels(Vec2i(200, 100)));
	mParams->addButton("Basic patch", std::bind(&libpdTestApp::setupBasic, this));
	mParams->addButton("Input patch", std::bind(&libpdTestApp::setupFileInput, this));
	setWindowSize(Vec2i(640, 200));
	gl::enableAlphaBlending();
}

void libpdTestApp::draw()
{
	gl::clear();
	mParams->draw();
	gl::drawString(msg, Vec2f(250, 25));
}

CINDER_APP_NATIVE(libpdTestApp, RendererGl)
