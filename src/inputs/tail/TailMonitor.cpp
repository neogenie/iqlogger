//
// TailMonitor.cpp
// ~~~~~~~~~~~~~~~~
//
// Copyright (C) 2018 IQOption Software, Inc.
//
//

#include "TailMonitor.h"

#include "SavePositionServer.h"
#include "core/Log.h"

using namespace iqlogger;
using namespace iqlogger::inputs::tail;

TailMonitor::TailMonitor(const std::string& name, RecordQueuePtr<Tail> queuePtr, DelimiterRegex&& startmsg_regex,
                         const std::string& path, bool followOnly, bool saveState) :
    m_name(name),
    m_internalTable(std::make_unique<PointersTableInternal>()),
    m_startMsgRegEx(std::move(startmsg_regex)),
    m_queuePtr(queuePtr),
    m_inotifyQueuePtr{std::make_unique<InotifyQueueT>("inputs." + m_name + ".monitor.event_queue")},
    m_followOnly(followOnly),
    m_saveState(saveState) {
  TRACE("TailMonitor::TailMonitor()");

  auto notifier = [this](EventPtr eventPtr) {
    DEBUG("Notifier Event: " << *eventPtr);
    m_inotifyQueuePtr->enqueue(std::move(eventPtr));
  };

  INFO("Add Watch to " << path);
  InotifyServer::getInstance()->addWatch(path, notifier);
}

void TailMonitor::modify(TailSource source) {
  DEBUG("TailMonitor::modify(" << source << ")");

  const auto& filename = source.getFilename();

  PointersTableInternal::accessor accessor;
  if (m_internalTable->insert(accessor, filename)) {
    accessor->second = createRecord(std::move(source));
  }

  accessor->second->process();
}

void TailMonitor::create(TailSource source) {
  DEBUG("TailMonitor::create(" << source << ")");
  modify(std::move(source));
}

void TailMonitor::move(TailSource source) {
  DEBUG("TailMonitor::move(" << source << ")");

  const auto& filename = source.getFilename();

  {
    PointersTableInternal::const_accessor accessor;
    if (m_internalTable->find(accessor, filename)) {
      auto newFilename = accessor->second->getCurrentFileName();

      DEBUG("New filename of " << filename << ": " << newFilename);

      if (newFilename != filename) {
        // @TODO Нужно удалять запись из таблицы, если имя файла более не отслеживается
        TRACE("@TODO");

        accessor->second->rename(newFilename);

        PointersTableInternal::accessor insert_accessor;
        if (m_internalTable->insert(insert_accessor, newFilename)) {
          insert_accessor->second = accessor->second;
        }

        m_internalTable->erase(accessor);
      }
    } else {
      WARNING("Internal Pointer of moved entry not found!");
      return;
    }
  }
}

void TailMonitor::remove(TailSource source) {
  DEBUG("TailMonitor::remove(" << source << ")");

  const auto& filename = source.getFilename();

  std::optional<std::string> symlinkPath;

  {
    PointersTableInternal::accessor accessor;
    if (m_internalTable->find(accessor, filename)) {
      symlinkPath = accessor->second->getSymlinkPath();
      m_internalTable->erase(accessor);
      SavePositionServer::getInstance()->erasePosition(filename);
    } else {
      WARNING("Internal Pointer of deleted entry not found!");
    }
  }

  if (symlinkPath) {
    DEBUG("Removed Pointer is symlink to: " << symlinkPath.value());
    InotifyServer::getInstance()->removeWatch(symlinkPath.value());
  }
}

size_t TailMonitor::run() {
  auto& queue_buffer = getEventPtrQueueBuffer();
  auto count = m_inotifyQueuePtr->try_dequeue_bulk(queue_buffer.begin(), max_queue_bulk_size);  // @TODO

  if (count) {
    TRACE("Fetch Event Queue: " << count);

    // @FIXME Optimization (Можно перекомпоновать события и убрать дубликаты)
    // auto last = std::unique(queue_buffer.begin(), queue_buffer.end());

    std::size_t i;

    for (i = 0; i < count; ++i) {
      const auto& eventPtr = queue_buffer[i];

      DEBUG("Fetch Event: " << *eventPtr);

      auto type = eventPtr->m_eventType;
      auto source = eventPtr->m_source;

      switch (type) {
      case Event::EventType::_INIT:
      case Event::EventType::CREATE:
        create(source);
        break;

      case Event::EventType::MODIFY:
        modify(source);
        break;

      case Event::EventType::MOVE:
        move(source);
        break;

      case Event::EventType::DELETE:
        remove(source);
        break;

      default:

        WARNING("Unknown event type: " << type);
      }
    }
  }

  return count;
}

void TailMonitor::flush() {
  //  DEBUG("TailMonitor::flush()");

  for (auto it = m_internalTable->begin(); it != m_internalTable->end(); ++it) {
    PointersTableInternal::const_accessor accessor;

    if (m_internalTable->find(accessor, it->first)) {
      accessor->second->flush();
    }
  }
}

TailMonitor::PointersTableInternalRecord::PointersTableInternalRecord(std::string name, TailSource source,
                                                                      DelimiterRegex startmsg_regex,
                                                                      RecordQueuePtr<Tail> queuePtr, bool followOnly,
                                                                      bool saveState) :
    m_name(std::move(name)),
    m_source(std::move(source)),
    m_fileDescriptor(boost::iostreams::file_descriptor_source(m_source.getFilename())),
    m_fileStream(m_fileDescriptor),
    m_fileCurrentPosition(0),
    m_startMsgRegEx(std::move(startmsg_regex)),
    m_queuePtr(std::move(queuePtr)),
    m_followOnly(followOnly),
    m_saveState(saveState) {
  DEBUG("PointersTableInternalRecord(`" << m_name << "`, `" << m_source << "`) FD: " << m_fileDescriptor.handle()
                                        << "");

  const auto& filename = m_source.getFilename();

  try {
    if (std::filesystem::is_symlink(filename)) {
      DEBUG("File name: `" << filename << "` is symlink...");
      m_symlinkPath = std::filesystem::read_symlink(filename).string();
    }
  } catch (const std::filesystem::filesystem_error& e) {
    ERROR("Error read from FS: " << e.what());
  }

  if (m_saveState) {
    auto savedPosition = SavePositionServer::getInstance()->getSavedPosition(filename);
    DEBUG("New PointersTableInternalRecord " << filename << " (Saved State Position: " << savedPosition << ")");
    m_fileCurrentPosition = savedPosition;
  }
}

TailMonitor::PointersTableInternalRecordPtr TailMonitor::createRecord(TailSource source) const {
  return std::make_unique<PointersTableInternalRecord>(m_name, std::move(source), m_startMsgRegEx, m_queuePtr,
                                                       m_followOnly, m_saveState);
}

void TailMonitor::PointersTableInternalRecord::process() {
  DEBUG("TailMonitor::PointersTableInternalRecord::process(" << m_fileDescriptor.handle() << ")");

  const auto& filename = m_source.getFilename();

  if (m_symlinkPath) {
    DEBUG("PointersTableInternalRecord " << filename << " is symlink... Ignore.");
    return;
  }

  if (!m_fileStream.is_open()) {
    WARNING("FD not opened... " << filename);
    return;
  }

  m_fileStream.seekg(0, std::ios::end);
  Position filesize = m_fileStream.tellg();

  if (filesize == std::ios::pos_type(-1)) {
    WARNING("Detected truncation of file " << filename << " while read...");
    m_fileCurrentPosition = 0;
    m_fileStream.clear();
    return;
  } else if (filesize < m_fileCurrentPosition) {
    m_fileCurrentPosition = 0;
  }

  size_t added = filesize - m_fileCurrentPosition;

  TRACE("Detected add " << added << " bytes to file " << filename);

  if (added > 0 && (!m_firstRead && m_followOnly)) {
    DEBUG("Initial file " << filename << " size detected: " << filesize << ". Follow only mode: skip!");
    m_fileCurrentPosition = filesize;
    m_firstRead = true;
    return;
  } else if (added > 0) {
    m_fileStream.seekg(-added, std::ios::cur);

    size_t need_read, total_read = 0, actual_read;

    while (total_read < added) {
      need_read = std::min(read_buffer_size, (added - total_read));
      TRACE("Detected need read " << need_read << " bytes");
      m_fileStream.read(&m_read_buffer[0], need_read);
      actual_read = m_fileStream.gcount();
      TRACE("Actual read " << actual_read << " bytes");

      if (!actual_read)
        break;

      processBuffer(std::string_view(m_read_buffer.cbegin(), actual_read));
      total_read += actual_read;
      m_fileCurrentPosition += actual_read;
    }

    TRACE("Current position: " << m_fileCurrentPosition);
  } else {
    TRACE("Nothing added...");
  }

  savePosition();
}

void TailMonitor::PointersTableInternalRecord::flush() {
  // TRACE("TailMonitor::PointersTableInternalRecord::flush()");
  if (!m_buffer.empty()) {
    TRACE("Flush buffer after timeout");
    processMessage(m_buffer);
    m_buffer.clear();

    savePosition();
  }
}

void TailMonitor::PointersTableInternalRecord::processBuffer(std::string_view buffer) {
  TRACE("TailMonitor::PointersTableInternalRecord::processBuffer()");
  TRACE("Buffer: " << buffer);

  if (m_startMsgRegEx) {
    TRACE("Regex delimiter");

    // @TODO Пока нет перегрузок для использования boost::sregex_iterator и std::string_view::iterator
    boost::cregex_iterator it(buffer.cbegin(), buffer.cend(), m_startMsgRegEx.value());
    boost::cregex_iterator prev, end;

    auto c = std::distance(it, end);
    if (c > 0) {
      TRACE("Found " << c << " splitters. Current buffer: " << m_buffer.size());

      prev = it;

      if (!m_buffer.empty()) {
        if (it->position() == 0) {
          processMessage(m_buffer);
        } else {
          processMessage(m_buffer + std::string(buffer.cbegin(), buffer.cbegin() + it->position()));
        }

        m_buffer.clear();
        ++it;
      } else {
        if (it->position()) {
          processMessage(buffer.substr(0, it->position()));
        }

        ++it;
      }

      for (; it != end; prev = it, ++it) {
        processMessage(buffer.substr(prev->position(), it->position() - prev->position()));
      }

      if (prev != end)
        m_buffer = std::string(buffer.cbegin() + prev->position(), buffer.cend());
    } else {
      TRACE("No delimiter in block... Buffering...");
      m_buffer += buffer;
    }
  } else {
    size_t first = 0;

    while (first < buffer.size()) {
      const auto second = buffer.find_first_of('\n', first);

      if (first != second) {
        processMessage(buffer.substr(first, second - first));
      }

      if (second == std::string_view::npos)
        break;

      first = second + 1;
    }
  }
}

void TailMonitor::PointersTableInternalRecord::processMessage(std::string_view buffer) {
  TRACE("Got tail message: " << buffer);

  try {
    if (!m_queuePtr->enqueue(std::make_unique<Record<Tail>>(m_name, std::string(buffer), m_source))) {
      ERROR("Journal Input queue is full... Dropping...");
    }
  } catch (const Exception& e) {
    WARNING("Tail buffer decode error: " << e.what());
  }
}

std::string TailMonitor::PointersTableInternalRecord::getCurrentFileName() const {
  std::string proc = "/proc/self/fd/" + std::to_string(m_fileDescriptor.handle());
  DEBUG("Check: " << proc);
  auto path = std::filesystem::read_symlink(proc);

  if (!path.empty()) {
    DEBUG("Filename of: " << m_fileDescriptor.handle() << " is: " << path);
    return path.string();
  }

  return m_source.getFilename();
}

void TailMonitor::PointersTableInternalRecord::rename(const std::string& filename) {
  m_source.setFilename(filename);
}

TailMonitor::PointersTableInternalRecord::~PointersTableInternalRecord() {
  DEBUG("TailMonitor::PointersTableInternalRecord::~PointersTableInternalRecord()");
}

void TailMonitor::PointersTableInternalRecord::savePosition() const {
  if (m_saveState) {
    SavePositionServer::getInstance()->savePosition(m_source.getFilename(), m_fileCurrentPosition - m_buffer.size());
  }
}
