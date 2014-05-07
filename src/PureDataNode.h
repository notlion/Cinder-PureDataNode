#pragma once

#include "PdBase.hpp"
#include "PdReceiver.hpp"
#include "PdTypes.hpp"

#include "cinder/audio2/Node.h"
#include "cinder/Thread.h"
#include "cinder/Datasource.h"

namespace cipd {

typedef std::shared_ptr<pd::Patch> PatchRef;
typedef std::shared_ptr<class PureDataNode> PureDataNodeRef;

class PureDataNode : public ci::audio2::Node {
public:
	PureDataNode( const Format &format = Format() );
	~PureDataNode();

	void initialize() override;
	void uninitialize() override;
	void process( ci::audio2::Buffer *buffer );

	pd::PdBase& getPd()	{ return mPdBase; }

	PatchRef loadPatch( ci::DataSourceRef dataSource );
	bool closePatch(PatchRef patch);
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

	ci::audio2::BufferInterleaved mBufferInterleaved;
};

} // namespace cipd