#pragma once

#include <stdint.h>

void xres_init_xrm_db(void);
void xres_clean(void);
bool xres_get_uint32(const char *name, uint32_t *value);
void xres_get_string(const char *name, char **value, const char *fallback);
