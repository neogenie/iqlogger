//
// Message.h
// ~~~~~~~~~~~~~~~~
//
// Copyright (C) 2018 IQOption Software, Inc.
//
//

#pragma once

#include <string>

namespace iqlogger::formats {

class Message
{
public:
  explicit Message(std::string input) : m_input(std::move(input)) {}

  [[nodiscard]] std::string getInput() const { return m_input; }

private:
  std::string m_input;
};
}  // namespace iqlogger::formats
