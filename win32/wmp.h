#pragma once

#ifdef __cplusplus
extern "C" {
#endif

bool wmp_init(char* app_name);
bool wmp_play(char* s);
void wmp_stop();
bool wmp_isplaying();

#define IGNORE_RETVAL(expr) do { (void)(expr); } while(0)

#ifdef __cplusplus
}
#endif

