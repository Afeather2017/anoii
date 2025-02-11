set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
find_package(fmt)
set(ANOII_SOURCES
  channel.cc
  current_thread.cc
  event_loop.cc
  poller.cc
  unix_poll.cc
  timer_queue.cc
  time_helpers.cc
  acceptor.cc
  inet_addr.cc
  socket.cc
  tcp_server.cc
  tcp_connection.cc
  buffer.cc
  connector.cc
  tcp_client.cc
)
add_library(anoii ${ANOII_SOURCES})
target_include_directories(anoii PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include/)
target_link_libraries(anoii fmt::fmt)
