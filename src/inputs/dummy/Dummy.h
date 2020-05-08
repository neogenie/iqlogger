//
// Dummy.h
// ~~~~~~~~~~~~~~~~
//
// Copyright (C) 2018 IQOption Software, Inc.
//
//

#pragma once

#include <string>

#include "formats/dummy/DummyMessage.h"

namespace iqlogger::inputs::dummy {

class Dummy
{
public:
  using RecordDataT = std::string;
  using MessageT = formats::dummy::DummyMessage;
  using SourceT = void;
};
}  // namespace iqlogger::inputs::dummy
