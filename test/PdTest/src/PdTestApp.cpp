#include "cinder/app/AppNative.h"
#include "cinder/gl/gl.h"
#include "Resources.h"

#include "audio2/audio.h"
#include "audio2/GeneratorNode.h"
#include "audio2/Debug.h"
#include "Gui.h"

#include "PdNode.h"

using namespace ci;
using namespace ci::app;
using namespace std;

using namespace audio2;

class PdTestApp : public AppNative {
  public:
	void setup();
	void update();
	void draw();

	void setupUI();
	void processDrag( Vec2i pos );
	void processTap( Vec2i pos );

	void initContext();
	void setupBasic();
	void setupFileInput();

	ContextRef mContext;
	PdNodeRef mPdNode;
	PatchRef mPatch;

	SourceFileRef mSourceFile;
	BufferPlayerNodeRef mPlayerNode;

	VSelector mTestSelector;
	Button mPlayButton;
	vector<TestWidget *> mWidgets;
};

void PdTestApp::setup()
{
	mContext = Context::instance()->createContext();
	mPdNode = make_shared<PdNode>();

	setupBasic();
	setupUI();

	mContext->start();
}

void PdTestApp::initContext()
{
	LOG_V << "----------- Graph configuration: (before) --------------" << endl;
	printGraph( mContext );

	mContext->initialize();

	LOG_V << "----------- Graph configuration: (after) --------------" << endl;
	console() << "Graph configuration: (after)" << endl;
	printGraph( mContext );
}

void PdTestApp::setupBasic()
{
	mPdNode->connect( mContext->getRoot() );
	initContext();

	mPatch = mPdNode->loadPatch( loadResource( RES_BASIC_PD_PATCH ) );
	LOG_V << "loaded patch: " << mPatch->filename() << endl;
}

void PdTestApp::setupFileInput()
{
	mSourceFile = SourceFile::create( loadResource( "cash_satisfied_mind.mp3" ), 0, 44100 );

	mPlayerNode = make_shared<BufferPlayerNode>( mSourceFile->loadBuffer() );
	mPlayerNode->setLoop();
	LOG_V << "BufferPlayerNode frames: " << mPlayerNode->getNumFrames() << endl;

	mPlayerNode->connect( mPdNode )->connect( mContext->getRoot() );
	initContext();

	mPatch = mPdNode->loadPatch( loadResource( RES_INPUT_PD_PATCH ) );
	LOG_V << "loaded patch: " << mPatch->filename() << endl;
}

void PdTestApp::setupUI()
{
	mPlayButton = Button( true, "stopped", "playing" );
	mWidgets.push_back( &mPlayButton );

	mTestSelector.segments = { "basic", "input" };
	mWidgets.push_back( &mTestSelector );

#if defined( CINDER_COCOA_TOUCH )
	mPlayButton.bounds = Rectf( 0, 0, 120, 60 );
	mPlayButton.textIsCentered = false;
	mTestSelector.bounds = Rectf( getWindowWidth() - 190, 0.0f, getWindowWidth(), 160.0f );
	mTestSelector.textIsCentered = false;
#else
	mPlayButton.bounds = Rectf( 0, 0, 200, 60 );
	mTestSelector.bounds = Rectf( getWindowCenter().x + 100, 0.0f, getWindowWidth(), 160.0f );
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

void PdTestApp::processDrag( Vec2i pos )
{
}

void PdTestApp::processTap( Vec2i pos )
{
	if( mPlayButton.hitTest( pos ) ) {
		if( mPlayerNode )
			mPlayerNode->setEnabled( mPlayButton.enabled );
		mPdNode->setEnabled( mPlayButton.enabled );
	}

	size_t currentIndex = mTestSelector.currentSectionIndex;
	if( mTestSelector.hitTest( pos ) && currentIndex != mTestSelector.currentSectionIndex ) {
		string currentTest = mTestSelector.currentSection();
		LOG_V << "selected: " << currentTest << endl;

		bool running = mContext->isEnabled();
		mContext->uninitialize();

		if( mPatch ) {
			mPdNode->getPd().closePatch( *mPatch );
			mPatch.reset();
		}

		if( currentTest == "basic" )
			setupBasic();
		if( currentTest == "input" ) {
			setupFileInput();
			mPlayerNode->setEnabled( mPlayButton.enabled );
			mPdNode->setEnabled( mPlayButton.enabled );
		}

		if( running )
			mContext->start();
	}
}

void PdTestApp::update()
{
}

void PdTestApp::draw()
{
	gl::clear();
	drawWidgets( mWidgets );
}

CINDER_APP_NATIVE( PdTestApp, RendererGl )
