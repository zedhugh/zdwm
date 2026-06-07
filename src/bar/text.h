#pragma once

#include <stdint.h>

typedef struct text_context_t text_context_t;

text_context_t *
text_context_create(const char *family, uint32_t size, uint32_t dpi);
void text_context_destory(text_context_t *context);
