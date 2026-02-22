#include "health.h"
#include "../config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void HandleHealth(const HttpRequest *req, HttpResponse *res) {
  (void)req;
  if (!res)
    return;
  res->status = 200;
  res->content_type = "application/json";
  char buf[128];
  snprintf(buf, sizeof(buf),
           "{\"status\":\"ok\",\"port\":%d,\"version\":\"1.0.0\"}",
           SERVER_PORT);
  res->body = strdup(buf);
}
