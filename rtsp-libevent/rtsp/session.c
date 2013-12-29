#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/buffer.h>
#include <event2/listener.h>

#include "rtsp/session.h"

//#include "slog.h"


struct rtsp_session *
rtsp_session_new()
{
	return NULL;
}

void
rtsp_session_free(struct rtsp_session *session)
{
	;
}
