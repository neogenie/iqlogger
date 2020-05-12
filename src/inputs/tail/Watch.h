//
// Watch.h
// ~~~~~~~~~~~~~~~~
//
// Copyright (C) 2018 IQOption Software, Inc.
//
//

#pragma once

#include <regex>
#include <string>
#include <vector>

#include "Event.h"

namespace iqlogger::inputs::tail {

class InotifyServer;

class Watch
{
public:
  using WatchPath = std::pair<std::string, std::string>;

  explicit Watch(InotifyServer& inotifyServer, fd_t watchDescriptor, std::string directory, std::string pattern,
                 notifier_t notifier);

  [[nodiscard]] fd_t getWatchDescriptor() const { return m_watchDescriptor; }
  [[nodiscard]] std::string getWatchDirectory() const { return m_path.first; }
  [[nodiscard]] const std::string& getWatchPattern() const { return m_path.second; }
  [[nodiscard]] const WatchPath& getWatchPath() const { return m_path; }

  void notify(EventPtr eventPtr) const;

private:
  InotifyServer& m_inotifyServer;
  fd_t m_watchDescriptor;
  const WatchPath m_path;
  const std::regex m_regex;
  const notifier_t m_notifier;
};

using WatchPtr = std::shared_ptr<Watch>;

}  // namespace iqlogger::inputs::tail

std::ostream& operator<<(std::ostream& os, const iqlogger::inputs::tail::Watch&);
