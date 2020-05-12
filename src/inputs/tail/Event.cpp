//
// Event.cpp
// ~~~~~~~~~~~~~~~~
//
// Copyright (C) 2018 IQOption Software, Inc.
//
//

#include "Event.h"

using namespace iqlogger;
using namespace iqlogger::inputs::tail;

Event::Event(fd_t wd, EventType event, std::string filename) :
m_watchDescriptor(wd), m_eventType(event), m_filename(std::move(filename)) {
  DEBUG("Event::Event(" << m_watchDescriptor << ", " << event_to_str(m_eventType).data() << ", " << m_filename
                        << ")");
}

std::ostream& operator<<(std::ostream& os, event_t event) {
  try {
    return os << Event::event_to_str(event).data();
  } catch (const std::out_of_range& ex) {
    ERROR("Exception: " << ex.what());
    return os;
  }
}