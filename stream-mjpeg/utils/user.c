#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "common.h"

#include "utils/user.h"

#define USER_PASSWD_MAXLEN 32

struct user_info{
	char *username;
	char *password;
	int verify_count;
	struct user_info *next;
};

static struct user_info user_list = {NULL, NULL, 0, NULL};

int add_user(char *user, char *password)
{
	struct user_info *new_user = NULL;

	if (!user || !password)
		return -1;

	if (strlen(user) > USER_PASSWD_MAXLEN || strlen(password) > USER_PASSWD_MAXLEN)
		return -1;
	
	new_user = malloc(sizeof(struct user_info));
	if (!new_user) {
		printf("Could not add new user(ENOMEM)\n");
		return -1;
	}

	new_user->username = strdup(user);
	new_user->password = strdup(password);
	new_user->verify_count = 0;
	new_user->next = NULL;

	/* append to head node */
	new_user->next = user_list.next;
	user_list.next = new_user;
	
	return 0;
}

int user_list_empty()
{
	return (user_list.next == NULL);
}

int user_get_password(const char *user, char *password)
{
	struct user_info *p = user_list.next;

	/* no user in list */
	if (p == NULL) {
		return 0;
	}
	
	while (p != NULL) {
		if (strcmp(p->username, user) == 0) {
			strncpy(password, p->password, USER_PASSWD_MAXLEN);
			return 0;
		}
	}
	
	return -1;
}

