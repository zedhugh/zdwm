#pragma once

typedef struct runtime_init_desc_t runtime_init_desc_t;

bool runtime_config_load(const char *override_path, runtime_init_desc_t *out);
void runtime_config_cleanup(runtime_init_desc_t *desc);
