#pragma once

#include "PdBase.hpp"
#include "PdReceiver.hpp"
#include "PdTypes.hpp"

#include "cinder/audio/Node.h"
#include "cinder/Thread.h"

#include <queue>

namespace cipd {

namespace detail {

template <typename T>
class LockedQueue {
  std::queue<T> mQueue;
  mutable std::mutex mMutex;

public:
  void push(const T &item) {
    std::lock_guard<std::mutex> lock(mMutex);
    mQueue.push(item);
  }

  template <typename... Args>
  void emplace(Args &&... args) {
    std::lock_guard<std::mutex> lock(mMutex);
    mQueue.emplace(std::forward<Args>(args)...);
  }

  T pop() {
    std::lock_guard<std::mutex> lock(mMutex);
    auto item = std::move(mQueue.front());
    mQueue.pop();
    return item;
  }

  bool empty() const {
    std::lock_guard<std::mutex> lock(mMutex);
    return mQueue.empty();
  }
};

} // detail

typedef std::shared_ptr<pd::Patch> PatchRef;
typedef std::shared_ptr<class PureDataNode> PureDataNodeRef;

class PureDataNode : public ci::audio::Node {
  pd::PdBase mPdBase;
  size_t mNumTicksPerBlock;

  ci::audio::BufferInterleaved mBufferInterleaved;

  using Task = std::function<void(pd::PdBase &)>;
  detail::LockedQueue<Task> mPendingTasks;

public:
  PureDataNode(const Format &format = Format());

  void initialize() override;
  void uninitialize() override;
  void process(ci::audio::Buffer *buffer);

//  pd::PdBase &getPd() {
//    return mPdBase;
//  }

  void loadPatch(ci::DataSourceRef dataSource);
  // void closePatch(const PatchRef &patch);

  void queueTask(Task &&task);

  void setReceiver(pd::PdReceiver* receiver);
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
  void writeArray(const std::string &name, std::vector<float> &source, int length = -1,
                  int offset = 0);
  void clearArray(const std::string &name, int value = 0);
};

} // namespace cipd
