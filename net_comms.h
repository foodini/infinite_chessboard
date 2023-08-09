#include "util.h"

#include <string>

class SocketBase {
protected:
  s32 m_socket_fd;

  void _error(const char * message, s32 exit_val);

public:
  SocketBase();
  ~SocketBase();

  void validate_socket();

  u32 socket_write(const char * message, u32 len);
  u32 socket_read(char * message, u32 len);
};

class Server: public SocketBase {
private:

public:
  Server(u16 port);
  bool transact(const std::string & work_unit, std::string & result);
};

class Client: public SocketBase {
private:

public:
  Client(const char * address, u16 port);

};
