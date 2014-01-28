#ifndef envirment_H
#define envirment_H

#ifdef __cplusplus
extern "C" {
#endif

void envirment_signal_handler();
void envirment_std_redirent();
void set_core_limit();
void set_oom_adj();

#ifdef __cplusplus
}
#endif


#endif

