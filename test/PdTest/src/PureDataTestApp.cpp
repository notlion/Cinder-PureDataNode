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
	void processDrag( Vec2i pos );
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
	mPureDataNode = ctx->makeNode( new PureDataNode() );

	setupBasic();
	setupUI();

	ctx->enable();

	ctx->printGraph();
}

void PureDataTestApp::setupBasic()
{
	mPureDataNode >> audio2::master()->getOutput();

	mPatch = mPureDataNode->loadPatch( loadResource( RES_BASIC_PD_PATCH ) );
	CI_LOG_V( "loaded patch: " << mPatch->filename() );
}

void PureDataTestApp::setupFileInput()
{
	auto ctx = audio2::master();
	mSourceFile = audio2::load( loadResource( "cash_satisfied_mind.mp3" ), ctx->getSampleRate() );

	mPlayerNode = ctx->makeNode( new audio2::BufferPlayer( mSourceFile->loadBuffer() ) );
	mPlayerNode->setLoopEnabled();
	CI_LOG_V( "BufferPlayerNode frames: " << mPlayerNode->getNumFrames() );

	mPlayerNode >> mPureDataNode >> ctx->getOutput();

	mPatch = mPureDataNode->loadPatch( loadResource( RES_INPUT_PD_PATCH ) );
	CI_LOG_V( "loaded patch: " << mPatch->filename() );
}

void PureDataTestApp::setupUI()
{
	mPlayButton = Button( true, "stopped", "playing" );
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
	getWindow()->getSignalMouseDrag().connect( [this] ( MouseEvent &event ) { processDrag( event.getPos() ); } );
	getWindow()->getSignalTouchesBegan().connect( [this] ( TouchEvent &event ) { processTap( event.getTouches().front().getPos() ); } );
	getWindow()->getSignalTouchesMoved().connect( [this] ( TouchEvent &event ) {
		for( const TouchEvent::Touch &touch : getActiveTouches() )
			processDrag( touch.getPos() );
	} );

	gl::enableAlphaBlending();
}

void PureDataTestApp::processDrag( Vec2i pos )
{
}

void PureDataTestApp::processTap( Vec2i pos )
{
	if( mPlayButton.hitTest( pos ) ) {
		if( mPlayerNode )
			mPlayerNode->setEnabled( mPlayButton.mEnabled );

		mPureDataNode->setEnabled( mPlayButton.mEnabled );
	}

	size_t currentIndex = mTestSelector.mCurrentSectionIndex;
	if( mTestSelector.hitTest( pos ) && currentIndex != mTestSelector.mCurrentSectionIndex ) {
		string currentTest = mTestSelector.currentSection();
		CI_LOG_V( "selected: " << currentTest );

		auto ctx = audio2::master();

		audio2::ScopedEnableContext scopedEnable( ctx, false ); // TODO: make work without this

		ctx->disconnectAllNodes();

		if( mPatch ) {
			mPureDataNode->getPd().closePatch( *mPatch );
			mPatch.reset();
		}

		if( currentTest == "basic" )
			setupBasic();
		if( currentTest == "input" ) {
			setupFileInput();
			mPlayerNode->setEnabled( mPlayButton.mEnabled );
			mPureDataNode->setEnabled( mPlayButton.mEnabled );
		}
	}
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
