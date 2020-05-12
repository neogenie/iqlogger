//
// WatchDescriptorsMap.h
// ~~~~~~~~~~~~~~~~
//
// Copyright (C) 2018 IQOption Software, Inc.
//
//

#pragma once

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index_container.hpp>

#include <shared_mutex>

#include "Watch.h"

namespace iqlogger::inputs::tail {

class WatchDescriptorsMap
{
  struct DescriptorTag {};
  struct PathTag {};

  using internal_t = boost::multi_index_container<
      WatchPtr,
      boost::multi_index::indexed_by<
          boost::multi_index::hashed_unique<boost::multi_index::tag<DescriptorTag>,
                                            boost::multi_index::const_mem_fun<Watch, fd_t, &Watch::getWatchDescriptor>>,
          boost::multi_index::hashed_unique<
              boost::multi_index::tag<PathTag>,
              boost::multi_index::const_mem_fun<Watch, const Watch::WatchPath&, &Watch::getWatchPath>>>>;

public:
  WatchDescriptorsMap() = default;

  void addWatch(WatchPtr&& watchPtr);
  void removeWatch(WatchPtr&& watchPtr);

  WatchPtr findByWatchDescriptor(fd_t watchDescriptor) const;
  WatchPtr findByWatchPath(const std::string& directory, const std::string& pattern) const;

private:
  mutable std::shared_mutex m_mutex;
  internal_t m_map;
};

}  // namespace iqlogger::inputs::tail
