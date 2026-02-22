#include "router.h"
#include "handlers/ai.h"
#include "handlers/calculate.h"
#include "handlers/health.h"
#include <stdio.h>
#include <string.h>

void HandleRequest(const HttpRequest *req, HttpResponse *res) {
  if (!req || !res)
    return;

  if (strcmp(req->method, "GET") == 0 && strcmp(req->path, "/health") == 0) {
    HandleHealth(req, res);
    return;
  }
  if (strcmp(req->method, "POST") == 0 && strcmp(req->path, "/ai/analyze") == 0) {
    HandleAiAnalyze(req, res);
    return;
  }
  if (strcmp(req->method, "POST") == 0 && strcmp(req->path, "/calculate") == 0) {
    HandleCalculate(req, res);
    return;
  }

  res->status = 404;
  res->body = strdup("{\"status\":\"error\",\"message\":\"not found\"}");
}
