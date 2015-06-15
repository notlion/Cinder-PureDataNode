#include "PureDataNode.h"

#include "cinder/audio/Context.h"
#include "cinder/audio/dsp/Converter.h"
#include "cinder/Log.h"

using namespace ci;

namespace cipd {

PureDataNode::PureDataNode(const Format &format) : Node(format) {
  if (getChannelMode() != ChannelMode::SPECIFIED) {
    setChannelMode(ChannelMode::SPECIFIED);
    setNumChannels(2);
  }
}

void PureDataNode::initialize() {
  mNumTicksPerBlock = getFramesPerBlock() / pd::PdBase::blockSize();

  auto numChannels = getNumChannels();
  auto sampleRate = getSampleRate();

  if (numChannels > 1) {
    mBufferInterleaved = audio::BufferInterleaved(getFramesPerBlock(), numChannels);
  }

  queueTask([=](pd::PdBase &pd) {
    bool success = pd.init(numChannels, numChannels, sampleRate);
    CI_ASSERT(success);

    // in libpd world, dsp computation is controlled through the process methods,
    // so computeAudio is enabled until uninitialize.
    pd.computeAudio(true);
  });
}

void PureDataNode::uninitialize() {
  queueTask([](pd::PdBase &pd) { pd.computeAudio(false); });
}

void PureDataNode::process(audio::Buffer *buffer) {
  while (!mPendingTasks.empty()) {
    mPendingTasks.pop()(mPdBase);
  }

  if (getNumChannels() > 1) {
    audio::dsp::interleaveBuffer(buffer, &mBufferInterleaved);

    mPdBase.processFloat(mNumTicksPerBlock, mBufferInterleaved.getData(),
                         mBufferInterleaved.getData());

    audio::dsp::deinterleaveBuffer(&mBufferInterleaved, buffer);
  } else {
    mPdBase.processFloat(mNumTicksPerBlock, buffer->getData(), buffer->getData());
  }

  mPdBase.receiveMessages();
}

void PureDataNode::loadPatch(ci::DataSourceRef dataSource) {
  if (!isInitialized()) {
    getContext()->initializeNode(shared_from_this());
  }

  const fs::path &path = dataSource->getFilePath();

  queueTask([=](pd::PdBase &pd) {
    auto patch = pd.openPatch(path.filename().string(), path.parent_path().string());
    if (!patch.isValid()) {
      CI_LOG_E("could not open patch from dataSource: " << path);
    }
  });
}

// void PureDataNode::closePatch(const PatchRef &patch) {
//   if (!patch) return;

//   lock_guard<mutex> lock(mMutex);
//   mPdBase.closePatch(*patch);
// }

void PureDataNode::queueTask(Task &&task) {
  mPendingTasks.emplace(task);
}

void PureDataNode::setReceiver(pd::PdReceiver* receiver) {
  queueTask([=](pd::PdBase &pd) { pd.setReceiver(receiver); });
}

void PureDataNode::subscribe(const std::string &address) {
  queueTask([=](pd::PdBase &pd) { pd.subscribe(address); });
}

void PureDataNode::sendBang(const std::string &dest) {
  queueTask([=](pd::PdBase &pd) { pd.sendBang(dest); });
}

void PureDataNode::sendFloat(const std::string &dest, float value) {
  queueTask([=](pd::PdBase &pd) { pd.sendFloat(dest, value); });
}

// void PureDataNode::sendSymbol(const std::string &dest, const std::string &symbol) {
//   queueTask([=](pd::PdBase &pd) { pd.sendSymbol(dest, symbol); });
// }

// void PureDataNode::sendList(const std::string &dest, const pd::List &list) {
//   lock_guard<mutex> lock(mMutex);
//   mPdBase.sendList(dest, list);
// }

// void PureDataNode::sendMessage(const std::string &dest, const std::string &msg,
//                                const pd::List &list) {
//   lock_guard<mutex> lock(mMutex);
//   mPdBase.sendMessage(dest, msg, list);
// }

// bool PureDataNode::readArray(const std::string &arrayName, std::vector<float> &dest, int readLen,
//                              int offset) {
//   lock_guard<mutex> lock(mMutex);
//   return mPdBase.readArray(arrayName, dest, readLen, offset);
// }

void PureDataNode::writeArray(const std::string &name, std::vector<float> &source, int length,
                              int offset) {
  queueTask([=](pd::PdBase &pd) mutable {
    auto success = pd.writeArray(name, source, length, offset);
  });
}

void PureDataNode::clearArray(const std::string &name, int value) {
  queueTask([=](pd::PdBase &pd) { pd.clearArray(name, value); });
}

} // namespace cipd
