//
// DummyMessage.h
// ~~~~~~~~~~~~~~~~
//
// Copyright (C) 2018 IQOption Software, Inc.
//
//

#pragma once

#define RAPIDJSON_HAS_STDSTRING 1

#include <sstream>
#include <string>

#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "formats/Message.h"
#include "formats/MessageInterface.h"

namespace iqlogger::formats::dummy {

class DummyMessage : public MessageInterface, public Message
{
public:
  using SourceT = void;

  template<typename T>
  explicit DummyMessage(std::string input, T&& data) : Message(std::move(input)), m_data(std::forward<T>(data)) {};

  DummyMessage(DummyMessage&&) noexcept = default;

  [[nodiscard]] std::string exportMessage() const override { return m_data; }

  [[nodiscard]] std::string exportMessage2Json() const override {
    rapidjson::StringBuffer s;
    rapidjson::Writer<rapidjson::StringBuffer> writer(s);

    writer.StartObject();
    writer.Key("data");
    writer.String(m_data);
    writer.Key("input");
    writer.String(getInput());
    writer.EndObject();
    return s.GetString();
  }

  ~DummyMessage() = default;

private:

  std::string m_data;

};
}  // namespace iqlogger::formats::dummy
