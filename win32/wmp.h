#pragma once

#ifdef __cplusplus
extern "C" {
#endif

bool wmp_init(char* app_name);
bool wmp_play(char* s);
void wmp_stop();
bool wmp_isplaying();

#ifdef __cplusplus
}
#endif

