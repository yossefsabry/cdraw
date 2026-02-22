#ifndef BACKEND_ROUTER_H
#define BACKEND_ROUTER_H

#include "utils/http.h"

void HandleRequest(const HttpRequest *req, HttpResponse *res);

#endif
