//
// Output.cpp
// ~~~~~~~~~~~~~~~~
//
// Copyright (C) 2018 IQOption Software, Inc.
//
//

#include "Log.h"
#include "LogOutput.h"
#include "Message.h"
#include "outputs/OutputInterface.h"
#include "outputs/Record.h"

#ifdef IQLOGGER_INPUT_MODULE_JOURNAL
#include "inputs/journal/Journal.h"
using namespace iqlogger::formats::journal;
#endif

#ifdef IQLOGGER_INPUT_MODULE_TAIL
#include "inputs/tail/Tail.h"
using namespace iqlogger::formats::tail;
#endif

#ifdef IQLOGGER_INPUT_MODULE_GELF
#include "inputs/gelf/Gelf.h"
using namespace iqlogger::formats::gelf;
#endif

#ifdef IQLOGGER_INPUT_MODULE_JSON
#include "inputs/json/Json.h"
using namespace iqlogger::formats::json;
#endif

#ifdef IQLOGGER_INPUT_MODULE_DUMMY
#include "inputs/dummy/Dummy.h"
#endif

#ifdef IQLOGGER_WITH_PROCESSOR
#include "processor/ProcessorMessage.h"
using namespace iqlogger::processor;
#endif

using namespace iqlogger;
using namespace iqlogger::outputs;
using namespace iqlogger::outputs::log;

template<>
OutputPtr OutputInterface::instantiateOutput<config::OutputType::LOG>(
    const config::DestinationConfig& destinationConfig,
    iqlogger::metrics::atomic_metric_t& total_outputs_send_counter) {
  return std::make_unique<iqlogger::outputs::log::LogOutput>(destinationConfig, total_outputs_send_counter);
}

#ifdef IQLOGGER_INPUT_MODULE_JOURNAL
// Journal
template<>
template<>
Record<Log>::Record(const iqlogger::inputs::journal::Journal::MessageT& message) {
  TRACE("Transform Journal -> Log: ");

  std::ostringstream oss;

  const auto& data = message.getData();

  for (const auto& [key, value] : data) {
    std::visit([&oss, &key = std::as_const(key)](const auto& value) { oss << key << " : " << value << std::endl; },
               value);
  }

  m_data = oss.str();
}
#endif

#ifdef IQLOGGER_INPUT_MODULE_TAIL
// Tail
template<>
template<>
Record<Log>::Record(const iqlogger::inputs::tail::Tail::MessageT& message) {
  TRACE("Transform Tail -> Log: ");

  std::ostringstream oss;
  oss << message.getFilename() << " : " << message.exportMessage() << std::endl;
  m_data = oss.str();
}
#endif

#ifdef IQLOGGER_INPUT_MODULE_GELF
// Gelf
template<>
template<>
Record<Log>::Record(const iqlogger::inputs::gelf::Gelf::MessageT& message) : m_data(message.exportMessage()) {
  TRACE("Transform Gelf -> Log: ");
}
#endif

#ifdef IQLOGGER_INPUT_MODULE_JSON
// Json
template<>
template<>
Record<Log>::Record(const iqlogger::inputs::json::Json::MessageT& message) : m_data(message.exportMessage()) {
  TRACE("Transform Json -> Log: ");
}
#endif

#ifdef IQLOGGER_INPUT_MODULE_DUMMY
// Dummy
template<>
template<>
Record<Log>::Record(const iqlogger::inputs::dummy::Dummy::MessageT& message) : m_data(message.exportMessage()) {
  TRACE("Transform Dummy -> Log: ");
}
#endif

#ifdef IQLOGGER_WITH_PROCESSOR
// Processor
template<>
template<>
Record<Log>::Record(const ProcessorMessage& message) : m_data(message.exportMessage()) {
  TRACE("Transform Processor -> Log: ");
}
#endif