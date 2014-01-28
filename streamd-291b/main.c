#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <time.h>

#include "event2/event.h"
#include "event2/event_struct.h"
#include "event2/buffer.h"
#include "event2/util.h"
#include "event2/listener.h"

#include "common.h"
#include "slog.h"
#include "media_source.h"

#include "input_sonix_291b.h"
#include "input_mjpeg_device.h"
#include "input_h264_device.h"
#include "input_sound_device.h"

#include "httplib.h"
#include "record_init.h"
#include "stat_init.h"

#define SERVER_PID_FILE		"/var/run/streamd.pid"

static int create_pid_file()
{
	struct stat pid_stat;
	if (stat(SERVER_PID_FILE, &pid_stat) == 0) {
		/* send siguser1 to existed process to reload config */
		SLOG(SLOG_INFO, "Could not create pid file, file exist, check its pid");

		char pidbuf[64];
		int pid_fd = open(SERVER_PID_FILE, O_RDONLY);
		if (pid_fd < 0) {
			SLOG(SLOG_ERROR, "could not open file /var/run/streamd.pid");
			return -1;
		}

		int nread = 0;
		nread = read(pid_fd, pidbuf, sizeof(pidbuf)-1);
		if (nread < 0) {
			SLOG(SLOG_ERROR, "could not read file /var/run/streamd.pid");
			close(pid_fd);
			return -1;
		}

		close(pid_fd);

		int pid = 0;
		pid = atoi(pidbuf);
		if (pid <= 0) {
			SLOG(SLOG_ERROR, "read malformat pid");
			return -1;
		}

		if (kill(pid, 0)) {
			if (errno == ESRCH) {	/* process not found, delete file & continue */
				unlink(SERVER_PID_FILE);
			} else {
				SLOG(SLOG_ERROR, "pid %d,  %s", pid, strerror(errno));
				return -1;
			}
		}else {
			// TODO: let exist process reload config

			return -1;
		}
	}

	/* pid file not exist, ok, here we create */
	int server_pid_fd = 0;
	server_pid_fd = open(SERVER_PID_FILE, O_WRONLY | O_CREAT | O_EXCL | O_TRUNC,
		S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (server_pid_fd < 0) {
		SLOG(SLOG_ERROR, "Could not create pid file, %s", strerror(errno));
		return -1;
	}

	int pid = 0;
	char pidbuf[32];
	memset(pidbuf, 0, sizeof(pidbuf));
	pid = getpid();
	snprintf(pidbuf, 32, "%d\n", pid);

	write(server_pid_fd, pidbuf, strlen(pidbuf));
	close(server_pid_fd);

	return 0;
}

static int delete_pid_file()
{
	if (unlink(SERVER_PID_FILE)) {
		SLOG(SLOG_ERROR, "Could not delete pid file %d", errno);
		return -1;
	}
	return 0;
}

static void signal_quit_cb(evutil_socket_t sig, short events, void *arg)
{
	struct event_base *base = arg;
	struct timeval delay = { 0, 5 * 1000 };

	event_base_loopexit(base, &delay);
}

int main(int argc, char *argv[])
{
	/* chdir to /, not dirent output to /dev/null */
	//if (daemon(0, 1)) {
	//	return -1;
	//}

	if (slog_init(SLOG_DEBUG)) {
		return -1;
	}

	/* create pid file */
	if (create_pid_file()) {
		return -1;
	}

	struct event_base *base = NULL;
	struct event *sigint_event = NULL;
	struct http_server *hs = NULL;

	/* create event base */
	base = event_base_new();
	if (!base) {
		goto exit;
	}

	/* install signal handle */
	//signal(SIGPIPE, SIG_IGN);
	//signal(SIGUSR1, SIG_IGN);
	//signal(SIGUSR2, SIG_IGN);
	//signal(SIGTERM, SIG_IGN);
	//signal(SIGINT,  SIG_IGN);
	sigint_event = evsignal_new(base, SIGINT, signal_quit_cb, (void *)base);
	if (sigint_event) {
		event_add(sigint_event, NULL);
	}

	if (server_media_source_init()) {
		goto exit;
	}

	/* input devices init */
	if (input_sonix_291b_init()) {
		goto exit;
	}
	
	if (input_mjpeg_device_init(base)) {
		goto exit;
	}

	if (input_h264_device_init(base)) {
		goto exit;
	}
	
	//if (input_sound_device_init(base)) {
	//	goto exit;
	//}

	/* output modules init */
	
	/* Test: record nalu/mjpeg to file */
	//if (record_init()) {
	//	goto exit;
	//}

	/* Test: streams statistics */
	//if (stat_init()) {
	//	goto exit;
	//}

	/* start http server */
    hs = http_server_new(base, 8080);
    if (!hs) {
        SLOG(SLOG_ERROR, "Could not create http server");
        goto exit;
    }

	SLOG(SLOG_INFO, "Server begin eventloop");

	/* start dispatch */
	event_base_dispatch(base);
	/* fall throught when exit from dispatch loop */
exit:
	SLOG(SLOG_INFO, "Server exit");

	if (hs) {
		http_server_free(hs);
	}

	//stat_destroy();
	//record_destroy();

	//input_sound_device_close();
	input_h264_device_close();
	input_mjpeg_device_close();

	server_media_source_destroy();

	if (sigint_event)
		evsignal_del(sigint_event);

	if (base)
		event_base_free(base);

	delete_pid_file();

	return 0;
}
