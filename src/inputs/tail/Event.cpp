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

Event::Event(fd_t wd, EventType event, TailSource source) :
    m_watchDescriptor(wd), m_eventType(event), m_source(std::move(source)) {
  DEBUG("Event::Event(" << *this << ")");
}

std::ostream& operator<<(std::ostream& os, Event::EventType eventType) {
  static constexpr std::pair<Event::EventType, frozen::string> s_event_to_name_map[]{
      {Event::EventType::UNKNOWN, "UNDEFINED"}, {Event::EventType::_INIT, "_INIT"},
      {Event::EventType::CREATE, "CREATE"},     {Event::EventType::MODIFY, "MODIFY"},
      {Event::EventType::MOVE, "MOVE"},         {Event::EventType::DELETE, "DELETE"},
  };

  static constexpr auto event_to_str_map = frozen::make_unordered_map(s_event_to_name_map);

  try {
    os << event_to_str_map.at(eventType).data();
  } catch (const std::out_of_range& ex) {
    ERROR("Exception: " << ex.what());
  }
  return os;
}

std::ostream& operator<<(std::ostream& os, const Event& event) {
  os << "[WD: " << event.m_watchDescriptor << ", TYPE: " << event.m_eventType << ", SOURCE: " << event.m_source << "]";
  return os;
}