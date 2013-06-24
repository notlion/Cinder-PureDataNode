#include "PdNode.h"

#include "audio2/Debug.h"

using namespace std;
using namespace ci;

PdNode::PdNode( const Format &format )
: Node( format )
{
	mTag = "PdNode";
	mBufferLayout = audio2::Buffer::Layout::Interleaved;
	if( mNumChannelsUnspecified )
		setNumChannels( 2 );
}

PdNode::~PdNode()
{
}

void PdNode::initialize()
{
	mNumTicksPerBlock = getContext()->getNumFramesPerBlock() / pd::PdBase::blockSize();

	lock_guard<mutex> lock( mMutex );

	bool success = mPdBase.init( getNumChannels(), getNumChannels(), getContext()->getSampleRate() );
	CI_ASSERT( success );

	// in libpd world, dsp computation is controlled through the process methods, so computeAudio is enabled until uninitialize
	mPdBase.computeAudio( true );
	mInitialized = true;
}

void PdNode::uninitialize()
{
	lock_guard<mutex> lock( mMutex );

	mPdBase.computeAudio( false );
	mInitialized = false;
}

void PdNode::start()
{

	mEnabled = true;
}

void PdNode::stop()
{

	mEnabled = false;
}

void PdNode::process( audio2::Buffer *buffer )
{
	CI_ASSERT( buffer->getLayout() == audio2::Buffer::Layout::Interleaved );

	mMutex.lock();
	mPdBase.processFloat( mNumTicksPerBlock, buffer->getData(), buffer->getData() );
	mMutex.unlock();
}

PatchRef PdNode::loadPatch( ci::DataSourceRef dataSource )
{
	CI_ASSERT_MSG( mInitialized, "PdNode must be initialized before opening a patch" );

	lock_guard<mutex> lock(mMutex);

	const fs::path& path = dataSource->getFilePath();
	pd::Patch patch = mPdBase.openPatch( path.filename().string(), path.parent_path().string() );
	if( ! patch.isValid() ) {
		LOG_V << "could not open patch from dataSource: " << path << endl;
		return PatchRef();
	}
	return PatchRef( new pd::Patch( patch ) );
}

void PdNode::sendBang( const std::string& dest )
{
	lock_guard<mutex> lock( mMutex );
	mPdBase.sendBang( dest );
}

void PdNode::sendFloat( const std::string& dest, float value )
{
	lock_guard<mutex> lock( mMutex );
	mPdBase.sendFloat( dest, value );
}

void PdNode::sendSymbol( const std::string& dest, const std::string& symbol )
{
	lock_guard<mutex> lock( mMutex );
	mPdBase.sendSymbol( dest, symbol );
}

void PdNode::sendList( const std::string& dest, const pd::List& list )
{
	lock_guard<mutex> lock( mMutex );
	mPdBase.sendList( dest, list );
}

void PdNode::sendMessage( const std::string& dest, const std::string& msg, const pd::List& list )
{
	lock_guard<mutex> lock( mMutex );
	mPdBase.sendMessage( dest, msg, list );
}
