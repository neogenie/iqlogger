//
// TailMessage.h
// ~~~~~~~~~~~~~~~~
//
// Copyright (C) 2018 IQOption Software, Inc.
//
//

#pragma once

#define RAPIDJSON_HAS_STDSTRING 1

#include <sstream>

#include "Exception.h"
#include "TailSource.h"
#include "formats/Message.h"
#include "formats/MessageInterface.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

namespace iqlogger::formats::tail {

class TailMessage : public MessageInterface, public Message
{
public:
  using SourceT = TailSource;

  template<typename T, typename U>
  explicit TailMessage(std::string input, T&& data, U&& source) :
      Message(std::move(input)), m_data(std::forward<T>(data)), m_source(std::forward<U>(source)){};

  template<typename T, typename = std::enable_if_t<std::is_same<std::string, std::decay_t<T>>::value>>
  explicit TailMessage(std::string input, T&& json) : Message(std::move(input)) {
    try {
      nlohmann::json j = nlohmann::json::parse(std::forward<T>(json));

      if (j.at("data").is_object()) {
        m_data = j.at("data").dump();
      } else {
        m_data = j.at("data").get<std::string>();
      }

      auto filename = j.at("filename").get<std::string>();

      try {
        auto symlinkFilename = j.at("symlink_filename").get<std::string>();
        m_source = TailSource(std::move(filename), std::move(symlinkFilename));
      } catch (const std::exception& e) {
        m_source = TailSource(std::move(filename));
      }
    } catch (const std::exception& e) {
      std::ostringstream oss;
      oss << "Coudn't construct Tail message from " << json << ":" << e.what() << std::endl;
      throw Exception(oss.str());
    }
  }

  TailMessage(TailMessage&&) noexcept = default;

  [[nodiscard]] std::string exportMessage() const override { return m_data; }

  [[nodiscard]] std::string getFilename() const { return m_source.getFilename(); }
  //
  //  [[nodiscard]] std::string getSource() const { return m_filename; }

  [[nodiscard]] std::string exportMessage2Json() const override {
    rapidjson::StringBuffer s;
    rapidjson::Writer<rapidjson::StringBuffer> writer(s);

    writer.StartObject();
    writer.Key("data");
    writer.String(m_data);
    writer.Key("filename");
    writer.String(m_source.getFilename());

    if (const auto& symlinkFilename = m_source.getSymlinkFilename(); symlinkFilename) {
      writer.Key("symlink_filename");
      writer.String(symlinkFilename.value());
    }

    writer.Key("input");
    writer.String(getInput());
    writer.EndObject();
    return s.GetString();
  }

  ~TailMessage() = default;

private:
  std::string m_data;
  SourceT m_source;
};
}  // namespace iqlogger::formats::tail
