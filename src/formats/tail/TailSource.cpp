//
// TailSource..cpp
// ~~~~~~~~~~~~~~~~
//
// Copyright (C) 2018 IQOption Software, Inc.
//
//

#include "TailSource.h"

#include "core/Log.h"

using namespace iqlogger;
using namespace iqlogger::formats::tail;

std::ostream& operator<<(std::ostream& os, const iqlogger::formats::tail::TailSource& tailSource) {
  os << "[FILE: " << tailSource.getFilename();
  if (const auto& symlinkFilename = tailSource.getSymlinkFilename(); symlinkFilename) {
    os << " SYMLINK: " << symlinkFilename.value();
  }
  os << " ]";
  return os;
}