//
// TailSource.h
// ~~~~~~~~~~~~~~~~
//
// Copyright (C) 2018 IQOption Software, Inc.
//
//

#pragma once

#include <optional>
#include <string>

namespace iqlogger::formats::tail {

class TailSource
{
public:
  explicit TailSource(std::string filename) : m_filename(std::move(filename)) {}
  explicit TailSource(std::string filename, std::string symlinkFilename) :
      m_filename(std::move(filename)), m_symlinkFilename(std::move(symlinkFilename)) {}

  const std::string& getFilename() const { return m_filename; }
  const std::optional<std::string>& getSymlinkFilename() const { return m_symlinkFilename; }

  void setFilename(std::string filename) { m_filename = std::move(filename); }

private:
  std::string m_filename;
  std::optional<std::string> m_symlinkFilename;
};
}  // namespace iqlogger::formats::tail

std::ostream& operator<<(std::ostream& os, const iqlogger::formats::tail::TailSource&);
