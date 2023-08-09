#include "net_comms.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
//#include <sys/types.h>
#include <unistd.h>

SocketBase::SocketBase() :
    m_socket_fd(-1)
{

}

SocketBase::~SocketBase() {
  close(m_socket_fd);
}

void SocketBase::_error(const char * message, s32 exit_val) {
  perror(message);
  switch(errno) {
    case EBADF:          fprintf(stderr, "EBADF\n"); break;
    case ECONNABORTED:   fprintf(stderr, "ECONNABORTED\n"); break;
    case EFAULT:         fprintf(stderr, "EFAULT\n"); break;
    case EINTR:          fprintf(stderr, "EINTR\n"); break;
    case EINVAL:         fprintf(stderr, "EINVAL\n"); break;
    case EMFILE:         fprintf(stderr, "EMFILE\n"); break;
    case ENFILE:         fprintf(stderr, "ENFILE\n"); break;
    case ENOMEM:         fprintf(stderr, "ENOMEM\n"); break;
    case ENOTSOCK:       fprintf(stderr, "ENOTSOCK\n"); break;
    case EOPNOTSUPP:     fprintf(stderr, "EOPNOTSUPP\n"); break;
    case EWOULDBLOCK:    fprintf(stderr, "EWOULDBLOCK\n"); break;

    case EACCES:         fprintf(stderr, "EACCES\n"); break;
    case EAFNOSUPPORT:   fprintf(stderr, "EAFNOSUPPORT\n"); break;
    case ENOBUFS:        fprintf(stderr, "ENOBUFS\n"); break;
    case EPROTONOSUPPORT:fprintf(stderr, "EPROTONOSUPPORT\n"); break;
    case EPROTOTYPE:     fprintf(stderr, "EPROTOTYPE\n"); break;

    case EADDRINUSE:     fprintf(stderr, "EADDRINUSE\n"); break;
    case EADDRNOTAVAIL:  fprintf(stderr, "EADDRNOTAVAIL\n"); break;
    case EDESTADDRREQ:   fprintf(stderr, "EDESTADDRREQ\n"); break;
    case EEXIST:         fprintf(stderr, "EEXIST\n"); break;
    case EIO:            fprintf(stderr, "EIO\n"); break;
    case EISDIR:         fprintf(stderr, "EISDIR\n"); break;
    case ELOOP:          fprintf(stderr, "ELOOP\n"); break;
    case ENAMETOOLONG:   fprintf(stderr, "ENAMETOOLONG\n"); break;
    case ENOENT:         fprintf(stderr, "ENOENT\n"); break;
    case ENOTDIR:        fprintf(stderr, "ENOTDIR\n"); break;
    case EROFS:          fprintf(stderr, "EROFS\n"); break;
    default:             fprintf(stderr, "unknown errno\n"); break;
  }
  if(exit_val) {
    exit(exit_val);
  }
}

u32 SocketBase::socket_write(const char * message, u32 len) {
  return write(m_socket_fd, message, len);
}

u32 SocketBase::socket_read(char * message, u32 len) {
  return read(m_socket_fd, message, len);
}

void SocketBase::validate_socket() {
  if(fcntl(m_socket_fd, F_GETFD) < 0) {
    _error("fcntl is telling us the socket descriptor is invalid.", 1);
  }
}

Server::Server(u16 port) {
  struct sockaddr_in recv_addr;

  m_socket_fd = socket(AF_INET, SOCK_STREAM, 0);

  if(m_socket_fd < 0) {
    _error("cannot create socket", 1);
  }

  validate_socket();

  memset(&recv_addr, 0, sizeof(recv_addr));

  recv_addr.sin_family = AF_INET;
  recv_addr.sin_port = htons(port);
  recv_addr.sin_addr.s_addr = INADDR_ANY;

  if(bind(m_socket_fd, (struct sockaddr*)&recv_addr, sizeof(recv_addr)) < 0) {
    _error("bind failed", 1);
  }
  validate_socket();

  if(listen(m_socket_fd, 10) < 0) {
    _error("listen failed", 1);
  }
  validate_socket();
}

bool Server::transact(const std::string & work_unit, std::string & result) {
  const u32 max_bytes = 1000;
  char buf[max_bytes];

  s32 client_fd = accept(m_socket_fd, NULL, NULL);
  if(client_fd < 0) {
    _error("error accepting transact connection", 0);
    return false;
  }

  u32 bytes_read = read(client_fd, buf, max_bytes);
  buf[bytes_read] = '\0';
  result = std::string(buf);
  write(client_fd, work_unit.c_str(), work_unit.size());
  close(client_fd);
  return true;
}


Client::Client(const char * address, u16 port) {
  struct sockaddr_in server_addr;

  m_socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if(m_socket_fd <= 0) {
    _error("cannot create socket", 1);
  }

  validate_socket();

  memset(&server_addr, 0, sizeof(server_addr));

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);

  s32 inet_pton_result = inet_pton(AF_INET, address, &server_addr.sin_addr);

  if(inet_pton_result < 0) {
    _error("first parameter is not a valid address family", 1);
  } else if(inet_pton_result == 0) {
    _error("not a valid ipaddress.", 1);
  }

  if(connect(m_socket_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
    _error("connect failed.", 1);
  }
}
