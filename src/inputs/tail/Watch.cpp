//
// Watch..cpp
// ~~~~~~~~~~~~~~~~
//
// Copyright (C) 2018 IQOption Software, Inc.
//
//

#include "Watch.h"

#include "InotifyServer.h"
#include "core/Log.h"

using namespace iqlogger;
using namespace iqlogger::inputs::tail;

Watch::Watch(InotifyServer& inotifyServer, fd_t watchDescriptor, std::string directory, std::string pattern,
             notifier_t notifier) :
    m_inotifyServer(inotifyServer),
    m_watchDescriptor(watchDescriptor),
    m_path(std::make_pair(std::move(directory), std::move(pattern))),
    m_regex(getWatchPattern()),
    m_notifier(std::move(notifier)) {
  DEBUG("InotifyServer::Watch::Watch(" << *this << ")");
}

void Watch::notify(EventPtr eventPtr) const {
  DEBUG("Notify Event: " << *eventPtr);

  auto filename = getWatchDirectory() + '/' + eventPtr->m_source.getFilename();

  if (std::regex_match(filename, m_regex)) {
    DEBUG("Event match: " << filename);

    if (std::filesystem::is_directory(filename)) {
      DEBUG("Path: `" << filename << "` is directory. Recursive add watch to directory...");
      m_inotifyServer.addWatch(filename, m_notifier);
    } else if (std::filesystem::is_symlink(filename)) {
      auto linked = std::filesystem::read_symlink(filename);
      DEBUG("Path: `" << filename << "` is symlink. Recursive add watch to source `" << linked.string() << "`...");
      auto notifier = [notifier = m_notifier, filename](EventPtr eventPtr) {
        DEBUG("Notifier Event Changer: " << *eventPtr);
        notifier(std::make_unique<Event>(eventPtr->m_watchDescriptor, eventPtr->m_eventType,
                                         TailSource(eventPtr->m_source.getFilename(), filename)));
      };
      m_inotifyServer.addWatch(linked.string(), notifier);
      m_notifier(std::make_unique<Event>(eventPtr->m_watchDescriptor, eventPtr->m_eventType, TailSource(filename)));
    } else {
      DEBUG("Path: `" << filename << "` is file. Notify...");
      m_notifier(std::make_unique<Event>(eventPtr->m_watchDescriptor, eventPtr->m_eventType, TailSource(filename)));
    }
  } else {
    DEBUG("Event not match: " << filename);
  }
}

std::ostream& operator<<(std::ostream& os, const iqlogger::inputs::tail::Watch& watch) {
  os << "[WD: " << watch.getWatchDescriptor() << ", DIRECTORY: " << watch.getWatchDirectory()
     << ", PATTERN: " << watch.getWatchPattern() << "]";
  return os;
}