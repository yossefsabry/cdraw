#include "http.h"
#include "../config.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>

static int FindHeaderInt(const char *headers, const char *name) {
  if (!headers || !name)
    return -1;
  size_t name_len = strlen(name);
  const char *p = headers;
  while (*p) {
    const char *line = p;
    const char *line_end = strstr(line, "\r\n");
    if (!line_end)
      line_end = line + strlen(line);
    if ((size_t)(line_end - line) >= name_len + 1) {
      if (strncasecmp(line, name, name_len) == 0 &&
          line[name_len] == ':') {
        const char *v = line + name_len + 1;
        while (*v == ' ' || *v == '\t')
          v++;
        return atoi(v);
      }
    }
    if (*line_end == '\0')
      break;
    p = line_end + 2;
  }
  return -1;
}

static void StripQuery(char *path) {
  if (!path)
    return;
  char *q = strchr(path, '?');
  if (q)
    *q = '\0';
}

void HttpInitResponse(HttpResponse *res) {
  if (!res)
    return;
  res->status = 200;
  res->content_type = "application/json";
  res->body = NULL;
}

void HttpFreeRequest(HttpRequest *req) {
  if (!req)
    return;
  free(req->body);
  req->body = NULL;
  req->body_len = 0;
}

void HttpFreeResponse(HttpResponse *res) {
  if (!res)
    return;
  free(res->body);
  res->body = NULL;
}

bool HttpReadRequest(int fd, HttpRequest *req, char *err, size_t err_sz) {
  if (!req)
    return false;
  memset(req, 0, sizeof(*req));

  char *buf = (char *)malloc(MAX_REQUEST_SIZE + 1);
  if (!buf)
    return false;

  size_t len = 0;
  ssize_t n = 0;
  while (len < MAX_REQUEST_SIZE) {
    n = recv(fd, buf + len, MAX_REQUEST_SIZE - len, 0);
    if (n <= 0)
      break;
    len += (size_t)n;
    buf[len] = '\0';
    if (strstr(buf, "\r\n\r\n"))
      break;
  }
  if (len == 0) {
    free(buf);
    return false;
  }

  char *header_end = strstr(buf, "\r\n\r\n");
  if (!header_end) {
    free(buf);
    if (err && err_sz > 0)
      snprintf(err, err_sz, "invalid headers");
    return false;
  }
  size_t header_len = (size_t)(header_end - buf) + 4;

  char *line_end = strstr(buf, "\r\n");
  if (!line_end) {
    free(buf);
    return false;
  }
  *line_end = '\0';
  sscanf(buf, "%7s %255s", req->method, req->path);
  StripQuery(req->path);

  int content_len = FindHeaderInt(line_end + 2, "Content-Length");
  if (content_len < 0)
    content_len = 0;

  size_t have_body = len - header_len;
  size_t need_body = (size_t)content_len;
  while (have_body < need_body && len < MAX_REQUEST_SIZE) {
    n = recv(fd, buf + len, MAX_REQUEST_SIZE - len, 0);
    if (n <= 0)
      break;
    len += (size_t)n;
    have_body += (size_t)n;
  }

  if (need_body > 0) {
    req->body = (char *)malloc(need_body + 1);
    if (!req->body) {
      free(buf);
      return false;
    }
    memcpy(req->body, buf + header_len, need_body);
    req->body[need_body] = '\0';
    req->body_len = need_body;
  }

  free(buf);
  return true;
}

static const char *StatusText(int status) {
  switch (status) {
  case 200:
    return "OK";
  case 400:
    return "Bad Request";
  case 401:
    return "Unauthorized";
  case 404:
    return "Not Found";
  case 502:
    return "Bad Gateway";
  default:
    return "Error";
  }
}

bool HttpSendResponse(int fd, const HttpResponse *res) {
  if (!res)
    return false;
  const char *body = res->body ? res->body : "";
  size_t body_len = strlen(body);
  char header[512];
  int n = snprintf(header, sizeof(header),
                   "HTTP/1.1 %d %s\r\n"
                   "Content-Type: %s\r\n"
                   "Content-Length: %zu\r\n"
                   "Connection: close\r\n"
                   "\r\n",
                   res->status,
                   StatusText(res->status),
                   res->content_type ? res->content_type : "application/json",
                   body_len);
  if (n <= 0)
    return false;
  send(fd, header, (size_t)n, 0);
  if (body_len > 0)
    send(fd, body, body_len, 0);
  return true;
}
