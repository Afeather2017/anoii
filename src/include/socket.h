#pragma once
#ifndef SOCKET_H
#define SOCKET_H
class InetAddr;
int GetSockError(int sockfd);
void IgnoreSigPipe();
void SetTcpNoDelay(int fd);
void SetTcpKeeyAlive(int fd);
void SetCloseExecNonBlock(int fd);
InetAddr GetLocalAddr(int sockfd);
InetAddr GetPeerAddr(int sockfd);
#endif  // SOCKET_H
