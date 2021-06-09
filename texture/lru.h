#pragma once

typedef unsigned int (*user_add_cb)(const char* key, void* user);
typedef unsigned int (*user_del_cb)(const char* key, void* value, void* user);

void cache_callback_userdata(void* user);
void cache_callback_add(user_add_cb callback);
void cache_callback_del(user_del_cb callback);

void cache_set_size(int size);

int find_in_cache(const char* key);
void add_to_cache(const char* key, int value);
