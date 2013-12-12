#ifndef user_H
#define user_H

#ifdef __cplusplus
extern "C" {
#endif

int add_user(char *user, char *password);
int user_get_password(const char *user, char *password);
int user_list_empty();

#ifdef __cplusplus
}
#endif


#endif

