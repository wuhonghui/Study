#ifndef receiver_H
#define receiver_H

#ifdef __cplusplus
extern "C" {
#endif

enum RECEIVER_TYPE{
	RECEIVER_INTERNAL,
	RECEIVER_HTTP,
	RECEIVER_ONCE,
	RECEIVER_RTP,
};

enum DEVICE_STATUS{
	DEVICE_RUN_STREAMING,
	DEVICE_RUN_NOT_STREAMING,
	DEVICE_DOWN,
	DEVICE_STATUS_NUM,
};

#define RECEIVER_DIR "/var/run/streamd/rcver/"

struct generic_receiver{
	/* caller specify */
	int (*cb)(void *data, size_t len, void *arg);
	void *arg;

	/* statistics */
	char *identity;
	int cbcount;
	struct timeval begin;

	/* list */
	struct generic_receiver *next;
};

int receiver_identity_init();
int receiver_identity_clear();

int receiver_identity_del(const char *identity);
int receiver_identity_add(const char *identity);

int receiver_identity_is_exist(const char *identity);

#ifdef __cplusplus
}
#endif

#endif
