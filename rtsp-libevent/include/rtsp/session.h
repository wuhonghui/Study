#ifndef SESSION_H
#define SESSION_H

enum RTSP_SESSION_STATUS{
	SESSION_START,
	SESSION_SETUP,
	SESSION_NONE,
};

struct rtsp_session {
	char id[32];
	struct rtsp_session *prev;
	struct rtsp_session *next;
};

#endif
