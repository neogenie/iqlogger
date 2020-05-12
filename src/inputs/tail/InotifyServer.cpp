//
// InotifyServer.cpp
// ~~~~~~~~~~~~~~~~
//
// Copyright (C) 2018 IQOption Software, Inc.
//
//

#include "InotifyServer.h"

#include "core/Log.h"

using namespace iqlogger;
using namespace iqlogger::inputs::tail;

InotifyServer::InotifyServer() :
    m_inotify_fd(inotify_init1(IN_NONBLOCK)), m_stream(m_io_service, m_inotify_fd), m_strand(m_io_service) {
  DEBUG("Create Inotify Descriptor: " << m_inotify_fd);

  if (m_inotify_fd == -1) {
    boost::system::system_error e(boost::system::error_code(errno, boost::asio::error::get_system_category()),
                                  "Init inotify failed");
    boost::throw_exception(e);
  }

  begin_read();
}

void InotifyServer::begin_read() {
  TRACE("InotifyServer::begin_read()");
  auto handler = [this](const boost::system::error_code& ec, std::size_t bytes_transferred) {
    end_read(ec, bytes_transferred);
  };

  m_stream.async_read_some(boost::asio::buffer(m_read_buffer), m_strand.wrap(std::move(handler)));
}

void InotifyServer::end_read(const boost::system::error_code& ec, std::size_t bytes_transferred) {
  DEBUG("InotifyServer::end_read()");

  if (!ec) {
    m_pending_read_buffer += std::string(m_read_buffer.data(), bytes_transferred);
    while (m_pending_read_buffer.size() >= sizeof(inotify_event)) {
      const auto* iev = reinterpret_cast<const inotify_event*>(m_pending_read_buffer.data());

      DEBUG("Inotify event WD: " << iev->wd << " Name: " << iev->name);

      switch (iev->mask) {
      case IN_ACCESS:
        DEBUG("Inotify IN_ACCESS: " << iev->wd << " Name: " << iev->name << " Mask: " << std::hex << iev->mask);
        break;

      case IN_OPEN:
        DEBUG("Inotify IN_OPEN: " << iev->wd << " Name: " << iev->name << " Mask: " << std::hex << iev->mask);
        break;

      case IN_CLOSE_NOWRITE:
        DEBUG("Inotify IN_CLOSE_NOWRITE: " << iev->wd << " Name: " << iev->name << " Mask: " << std::hex << iev->mask);
        break;

      case IN_CLOSE_WRITE:
        DEBUG("Inotify IN_CLOSE_WRITE: " << iev->wd << " Name: " << iev->name << " Mask: " << std::hex << iev->mask);
        // notify(iev->wd, std::make_unique<Event>(iev->wd, event_t::MODIFY, iev->name));
        break;

      case IN_CREATE:
        DEBUG("Inotify CREATE: " << iev->wd << " Name: " << iev->name << " Mask: " << std::hex << iev->mask);
        notify(iev->wd, std::make_unique<Event>(iev->wd, event_t::CREATE, iev->name));
        break;

      case IN_ATTRIB:
        DEBUG("Inotify IN_ATTRIB: " << iev->wd << " Name: " << iev->name << " Mask: " << std::hex << iev->mask);
        break;

      case IN_DELETE:
        DEBUG("Inotify DELETE: " << iev->wd << " Name: " << iev->name << " Mask: " << std::hex << iev->mask);
        notify(iev->wd, std::make_unique<Event>(iev->wd, event_t::DELETE, iev->name));
        break;

      case IN_MOVED_FROM:
        DEBUG("Inotify MOVED_FROM: " << iev->wd << " Name: " << iev->name << " Mask: " << std::hex << iev->mask);
        notify(iev->wd, std::make_unique<Event>(iev->wd, event_t::MOVE, iev->name));
        break;

      case IN_MOVED_TO:
        DEBUG("Inotify MOVED_TO: " << iev->wd << " Name: " << iev->name << " Mask: " << std::hex << iev->mask);
        notify(iev->wd, std::make_unique<Event>(iev->wd, event_t::CREATE, iev->name));
        break;

      case IN_CREATE | IN_ISDIR:
        DEBUG("Inotify IN_CREATE | IN_ISDIR: " << iev->wd << " Name: " << iev->name << " Mask: " << std::hex
                                               << iev->mask);
        notify(iev->wd, std::make_unique<Event>(iev->wd, event_t::CREATE, iev->name));
        break;

      case IN_MOVE_SELF:
        DEBUG("Inotify IN_MOVE_SELF: " << iev->wd << " Name: " << iev->name << " Mask: " << std::hex << iev->mask);
        break;

      case IN_MODIFY:
        DEBUG("Inotify MODIFY: " << iev->wd << " Name: " << iev->name << " Mask: " << std::hex << iev->mask);
        notify(iev->wd, std::make_unique<Event>(iev->wd, event_t::MODIFY, iev->name));
        break;

      default:
        DEBUG("Not expected event: " << iev->wd << " Name: " << iev->name << " Mask: " << std::hex << iev->mask);
      }

      m_pending_read_buffer.erase(0, sizeof(inotify_event) + iev->len);
    }

    begin_read();
  } else if (ec != boost::asio::error::operation_aborted) {
    WARNING("Inotify EC: " << ec.value() << " (" << ec.message() << ")");
  }
}

void InotifyServer::notify(fd_t watchDescriptor, EventPtr eventPtr) {
  DEBUG("InotifyServer::notify(): " << watchDescriptor << " Name: " << eventPtr->m_filename
                                    << " Type: " << eventPtr->m_eventType);

  if (auto watchPtr = m_watchDescriptorsMap.findByWatchDescriptor(watchDescriptor); watchPtr) {
    DEBUG("Found path notify WD: " << *watchPtr);
    watchPtr->notify(std::move(eventPtr));
  }
}

std::pair<std::string, std::string> InotifyServer::makeWatchPath(const std::string& path) {
  std::pair<std::string, std::string> result;

  auto& [directory, pattern] = result;

  if (std::filesystem::is_directory(path)) {
    directory = path;
    pattern = path + "/(.*)";
  } else {
    directory = std::filesystem::path(path).parent_path();
    pattern = path;
  }

  return result;
}

void InotifyServer::addWatch(const std::string& path, const notifier_t& notifier) {
  DEBUG("Add Watch: path: `" << path << "`");

  auto [directory, pattern] = makeWatchPath(path);

  DEBUG("Add Watch to directory: `" << directory << "` Pattern:: `" << pattern << "`");

  auto watchDescriptor = inotify_add_watch(m_inotify_fd, directory.c_str(), inotify_events);
  INFO("New listener to `" << directory << "` I_FD: " << m_inotify_fd << " W_FD: " << watchDescriptor);

  if (watchDescriptor == -1) {
    CRITICAL("Inotify watch failed to " << directory);
    return;
  }

  auto watchPtr = std::make_shared<Watch>(*this, watchDescriptor, directory, pattern, notifier);
  m_watchDescriptorsMap.addWatch(std::move(watchPtr));

  try {
    INFO("Find all files & directories in `" << directory << "` with pattern `" << pattern << "`");
    for (const auto& p : std::filesystem::directory_iterator(directory)) {
      auto filename = p.path().string();
      if (std::regex_match(filename, std::regex(pattern))) {
        if (std::filesystem::is_directory(filename)) {
          DEBUG("Path: `" << filename << "` is directory. Recursive add watch to directory...");
          addWatch(filename, notifier);
        } else if (std::filesystem::is_symlink(filename)) {
          auto linked = std::filesystem::read_symlink(filename);
          DEBUG("Path: `" << filename << "` is symlink. Recursive add watch to source `" << linked.string() << "`...");
          addWatch(linked.string(), notifier);
          notify(watchDescriptor, std::make_unique<Event>(watchDescriptor, event_t::_INIT, p.path().filename()));
        } else {
          DEBUG("Path: `" << filename << "` is file. Initial read from matched file...");
          notify(watchDescriptor, std::make_unique<Event>(watchDescriptor, event_t::_INIT, p.path().filename()));
        }
      } else {
        DEBUG("NOT Initial read from not matched file: " << filename);
      }
    }
  } catch (const std::filesystem::filesystem_error& e) {
    ERROR("Error read from FS: " << e.what() << " during add watch to " << directory);
  }

  TRACE("InotifyServer::addWatch() <-");
}

void InotifyServer::removeWatch(const std::string& path) {
  DEBUG("Remove Watch to path: `" << path << "`");

  auto [directory, pattern] = makeWatchPath(path);

  if (auto watchPtr = m_watchDescriptorsMap.findByWatchPath(directory, pattern); watchPtr) {
    DEBUG("Found path for remove WD: " << *watchPtr);
    auto watchDescriptor = watchPtr->getWatchDescriptor();
    m_watchDescriptorsMap.removeWatch(std::move(watchPtr));
    if (auto result = inotify_rm_watch(m_inotify_fd, watchDescriptor); result) {
      ERROR("Inotify remove watch failed WD: " << watchDescriptor << " Error code: " << strerror(errno));
    }
  }
}

void InotifyServer::initImpl(std::any) {
  // @TODO
}

void InotifyServer::startImpl() {
  TRACE("InotifyServer::start()");

  INFO("Start Inotify Server");

  INFO("Start InotifyServer threads...");
  for (size_t i = 0; i < m_thread_num; ++i) {
    m_threads.emplace_back(std::thread([this]() {
      try {
        m_io_service.run();
      } catch (const std::exception& e) {
        CRITICAL("Inotify thread running exited with " << e.what());
      }
    }));
  }
}

void InotifyServer::stopImpl() {
  INFO("Stop Inotify Server");

  m_io_service.stop();

  DEBUG("Join Inotify Server threads... ");
  for (auto& t : m_threads) {
    if (t.joinable()) {
      t.join();
    }
  }
}

InotifyServer::~InotifyServer() {
  DEBUG("InotifyServer::~InotifyServer()");
}
