#include "PdNode.h"

#include "audio2/Debug.h"

using namespace std;
using namespace ci;

PdNode::~PdNode()
{
}

// TODO: audio computation / play should be toggleable via play / stop
void PdNode::init()
{
	mSampleRate = 44100;
	mNumInputs = 0;
	mNumOutputs = 2;

	lock_guard<mutex> lock(mMutex);

	if( ! mPdBase.init( mNumInputs, mNumOutputs, mSampleRate ) ) {
		LOG_E << "pd init failed" << endl;
		return;
	}
	LOG_V << "pd init success";

	mPdBase.computeAudio(true);

//	mOutput = audio::Output::addTrack( audio::createCallback(this, &PdNode::audioCallback, false, mSampleRate, mNumOutputs) );
//	mOutput->play();
}

PatchRef PdNode::loadPatch(const std::string& patchName)
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
