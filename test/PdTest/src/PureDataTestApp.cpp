#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "Resources.h"

#include "cinder/audio/Context.h"
#include "cinder/audio/Source.h"
#include "cinder/audio/SamplePlayerNode.h"
#include "cinder/audio/Debug.h"

#include "../../../../../test/_audio/common/AudioTestGui.h"


#include "PureDataNode.h"

using namespace ci;
using namespace ci::app;
using namespace std;

class PureDataTestApp : public AppNative {
  public:
	void setup();
	void update();
	void draw();

	void setupUI();
	void processTap( Vec2i pos );

	void setupBasic();
	void setupFileInput();

	cipd::PureDataNodeRef	mPureDataNode;
	cipd::PatchRef			mPatch;

	audio::SourceFileRef		mSourceFile;
	audio::BufferPlayerNodeRef	mPlayerNode;

	VSelector mTestSelector;
	Button mPlayButton;
	vector<TestWidget *> mWidgets;
};

void PureDataTestApp::setup()
{
	auto ctx = audio::master();
	mPureDataNode = ctx->makeNode( new cipd::PureDataNode( audio::Node::Format().autoEnable() ) );

	setupBasic();

	ctx->enable();
	PRINT_GRAPH( ctx );

	setupUI();
}

void PureDataTestApp::setupBasic()
{
	mPureDataNode->disconnectAll();

	mPatch = mPureDataNode->loadPatch( loadResource( RES_BASIC_PD_PATCH ) );
	CI_LOG_V( "loaded patch: " << mPatch->filename() );

	mPureDataNode >> audio::master()->getOutput();
}

void PureDataTestApp::setupFileInput()
{
	mPureDataNode->disconnectAll();

	auto ctx = audio::master();
	mSourceFile = audio::load( loadResource( "cash_satisfied_mind.mp3" ), ctx->getSampleRate() );

	mPlayerNode = ctx->makeNode( new audio::BufferPlayerNode( mSourceFile->loadBuffer() ) );
	mPlayerNode->setLoopEnabled();
	CI_LOG_V( "BufferPlayerNode frames: " << mPlayerNode->getNumFrames() );

	mPatch = mPureDataNode->loadPatch( loadResource( RES_INPUT_PD_PATCH ) );
	CI_LOG_V( "loaded patch: " << mPatch->filename() );

	mPlayerNode >> mPureDataNode >> ctx->getOutput();
}

void PureDataTestApp::setupUI()
{
	mPlayButton = Button( true, "stopped", "playing" );
	mPlayButton.setEnabled( audio::master()->isEnabled() );
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

	PRINT_GRAPH( audio::master() );
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
