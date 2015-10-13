#pragma once

#include "PdBase.hpp"
#include "PdReceiver.hpp"
#include "PdTypes.hpp"

#include "cinder/audio/Node.h"
#include "cinder/Thread.h"
#include "cinder/DataSource.h"

#include "readerwriterqueue.h"

namespace cipd {

typedef std::shared_ptr<pd::Patch> PatchRef;
typedef std::shared_ptr<class PureDataNode> PureDataNodeRef;

class PureDataNode : public ci::audio::Node, public pd::PdReceiver {
  pd::PdBase mPdBase;
  size_t mNumTicksPerBlock;

  ci::audio::BufferInterleaved mBufferInterleaved;

  using Task = std::function<void(pd::PdBase &)>;
  moodycamel::ReaderWriterQueue<Task> mAudioTasks;

  struct Message {
    enum Type { kTypeBang, kTypeFloat } type;
    std::string address;
    float value;
  };
  moodycamel::ReaderWriterQueue<Message> mMessages;

public:
  PureDataNode(const Format &format = Format());

  void initialize() override;
  void uninitialize() override;

  void print(const std::string &message) override;
  void receiveFloat(const std::string &address, float value) override;
  void receiveBang(const std::string &address) override;

  void process(ci::audio::Buffer *buffer);

  void loadPatch(ci::DataSourceRef dataSource);
  // void closePatch(const PatchRef &patch);

  void queueTask(Task &&task);
  void subscribe(const std::string &address);

  // thread-safe senders
  void sendBang(const std::string &dest);
  void sendFloat(const std::string &dest, float value);
  // void sendSymbol(const std::string &dest, const std::string &symbol);
  // void sendList(const std::string &dest, const pd::List &list);
  // void sendMessage(const std::string &dest, const std::string &msg,
  //                  const pd::List &list = pd::List());

  // bool readArray(const std::string &arrayName, std::vector<float> &dest, int readLen = -1,
  //                int offset = 0);
  void writeArray(const std::string &name, std::vector<float> source, int length = -1,
                  int offset = 0);
  void clearArray(const std::string &name, int value = 0);

  void receiveAll(pd::PdReceiver &receiver);
};

} // namespace cipd
