#include "PureDataNode.h"

#include "cinder/audio/Context.h"
#include "cinder/audio/dsp/Converter.h"
#include "cinder/Log.h"

using namespace ci;

namespace cipd {

PureDataNode::PureDataNode(const Format &format) : Node{ format }, mAudioTasks{ 64 } {
  if (getChannelMode() != ChannelMode::SPECIFIED) {
    setChannelMode(ChannelMode::SPECIFIED);
    setNumChannels(2);
  }
}

void PureDataNode::initialize() {
  mPdBase.setReceiver(this);
  mPdBase.setMidiReceiver(this);
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
  {
    Task task;
    for (bool success = true; success;) {
      success = mAudioTasks.try_dequeue(task);
      if (success) task(mPdBase);
    }
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
  mPdBase.receiveMidi();
}

std::future<PatchRef> PureDataNode::loadPatch(const ci::DataSourceRef &dataSource) {
  if (!isInitialized()) {
    getContext()->initializeNode(shared_from_this());
  }

  const fs::path &path = dataSource->getFilePath();

  return queueTaskWithReturn([path](pd::PdBase &pd) {
    auto patch = pd.openPatch(path.filename().string(), path.parent_path().string());
    if (!patch.isValid()) {
      CI_LOG_E("Could not open patch from dataSource: " << path);
      return PatchRef();
    }
    return std::make_shared<pd::Patch>(std::move(patch));
  });
}

void PureDataNode::closePatch(const PatchRef &patch) {
  if (patch) {
    queueTask([=](pd::PdBase &pd) { pd.closePatch(*patch); });
  }
}

void PureDataNode::queueTask(Task &&task) {
  mAudioTasks.enqueue(std::move(task));
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

void PureDataNode::sendSymbol(const std::string &dest, const std::string &symbol) {
  queueTask([=](pd::PdBase &pd) { pd.sendSymbol(dest, symbol); });
}

void PureDataNode::sendList(const std::string &dest, const pd::List &list) {
  queueTask([=](pd::PdBase &pd) { pd.sendList(dest, list); });
}

void PureDataNode::sendMessage(const std::string &dest, const std::string &msg,
                               const pd::List &list) {
  queueTask([=](pd::PdBase &pd) { pd.sendMessage(dest, msg, list); });
}

std::future<std::vector<float>> PureDataNode::readArray(const std::string &arrayName, int readLen,
                                                        int offset) {
  return queueTaskWithReturn([=](pd::PdBase &pd) {
    std::vector<float> dest;
    pd.readArray(arrayName, dest, readLen, offset);
    return dest;
  });
}

void PureDataNode::writeArray(const std::string &name, const std::vector<float> &source, int length,
                              int offset) {
  queueTask([=](pd::PdBase &pd) {
    // NOTE(ryan): source is internal to this lambda, so it's okay to cast away the const-ness.
    pd.writeArray(name, const_cast<std::vector<float> &>(source), length, offset);
  });
}

void PureDataNode::clearArray(const std::string &name, int value) {
  queueTask([=](pd::PdBase &pd) { pd.clearArray(name, value); });
}

void PureDataNode::print(const std::string &message) {
  CI_LOG_I(message);
}

void PureDataNode::receiveFloat(const std::string &address, float value) {
  mMessages.enqueue({ Message::kTypeFloat, address, value });
}

void PureDataNode::receiveBang(const std::string &address) {
  mMessages.enqueue({ Message::kTypeBang, address });
}

void PureDataNode::receiveNoteOn(const int channel, const int pitch, const int velocity) {
  mMidiNotes.enqueue({ channel, pitch, velocity });
}

void PureDataNode::receiveAll(pd::PdReceiver &receiver) {
  Message msg;
  for (bool success = true; success;) {
    success = mMessages.try_dequeue(msg);
    if (success) {
      switch (msg.type) {
        case Message::kTypeBang: {
          receiver.receiveBang(msg.address);
        } break;
        case Message::kTypeFloat: {
          receiver.receiveFloat(msg.address, msg.value);
        } break;
      }
    }
  }
}

void PureDataNode::receiveAllMidi(pd::PdMidiReceiver &receiver) {
  MidiNote note;
  for (bool success = true; success;) {
    success = mMidiNotes.try_dequeue(note);
    if (success) {
      receiver.receiveNoteOn(note.channel, note.pitch, note.velocity);
    }
  }
}

} // namespace cipd
