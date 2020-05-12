//
// TailMonitor.h
// ~~~~~~~~~~~~~~~~
//
// Copyright (C) 2018 IQOption Software, Inc.
//
//

#pragma once

#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/regex.hpp>

#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <tbb/concurrent_hash_map.h>

#include "Exception.h"
#include "InotifyServer.h"
#include "MessageQueue.h"
#include "formats/tail/TailMessage.h"
#include "inputs/Record.h"

using namespace iqlogger::formats::tail;

namespace iqlogger::inputs::tail {

class TailMonitor
{
  using DelimiterRegex = std::optional<boost::regex>;

  class PointersTableInternalRecord
  {
    constexpr static size_t read_buffer_size = 4096 * 1024;  // 4Mb
    using read_buffer_t = std::array<char, read_buffer_size>;

    const std::string m_name;

    std::string m_fileName;

    boost::iostreams::file_descriptor_source m_fileDescriptor;
    boost::iostreams::stream<boost::iostreams::file_descriptor_source> m_fileStream;

    Position m_fileCurrentPosition;

    DelimiterRegex m_startMsgRegEx;
    std::string m_buffer;

    RecordQueuePtr<Tail> m_queuePtr;

    bool m_followOnly;
    bool m_saveState;

    read_buffer_t m_read_buffer;

    bool m_firstRead = false;

    std::optional<std::string> m_symlinkPath;

    void processBuffer(std::string_view buffer);
    void processMessage(std::string_view buffer);

    void savePosition() const;

  public:
    explicit PointersTableInternalRecord(std::string name, std::string file, DelimiterRegex startmsg_regex,
                                         RecordQueuePtr<Tail> queuePtr, bool followOnly, bool saveState);

    ~PointersTableInternalRecord();

    void process();
    void flush();

    std::string getCurrentFileName() const;

    const std::optional<std::string>& getSymlinkPath() const { return m_symlinkPath; }
    void rename(const std::string& filename);
  };

  using PointersTableInternalRecordPtr = std::shared_ptr<PointersTableInternalRecord>;
  using PointersTableInternal = tbb::concurrent_hash_map<std::string, PointersTableInternalRecordPtr>;
  using PointersTableInternalPtr = std::unique_ptr<PointersTableInternal>;

  using InotifyQueueT = MessageQueue<EventPtr>;
  using InotifyQueueTPtr = std::unique_ptr<InotifyQueueT>;

  const std::string m_name;

  PointersTableInternalPtr m_internalTable;
  DelimiterRegex m_startMsgRegEx;
  const RecordQueuePtr<Tail> m_queuePtr;

  InotifyQueueTPtr m_inotifyQueuePtr;

  bool m_followOnly;
  bool m_saveState;

  static std::array<EventPtr, max_queue_bulk_size>& getEventPtrQueueBuffer() {
    static thread_local std::array<EventPtr, max_queue_bulk_size> queue_buffer;
    return queue_buffer;
  }

  [[nodiscard]] PointersTableInternalRecordPtr createRecord(const std::string& filename) const;

  void modify(const std::string& filename);
  void create(const std::string& filename);
  void move(const std::string& filename);
  void remove(const std::string& filename);

public:
  TailMonitor(const std::string& name, RecordQueuePtr<Tail> queuePtr, DelimiterRegex&& startmsg_regex,
              const std::string& path, bool followOnly, bool saveState);

  void flush();
  size_t run();
};

using TailMonitorPtr = std::shared_ptr<TailMonitor>;
}  // namespace iqlogger::inputs::tail
