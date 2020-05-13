//
// Event.h
// ~~~~~~~~~~~~~~~~
//
// Copyright (C) 2018 IQOption Software, Inc.
//
//

#pragma once

#include <memory>

#include "config/Config.h"
#include "core/Log.h"
#include "formats/tail/TailSource.h"

namespace iqlogger::inputs::tail {

using fd_t = int;
using TailSource = formats::tail::TailSource;

struct Event {
  enum class EventType
  {
    UNKNOWN,
    _INIT,
    CREATE,
    MODIFY,
    MOVE,
    DELETE
  };

  fd_t m_watchDescriptor;
  EventType m_eventType;
  TailSource m_source;

  explicit Event(fd_t wd, EventType event, TailSource source);

  ~Event() { DEBUG("Event::~Event()"); }
};

using EventPtr = std::unique_ptr<Event>;

using notifier_t = std::function<void(EventPtr)>;

}  // namespace iqlogger::inputs::tail

std::ostream& operator<<(std::ostream& os, iqlogger::inputs::tail::Event::EventType eventType);
std::ostream& operator<<(std::ostream& os, const iqlogger::inputs::tail::Event& event);
