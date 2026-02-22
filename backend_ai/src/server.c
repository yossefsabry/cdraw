#include "server.h"
#include "config.h"
#include "router.h"
#include "utils/http.h"
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static volatile sig_atomic_t g_running = 1;

static void OnSignal(int sig) {
  (void)sig;
  g_running = 0;
}

int ServerRun(void) {
  signal(SIGTERM, OnSignal);
  signal(SIGINT, OnSignal);

  int server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
    fprintf(stderr, "backend_ai: socket failed: %s\n", strerror(errno));
    return 1;
  }

  int opt = 1;
  setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  addr.sin_port = htons(SERVER_PORT);

  if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    fprintf(stderr, "backend_ai: bind failed: %s\n", strerror(errno));
    close(server_fd);
    return 1;
  }
  if (listen(server_fd, 16) < 0) {
    fprintf(stderr, "backend_ai: listen failed: %s\n", strerror(errno));
    close(server_fd);
    return 1;
  }

  fprintf(stderr, "backend_ai: listening on 127.0.0.1:%d\n", SERVER_PORT);

  while (g_running) {
    struct sockaddr_in client;
    socklen_t len = sizeof(client);
    int fd = accept(server_fd, (struct sockaddr *)&client, &len);
    if (fd < 0) {
      if (errno == EINTR)
        continue;
      fprintf(stderr, "backend_ai: accept failed: %s\n", strerror(errno));
      break;
    }

    HttpRequest req;
    HttpResponse res;
    HttpInitResponse(&res);
    char err[128] = {0};
    if (!HttpReadRequest(fd, &req, err, sizeof(err))) {
      res.status = 400;
      res.body = strdup("{\"status\":\"error\",\"message\":\"invalid request\"}");
    } else {
      HandleRequest(&req, &res);
    }
    HttpSendResponse(fd, &res);
    HttpFreeRequest(&req);
    HttpFreeResponse(&res);
    close(fd);
  }

  close(server_fd);
  return 0;
}
