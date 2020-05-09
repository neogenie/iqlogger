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
  TRACE("InotifyServer::InotifyServer()");
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

  WatchDescriptorsMap::const_accessor wd_accessor;

  if (m_watchDescriptorsMap.find(wd_accessor, watchDescriptor)) {
    wd_accessor->second->notify(std::move(eventPtr));
  } else {
    ERROR("Error find WATCH WD: " << watchDescriptor);
  }
}

void InotifyServer::addWatch(const std::string& path, const notifier_t& notifier, bool fileOnlyMode) {
  DEBUG("Add Watch: path: `" << path << "`. File Only Mode: " << std::boolalpha << fileOnlyMode);

  std::string directory;
  std::string pattern;

  if (fileOnlyMode) {
    directory = pattern = path;
  } else {
    if (std::filesystem::is_directory(path)) {
      directory = path;
      pattern = path + "/(.*)";
    } else {
      directory = std::filesystem::path(path).parent_path();
      pattern = path;
    }
  }

  DEBUG("Add WATCH: DIR: `" << directory << "` PATTERN: `" << pattern << "`");

  auto watchDescriptor = inotify_add_watch(m_inotify_fd, directory.c_str(), inotify_events);
  INFO("New listener to directory " << directory << " I_FD: " << m_inotify_fd << " W_FD: " << watchDescriptor);

  if (watchDescriptor == -1) {
    boost::system::system_error e(boost::system::error_code(errno, boost::asio::error::get_system_category()),
                                  "Inotify watch failed");
    boost::throw_exception(e);
  }

  {
    WatchDescriptorsMap::accessor wd_accessor;

    if (m_watchDescriptorsMap.insert(wd_accessor, watchDescriptor)) {
      wd_accessor->second = std::make_unique<Watch>(*this, directory);
    }

    wd_accessor->second->addNotifier(notifier, pattern);
  }

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

InotifyServer::Watch::Watch(InotifyServer& inotifyServer, std::string directory) :
    m_inotifyServer(inotifyServer), m_directory(std::move(directory)) {
  DEBUG("InotifyServer::Watch::Watch(" << m_directory << ")");
}

void InotifyServer::Watch::addNotifier(notifier_t notifier, const std::string& pattern) {
  DEBUG("InotifyServer::Watch::addNotifier(" << pattern << ")");
  m_notifiers.emplace_back(
      std::make_unique<InotifyServer::Watch::WatchNotifier>(std::regex(pattern), std::move(notifier)));
}

void InotifyServer::Watch::notify(EventPtr eventPtr) const {
  DEBUG("Notify: " << eventPtr->m_watchDescriptor << " Name: " << eventPtr->m_filename
                   << " Type: " << eventPtr->m_eventType);

  auto filename = m_directory + '/' + eventPtr->m_filename;

  for (auto& notifier : m_notifiers) {
    if (std::regex_match(filename, notifier->m_regex)) {
      DEBUG("Event match: " << filename);

      if (std::filesystem::is_directory(filename)) {
        DEBUG("Path: `" << filename << "` is directory. Recursive add watch to directory...");
        m_inotifyServer.addWatch(filename, notifier->m_notifier);
      } else if (std::filesystem::is_symlink(filename)) {
        auto linked = std::filesystem::read_symlink(filename);
        DEBUG("Path: `" << filename << "` is symlink. Recursive add watch to source `" << linked.string() << "`...");
        m_inotifyServer.addWatch(linked.string(), notifier->m_notifier);
      } else {
        DEBUG("Path: `" << filename << "` is file. Notify...");
        notifier->m_notifier(std::make_unique<Event>(eventPtr->m_watchDescriptor, eventPtr->m_eventType, filename));
      }
    } else {
      DEBUG("Event not match: " << filename);
    }
  }
}