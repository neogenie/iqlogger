//
// WatchDescriptorsMap.cpp
// ~~~~~~~~~~~~~~~~
//
// Copyright (C) 2018 IQOption Software, Inc.
//
//

#include "WatchDescriptorsMap.h"

#include "InotifyServer.h"
#include "core/Log.h"

using namespace iqlogger;
using namespace iqlogger::inputs::tail;

void WatchDescriptorsMap::addWatch(WatchPtr&& watchPtr) {
  DEBUG("Add Watch WD: " << *watchPtr);
  {
    std::unique_lock lock(m_mutex);
    m_map.insert(std::move(watchPtr));
  }
}

void WatchDescriptorsMap::removeWatch(WatchPtr&& watchPtr) {
  DEBUG("Remove Watch WD: " << *watchPtr);
  {
    std::unique_lock lock(m_mutex);
    if (auto it = m_map.get<DescriptorTag>().find(watchPtr->getWatchDescriptor()); it != m_map.end()) {
      m_map.erase(it);
    } else {
      WARNING("Removing Watch WD: " << watchPtr->getWatchDescriptor() << " not found!");
    }
  }
}

WatchPtr WatchDescriptorsMap::findByWatchDescriptor(fd_t watchDescriptor) const {
  DEBUG("Find By Watch Descriptor: " << watchDescriptor);
  {
    std::shared_lock lock(m_mutex);
    if (auto it = m_map.get<DescriptorTag>().find(watchDescriptor); it != m_map.end()) {
      return *it;
    }
  }
  return {};
}

WatchPtr WatchDescriptorsMap::findByWatchPath(const std::string& directory, const std::string& pattern) const {
  DEBUG("Find By Watch Directory: " << directory << " and Pattern: " << pattern);
  {
    std::shared_lock lock(m_mutex);
    if (auto it = m_map.get<PathTag>().find(std::make_pair(directory, pattern)); it != m_map.get<PathTag>().end()) {
      return *it;
    }
  }
  return {};
}
