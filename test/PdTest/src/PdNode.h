#pragma once

#include "PdBase.hpp"
#include "PdReceiver.hpp"
#include "PdTypes.hpp"

#include "audio2/Context.h"
#include "cinder/Thread.h"

typedef std::shared_ptr<pd::Patch> PatchRef;

class PdNode {
public:
	PdNode() {};
	~PdNode();
	void init();
	PatchRef loadPatch(const std::string& name);

	// thread-safe senders
	void sendBang(const std::string& dest);
	void sendFloat(const std::string& dest, float value);
	void sendSymbol(const std::string& dest, const std::string& symbol);
	void sendList(const std::string& dest, const pd::List& list);
	void sendMessage(const std::string& dest, const std::string& msg, const pd::List& list = pd::List());

private:

	pd::PdBase mPdBase;


	std::mutex mMutex;

	int mSampleRate;
	int mNumInputs;
	int mNumOutputs;
	
};
