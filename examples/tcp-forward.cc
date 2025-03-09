// 一个简单的TCP流量转发程序。
#include "event_loop.h"
#include "inet_addr.h"
#include "tcp_server.h"
EventLoop loop{};
InetAddr addr{9996};
TcpServer server{&loop, addr};
int main() {}
