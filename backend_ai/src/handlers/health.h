#ifndef BACKEND_HEALTH_H
#define BACKEND_HEALTH_H

#include "../utils/http.h"

void HandleHealth(const HttpRequest *req, HttpResponse *res);

#endif
