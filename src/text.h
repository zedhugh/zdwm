#pragma once

#include <stdint.h>

int text_init_pango_layout(const char *family, uint32_t size, uint32_t dpi);
void text_clean_pango_layout(void);
