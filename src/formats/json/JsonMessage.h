//
// JsonMessage.h
// ~~~~~~~~~~~~~~~~
//
// Copyright (C) 2018 IQOption Software, Inc.
//
//

#pragma once

#include <nlohmann/json.hpp>
#include <sstream>

#include "JsonException.h"
#include "config/ConfigManager.h"
#include "formats/Message.h"
#include "formats/MessageInterface.h"

namespace iqlogger::formats::json {

class JsonMessage : public MessageInterface, public Message
{
  nlohmann::json m_json;
  endpoint_t m_endpoint;

public:
  using SourceT = endpoint_t;

  explicit JsonMessage(std::string input) : Message(std::move(input)) { TRACE("JsonMessage::JsonMessage()"); }

  template<class T, typename = std::enable_if_t<std::is_same<std::decay_t<T>, endpoint_t>::value>>
  explicit JsonMessage(std::string input, std::string&& message, T&& endpoint) :
      Message(std::move(input)), m_endpoint(std::forward<T>(endpoint)) {
    TRACE("JsonMessage::JsonMessage(std::string&& message, T&& endpoint)");

    try {
      m_json = nlohmann::json::parse(message);
    } catch (const nlohmann::json::parse_error& e) {
      std::ostringstream oss;
      oss << "Coudn't construct JSON message: " << e.what() << std::endl
          << "exception id: " << e.id << std::endl
          << "byte position of error: " << e.byte << std::endl;
      throw JsonParseError(oss.str());
    }

    TRACE("Construct JSON Message: " << message << " from " << m_endpoint);
  }

  explicit JsonMessage(std::string input, nlohmann::json&& json) : Message(std::move(input)), m_json(std::move(json)) {
    TRACE("JsonMessage::JsonMessage(nlohmann::json&&)");
  }

  JsonMessage(JsonMessage&&) noexcept = default;

  std::string exportMessage() const override { return m_json.dump(); }

  // @TODO
  std::string exportMessage2Json() const override { return m_json.dump(); }

  endpoint_t getSource() const { return m_endpoint; }

  nlohmann::json::const_iterator jbegin() const noexcept { return m_json.begin(); }

  nlohmann::json::iterator jbegin() noexcept { return m_json.begin(); }

  nlohmann::json::iterator jend() noexcept { return m_json.end(); }

  nlohmann::json::const_iterator jend() const noexcept { return m_json.end(); }
};
}  // namespace iqlogger::formats::json
