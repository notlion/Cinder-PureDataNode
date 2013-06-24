#pragma once

#include "PdBase.hpp"
#include "PdReceiver.hpp"
#include "PdTypes.hpp"

#include "audio2/Context.h"
#include "cinder/Thread.h"

typedef std::shared_ptr<pd::Patch> PatchRef;
typedef std::shared_ptr<class PdNode> PdNodeRef;

class PdNode : public audio2::Node {
public:
	PdNode( const Format &format = Format() );
	~PdNode();

	void initialize() override;
	void uninitialize() override;
	void start() override;
	void stop() override;
	void process( audio2::Buffer *buffer );

	pd::PdBase& getPd()	{ return mPdBase; }

	PatchRef loadPatch( ci::DataSourceRef dataSource );

	// thread-safe senders
	void sendBang(const std::string& dest);
	void sendFloat(const std::string& dest, float value);
	void sendSymbol(const std::string& dest, const std::string& symbol);
	void sendList(const std::string& dest, const pd::List& list);
	void sendMessage(const std::string& dest, const std::string& msg, const pd::List& list = pd::List());

private:
	pd::PdBase	mPdBase;
	std::mutex	mMutex;
	size_t		mNumTicksPerBlock;
};
