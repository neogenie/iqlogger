//
// Record.h
// ~~~~~~~~~~~~~~~~
//
// Copyright (C) 2018 IQOption Software, Inc.
//
//

#pragma once

#include <memory>
#include <type_traits>

#include "Message.h"
#include "MessageQueue.h"
#include "core/Log.h"

namespace iqlogger::inputs {

template<class T>
class Record;

template<class T>
using RecordPtr = std::unique_ptr<Record<T>>;

template<class T>
class Record
{
  using RecordDataT = typename T::RecordDataT;
  using MessageT = typename T::MessageT;

  template<typename U, void* fake = nullptr>
  struct Source {
    U source;

    template<class W>
    explicit Source(W&& s) : source(std::forward<W>(s)){};
  };

  template<void* fake>
  class Source<void, fake>
  {};

  using SourceT = Source<typename T::SourceT>;

  std::string m_input;
  RecordDataT m_data;
  SourceT m_source;

public:
  // @TODO Refactor this

  template<typename U, typename ST = typename T::SourceT, typename = std::enable_if_t<std::is_same_v<ST, void>>>
  explicit Record(U&& input, RecordDataT&& data) : m_input(std::forward<U>(input)), m_data(std::move(data)) {
    TRACE("inputs::Record::Record()");
  }

  template<typename U, typename S, typename ST = typename T::SourceT,
           typename = std::enable_if_t<std::is_constructible_v<ST, std::decay_t<S>>>,
           typename = std::enable_if_t<!std::is_same_v<ST, void>>>
  explicit Record(U&& input, RecordDataT&& data, S&& source) :
      m_input(std::forward<U>(input)), m_data(std::move(data)), m_source(std::forward<S>(source)) {
    TRACE("inputs::Record::Record()");
  }

  Record(const Record&) = delete;
  Record& operator=(const Record&) = delete;
  Record(Record&&) noexcept = default;

  ~Record() { TRACE("inputs::Record::~Record()"); }

  UniqueMessagePtr exportMessage() {

    if constexpr (std::is_same_v<MessageT, RecordDataT>) {
      MessageT message(std::move(m_data));
      return std::make_unique<Message>(std::move(message));
    }
    else if constexpr (std::is_void_v<typename T::SourceT>) {
      MessageT message(m_input, std::move(m_data));
      return std::make_unique<Message>(std::move(message));
    } else {
      MessageT message(m_input, std::move(m_data), m_source.source);
      return std::make_unique<Message>(std::move(message));
    }
  }
};

template<class T>
using RecordQueue = MessageQueue<RecordPtr<T>>;

template<class T>
using RecordQueuePtr = std::shared_ptr<RecordQueue<T>>;
}  // namespace iqlogger::inputs
