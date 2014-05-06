#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "Resources.h"

#include "cinder/audio2/Context.h"
#include "cinder/audio2/Source.h"
#include "cinder/audio2/SamplePlayer.h"
#include "cinder/audio2/Debug.h"

#include "../test/common/AudioTestGui.h"


#include "PureDataNode.h"

using namespace ci;
using namespace ci::app;
using namespace std;

using namespace audio2;

class PureDataTestApp : public AppNative {
  public:
	void setup();
	void update();
	void draw();

	void setupUI();
	void processTap( Vec2i pos );

	void setupBasic();
	void setupFileInput();

	PureDataNodeRef mPureDataNode;
	PatchRef mPatch;

	audio2::SourceFileRef	mSourceFile;
	audio2::BufferPlayerRef	mPlayerNode;

	VSelector mTestSelector;
	Button mPlayButton;
	vector<TestWidget *> mWidgets;
};

void PureDataTestApp::setup()
{
	auto ctx = audio2::master();
	mPureDataNode = ctx->makeNode( new PureDataNode( audio2::Node::Format().autoEnable() ) );

	setupBasic();

	ctx->enable();
	ctx->printGraph();

	setupUI();
}

void PureDataTestApp::setupBasic()
{
	mPureDataNode->disconnectAll();

	mPatch = mPureDataNode->loadPatch( loadResource( RES_BASIC_PD_PATCH ) );
	CI_LOG_V( "loaded patch: " << mPatch->filename() );

	mPureDataNode >> audio2::master()->getOutput();
}

void PureDataTestApp::setupFileInput()
{
	mPureDataNode->disconnectAll();

	auto ctx = audio2::master();
	mSourceFile = audio2::load( loadResource( "cash_satisfied_mind.mp3" ), ctx->getSampleRate() );

	mPlayerNode = ctx->makeNode( new audio2::BufferPlayer( mSourceFile->loadBuffer() ) );
	mPlayerNode->setLoopEnabled();
	CI_LOG_V( "BufferPlayerNode frames: " << mPlayerNode->getNumFrames() );

	mPatch = mPureDataNode->loadPatch( loadResource( RES_INPUT_PD_PATCH ) );
	CI_LOG_V( "loaded patch: " << mPatch->filename() );

	mPlayerNode >> mPureDataNode >> ctx->getOutput();
}

void PureDataTestApp::setupUI()
{
	mPlayButton = Button( true, "stopped", "playing" );
	mPlayButton.setEnabled( audio2::master()->isEnabled() );
	mWidgets.push_back( &mPlayButton );

	mTestSelector.mSegments = { "sinetone", "file input" };
	mWidgets.push_back( &mTestSelector );

#if defined( CINDER_COCOA_TOUCH )
	mPlayButton.bounds = Rectf( 0, 0, 120, 60 );
	mPlayButton.textIsCentered = false;
	mTestSelector.bounds = Rectf( getWindowWidth() - 190, 0.0f, getWindowWidth(), 160.0f );
	mTestSelector.textIsCentered = false;
#else
	mPlayButton.mBounds = Rectf( 0, 0, 200, 60 );
	mTestSelector.mBounds = Rectf( getWindowCenter().x + 100, 0.0f, getWindowWidth(), 160.0f );
#endif

	getWindow()->getSignalMouseDown().connect( [this] ( MouseEvent &event ) { processTap( event.getPos() ); } );
	getWindow()->getSignalTouchesBegan().connect( [this] ( TouchEvent &event ) { processTap( event.getTouches().front().getPos() ); } );

	gl::enableAlphaBlending();
}

void PureDataTestApp::processTap( Vec2i pos )
{
	if( mPlayButton.hitTest( pos ) ) {
		if( mPlayerNode )
			mPlayerNode->setEnabled( mPlayButton.mEnabled );
	}

	size_t currentIndex = mTestSelector.mCurrentSectionIndex;
	if( mTestSelector.hitTest( pos ) && currentIndex != mTestSelector.mCurrentSectionIndex ) {
		string currentTest = mTestSelector.currentSection();
		CI_LOG_V( "selected: " << currentTest );

		if( mPatch ) {
			mPureDataNode->getPd().closePatch( *mPatch );
			mPatch.reset();
		}

		if( currentTest == "sinetone" )
			setupBasic();
		if( currentTest == "file input" ) {
			setupFileInput();
			mPlayerNode->setEnabled( mPlayButton.mEnabled );
			mPureDataNode->setEnabled( mPlayButton.mEnabled );
		}
	}

	audio2::master()->printGraph();
}

void PureDataTestApp::update()
{
}

void PureDataTestApp::draw()
{
	gl::clear();
	drawWidgets( mWidgets );
}

CINDER_APP_NATIVE( PureDataTestApp, RendererGl )
