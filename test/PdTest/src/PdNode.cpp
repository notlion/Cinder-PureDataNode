#include "PdNode.h"

#include "audio2/Debug.h"

using namespace std;
using namespace ci;

PdNode::PdNode()
: Node()
{
}

PdNode::~PdNode()
{
}

void PdNode::initialize()
{
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
	mMutex.lock();

	int ticks = ioSampleCount / pd::PdBase::blockSize();
	mPdBase.processFloat( ticks, buffer->getData(), buffer->getData() );

	mMutex.unlock();
}

PatchRef PdNode::loadPatch( const std::string& patchName )
{
	lock_guard<mutex> lock(mMutex);
	return PatchRef( new pd::Patch( mPdBase.openPatch( patchName, app::App::getResourcePath().string() ) ) );
}

//void PdNode::audioCallback(uint64_t inSampleOffset, uint32_t ioSampleCount, audio::Buffer32f *ioBuffer)
//{
//	mMutex.lock();
//	int ticks = ioSampleCount / PdBase::blockSize();
//	processFloat(ticks, ioBuffer->mData, ioBuffer->mData);
//	mMutex.unlock();
//}

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
