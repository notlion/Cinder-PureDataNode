#include "PureDataNode.h"

#include "cinder/Log.h"
#include "cinder/audio/Context.h"
#include "cinder/audio/dsp/Converter.h"

using namespace ci;

namespace cipd {

PureDataNode::PureDataNode(const Format &format)
: Node{ format }, mMaxMessagesToProcessPerBlock{ std::numeric_limits<size_t>::max() } {
  if (getChannelMode() != ChannelMode::SPECIFIED) {
    setChannelMode(ChannelMode::SPECIFIED);
    setNumChannels(2);
  }
}

void PureDataNode::initialize() {
  mPdBase.setReceiver(this);
  mNumTicksPerBlock = getFramesPerBlock() / pd::PdBase::blockSize();

  auto numChannels = getNumChannels();
  auto sampleRate = getSampleRate();

  if (numChannels > 1) {
    mBufferInterleaved = audio::BufferInterleaved(getFramesPerBlock(), numChannels);
  }

  queueTask([=](pd::PdBase &pd) {
    bool success = pd.init(int(numChannels), int(numChannels), int(sampleRate));
    CI_ASSERT(success);

    // in libpd world, dsp computation is controlled through the process methods,
    // so computeAudio is enabled until uninitialize.
    pd.computeAudio(true);
  });
}

void PureDataNode::uninitialize() {
  queueTask([](pd::PdBase &pd) { pd.computeAudio(false); });
}

void PureDataNode::setMaxMessagesToProcessPerBlock(size_t maxMessages) {
  mMaxMessagesToProcessPerBlock = maxMessages;
}

void PureDataNode::processQueueToAudio() {
  QueueItem item;
  bool success = true;

  for (size_t i = 0; success && i < mMaxMessagesToProcessPerBlock; ++i) {
    success = mQueueToAudio.try_dequeue(item);
    if (success) {
      switch (item.which()) {
        case 0:
          break;

        case 1: {
          (*boost::get<TaskPtr>(item))(mPdBase);
        } break;

        case 2: {
          const auto &msg = boost::get<BangMessage>(item);
          mPdBase.sendBang(msg.address);
        } break;

        case 3: {
          const auto &msg = boost::get<FloatMessage>(item);
          mPdBase.sendFloat(msg.address, msg.value);
        } break;

        case 4: {
          const auto &msg = boost::get<SymbolMessage>(item);
          mPdBase.sendSymbol(msg.address, msg.symbol);
        } break;

        case 5: {
          auto &write = boost::get<ArrayWrite>(item);
          mPdBase.writeArray(write.arrayName, write.array, write.length, write.offset);
        } break;

        case 6: {
          const auto &note = boost::get<Note>(item);
          mPdBase.sendNoteOn(note.channel, note.pitch, note.velocity);
        } break;

        case 7: {
          const auto &ctrl = boost::get<ControlChange>(item);
          mPdBase.sendControlChange(ctrl.channel, ctrl.controller, ctrl.value);
        } break;

        case 8: {
          const auto &pgm = boost::get<ProgramChange>(item);
          mPdBase.sendProgramChange(pgm.channel, pgm.value);
        } break;

        case 9: {
          const auto &pb = boost::get<PitchBend>(item);
          mPdBase.sendPitchBend(pb.channel, pb.value);
        } break;

        case 10: {
          const auto &at = boost::get<AfterTouch>(item);
          mPdBase.sendAftertouch(at.channel, at.value);
        } break;

        case 11: {
          const auto &pat = boost::get<PolyAfterTouch>(item);
          mPdBase.sendPolyAftertouch(pat.channel, pat.pitch, pat.value);
        } break;

        case 12: {
          const auto &midiByte = boost::get<MidiByte>(item);
          mPdBase.sendMidiByte(midiByte.port, midiByte.value);
        } break;

        case 13: {
          const auto &sys = boost::get<Sysex>(item);
          mPdBase.sendSysex(sys.port, sys.value);
        } break;

        case 14: {
          const auto &sysrt = boost::get<SysRealTime>(item);
          mPdBase.sendSysRealTime(sysrt.port, sysrt.value);
        } break;
      }
    }
  }
}

void PureDataNode::processAudio(audio::Buffer *buffer) {
  if (getNumChannels() > 1) {
    if (getNumConnectedInputs() > 0) {
      audio::dsp::interleaveBuffer(buffer, &mBufferInterleaved);
    }

    mPdBase.processFloat(
        int(mNumTicksPerBlock), mBufferInterleaved.getData(), mBufferInterleaved.getData());

    audio::dsp::deinterleaveBuffer(&mBufferInterleaved, buffer);
  }
  else {
    mPdBase.processFloat(int(mNumTicksPerBlock), buffer->getData(), buffer->getData());
  }
}

void PureDataNode::process(audio::Buffer *buffer) {
  processQueueToAudio();
  processAudio(buffer);
  mPdBase.receiveMessages();
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
  mQueueToAudio.enqueue(QueueItem(std::make_unique<Task>(std::move(task))));
}

void PureDataNode::subscribe(const std::string &address) {
  queueTask([=](pd::PdBase &pd) { pd.subscribe(address); });
}

void PureDataNode::sendBang(const std::string &dest) {
  mQueueToAudio.enqueue(QueueItem(BangMessage{ dest }));
}

void PureDataNode::sendFloat(const std::string &dest, float value) {
  mQueueToAudio.enqueue(QueueItem(FloatMessage{ dest, value }));
}

void PureDataNode::sendSymbol(const std::string &dest, const std::string &symbol) {
  mQueueToAudio.enqueue(QueueItem(SymbolMessage{ dest, symbol }));
}

void PureDataNode::sendList(const std::string &dest, const pd::List &list) {
  queueTask([=](pd::PdBase &pd) { pd.sendList(dest, list); });
}

void PureDataNode::sendMessage(const std::string &dest,
                               const std::string &msg,
                               const pd::List &list) {
  queueTask([=](pd::PdBase &pd) { pd.sendMessage(dest, msg, list); });
}

void PureDataNode::sendNoteOn(int channel, int pitch, int velocity) {
  mQueueToAudio.enqueue(QueueItem(Note{ channel, pitch, velocity }));
}

void PureDataNode::sendControlChange(int channel, int controller, int value) {
  mQueueToAudio.enqueue(QueueItem(ControlChange{ channel, controller, value }));
}

void PureDataNode::sendProgramChange(int channel, int value) {
  mQueueToAudio.enqueue(QueueItem(ProgramChange{ channel, value }));
}

void PureDataNode::sendPitchBend(int channel, int value) {
  mQueueToAudio.enqueue(QueueItem(PitchBend{ channel, value }));
}

void PureDataNode::sendAfterTouch(int channel, int value) {
  mQueueToAudio.enqueue(QueueItem(AfterTouch{ channel, value }));
}

void PureDataNode::sendPolyAfterTouch(int channel, int pitch, int value) {
  mQueueToAudio.enqueue(QueueItem(PolyAfterTouch{ channel, pitch, value }));
}

void PureDataNode::sendMidiByte(int port, int byte) {
  mQueueToAudio.enqueue(QueueItem(MidiByte{ port, byte }));
}

void PureDataNode::sendSysex(int port, int byte) {
  mQueueToAudio.enqueue(QueueItem(Sysex{ port, byte }));
}

void PureDataNode::sendSysRealTime(int port, int byte) {
  mQueueToAudio.enqueue(QueueItem(SysRealTime{ port, byte }));
}


std::future<std::vector<float>> PureDataNode::readArray(const std::string &arrayName,
                                                        int readLen,
                                                        int offset) {
  return queueTaskWithReturn([=](pd::PdBase &pd) {
    std::vector<float> dest;
    pd.readArray(arrayName, dest, readLen, offset);
    return dest;
  });
}

void PureDataNode::writeArray(const std::string &name,
                              const std::vector<float> &source,
                              int length,
                              int offset) {
  mQueueToAudio.enqueue(QueueItem(ArrayWrite{ name, source, offset, length }));
}

void PureDataNode::clearArray(const std::string &name, int value) {
  queueTask([=](pd::PdBase &pd) { pd.clearArray(name, value); });
}


void PureDataNode::print(const std::string &message) {
  CI_LOG_I(message);
}

void PureDataNode::receiveBang(const std::string &address) {
  mQueueFromAudio.enqueue(QueueItem(BangMessage{ address }));
}

void PureDataNode::receiveFloat(const std::string &address, float value) {
  mQueueFromAudio.enqueue(QueueItem(FloatMessage{ address, value }));
}

void PureDataNode::receiveSymbol(const std::string &address, const std::string &symbol) {
  mQueueFromAudio.enqueue(QueueItem(SymbolMessage{ address, symbol }));
}


void PureDataNode::receiveAll(pd::PdReceiver &receiver) {
  QueueItem item;
  for (bool success = true; success;) {
    success = mQueueFromAudio.try_dequeue(item);
    if (success) {
      switch (item.which()) {
        case 0:
          break;

        case 1:
          break;

        case 2: {
          const auto &msg = boost::get<BangMessage>(item);
          receiver.receiveBang(msg.address);
        } break;

        case 3: {
          const auto &msg = boost::get<FloatMessage>(item);
          receiver.receiveFloat(msg.address, msg.value);
        } break;

        case 4: {
          const auto &msg = boost::get<SymbolMessage>(item);
          receiver.receiveSymbol(msg.address, msg.symbol);
        } break;
      }
    }
  }
}

} // namespace cipd
