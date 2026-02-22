#ifndef BACKEND_HTTP_H
#define BACKEND_HTTP_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
  char method[8];
  char path[256];
  char *body;
  size_t body_len;
} HttpRequest;

typedef struct {
  int status;
  const char *content_type;
  char *body;
} HttpResponse;

bool HttpReadRequest(int fd, HttpRequest *req, char *err, size_t err_sz);
bool HttpSendResponse(int fd, const HttpResponse *res);
void HttpFreeRequest(HttpRequest *req);
void HttpInitResponse(HttpResponse *res);
void HttpFreeResponse(HttpResponse *res);

#endif
