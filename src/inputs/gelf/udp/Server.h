//
// Server.h
// ~~~~~~~~~~~~~~~~
//
// Copyright (C) 2018 IQOption Software, Inc.
//
//

#pragma once

#include <boost/asio.hpp>

#include <array>
#include <inputs/Input.h>
#include <iostream>

#include "GelfChunk.h"
#include "MessageQueue.h"
#include "formats/gelf/GelfException.h"
#include "inputs/Record.h"
#include "inputs/gelf/Gelf.h"

using namespace iqlogger::inputs;
using namespace iqlogger::inputs::gelf;

namespace iqlogger::inputs::gelf::udp {

using boost::asio::ip::udp;

constexpr size_t buffer_size = 4096;
using buffer_t = std::array<char, buffer_size>;

class Server
{
  class udp_session : public std::enable_shared_from_this<udp_session>
  {
  public:
    using pointer = std::shared_ptr<udp_session>;

    static pointer create(std::string name, RecordQueuePtr<Gelf> queuePtr, ChunkQueuePtr chunkQueuePtr) {
      return std::make_shared<udp_session>(std::move(name), queuePtr, chunkQueuePtr);
    }

    buffer_t& buffer() { return m_buffer; }

    udp::endpoint& remote_endpoint() { return m_remote_endpoint; }

    void process(const boost::system::error_code& error, std::size_t bytes_transferred);

    udp_session(std::string name, RecordQueuePtr<Gelf> queuePtr, ChunkQueuePtr chunkQueuePtr) :
        m_name(std::move(name)), m_queuePtr(queuePtr), m_chunkQueuePtr(chunkQueuePtr) {}

  private:
    std::string m_name;
    buffer_t m_buffer;
    RecordQueuePtr<Gelf> m_queuePtr;
    ChunkQueuePtr m_chunkQueuePtr;
    udp::endpoint m_remote_endpoint;
  };

private:
  std::string m_name;
  udp::socket m_socket;
  boost::asio::io_service::strand m_strand;
  RecordQueuePtr<Gelf> m_queuePtr;
  ChunkQueuePtr m_chunkQueuePtr;

public:
  explicit Server(std::string name, RecordQueuePtr<Gelf> queuePtr, ChunkQueuePtr chunkQueuePtr,
                  boost::asio::io_service& io_service, unsigned short port) :
      m_name(std::move(name)),
      m_socket(io_service, udp::endpoint(udp::v4(), port)),
      m_strand(io_service),
      m_queuePtr(queuePtr),
      m_chunkQueuePtr(chunkQueuePtr) {
    do_receive();
  }

  void do_receive() {
    TRACE("do_receive()");

    udp_session::pointer new_session = udp_session::create(m_name, m_queuePtr, m_chunkQueuePtr);

    auto handler = [this, new_session](const boost::system::error_code& error, std::size_t bytes_transferred) {
      handle_receive(new_session, error, bytes_transferred);
    };

    m_socket.async_receive_from(boost::asio::buffer(new_session->buffer()), new_session->remote_endpoint(),
                                m_strand.wrap(handler));
  }

  void handle_receive(udp_session::pointer new_session, const boost::system::error_code& error,
                      std::size_t bytes_transferred) {
    TRACE("handle_receive()");

    if (!error && (bytes_transferred > 0)) {
      auto handle = [new_session, error, bytes_transferred]() { new_session->process(error, bytes_transferred); };

      m_strand.post(handle);
    } else {
      WARNING("Error (receive): " << error.message() << ". Bytes: " << bytes_transferred);
    }

    do_receive();
  }
};
}  // namespace iqlogger::inputs::gelf::udp
