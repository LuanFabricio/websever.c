#ifndef __HANDLERS_H
#define __HANDLERS_H

#include "request.h"

typedef void (*handle_request_t)(int, create_response_header_t);

#endif // __HANDLERS_H
