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
#include "input_video.h"
#include "input_sound.h"
#include "httplib.h"
#include "utils/user.h"

#define SERVER_PID_FILE		"/var/run/streamd.pid"

static int create_pid_file()
{
	struct stat pid_stat;
	if (stat(SERVER_PID_FILE, &pid_stat) == 0) {
		/* send siguser1 to existed process to reload config */
		printf("Could not create pid file, file exist\n");

		char pidbuf[64];
		int pid_fd = open(SERVER_PID_FILE, O_RDONLY);
		if (pid_fd < 0) {
			printf("could not open file /var/run/streamd.pid\n");
			return -1;
		}

		int nread = 0;
		nread = read(pid_fd, pidbuf, sizeof(pidbuf)-1);
		if (nread < 0) {
			printf("could not read file /var/run/streamd.pid\n");
			close(pid_fd);
			return -1;
		}

		close(pid_fd);

		int pid = 0;
		pid = atoi(pidbuf);
		if (pid <= 0) {
			printf("read malformat pid\n");
			return -1;
		}

		if (kill(pid, 0)) {
			if (errno == ESRCH) {	/* process not found, delete file & continue */
				unlink(SERVER_PID_FILE);
			} else {
				printf("pid %d,  %s\n", pid, strerror(errno));
				return -1;
			}
		}else {
			return -1;
		}
	}

	/* pid file not exist, ok, here we create */
	int server_pid_fd = 0;
	server_pid_fd = open(SERVER_PID_FILE, O_WRONLY | O_CREAT | O_EXCL | O_TRUNC,
		S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (server_pid_fd < 0) {
		printf("Could not create pid file, %s\n", strerror(errno));
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
		printf("Could not unlink pid file %d\n", errno);
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

#define PROGRAME_NAME "stream-mjpeg"

static int width = 640;
static int height = 480;
static int fps = 20;
static int max_connection = 10;
static char *video_dev = "/dev/video0";
static int http_port = 80;

void help()
{
	fprintf(stderr, "stream-mjpeg [options]\n");
	fprintf(stderr, "options:\n");
	fprintf(stderr, 
"   -u/--user [username]      username\n"
"   -p/--password [password]  password\n"
"   -f/--fps [n]              video fps\n"
"   -r/--resolution [nxm]     video resolution, 640*480 etc.\n"
"   -m/--max-connection [n]   max video connection\n"
"   -P/--http-port [n]        http port \n"
"   -D                        run as a daemon\n"
"   -h                        show this help message\n");
	exit(1);
}

int parse_opt(int argc, char *argv[])
{
	int i = 0;
	char *end = NULL;
	char *user = NULL;
	char *password = NULL;
	
	for (i = 1; i < argc; i++) {
		if ((strcmp(argv[i], "-u") == 0 ||
			strcmp(argv[i], "--user") == 0) && (i+1) < argc) {
			user = argv[++i];
		}
		else if ((strcmp(argv[i], "-p") == 0 ||
			strcmp(argv[i], "--password") == 0) && (i+1) < argc) {
			password = argv[++i];
		}
		else if ((strcmp(argv[i], "-f") == 0 ||
			strcmp(argv[i], "--fps") == 0) && (i+1) < argc) {
			fps = strtol(argv[++i], &end, 10);
			if (*end != '\0') {
				fprintf(stderr, "unrecognized value %s\n", argv[i]);
				help();
			}
		}
		else if ((strcmp(argv[i], "-m") == 0 ||
			strcmp(argv[i], "--max-connection") == 0) && (i+1) < argc) {
			max_connection = strtol(argv[++i], &end, 10);
			if (*end != '\0') {
				fprintf(stderr, "unrecognized value %s\n", argv[i]);
				help();
			}
		}
		else if ((strcmp(argv[i], "-r") == 0 ||
			strcmp(argv[i], "--resolution") == 0) && (i+1) < argc) {
			char str_num[100];
			char *pstr = NULL;
			int p_index = 0;
			pstr = argv[++i];
			memset(str_num, 0, sizeof(str_num));
			while(isdigit(*pstr) && p_index < (sizeof(str_num) - 1)) {
				str_num[p_index++] = *pstr++;
			}
			str_num[p_index] = 0;
			width = strtol(str_num, &end, 10);
			if (*end != '\0') {
				fprintf(stderr, "unrecognized value %s\n", argv[i]);
				help();
			}

			pstr++;
			p_index = 0;
			while(isdigit(*pstr) && p_index < (sizeof(str_num) - 1)) {
				str_num[p_index++] = *pstr++;
			}
			str_num[p_index] = 0;
			height = strtol(str_num, &end, 10);
			if (*end != '\0') {
				fprintf(stderr, "unrecognized value %s\n", argv[i]);
				help();
			}
		}
		else if ((strcmp(argv[i], "-P") == 0 ||
			strcmp(argv[i], "--http-port") == 0) && (i+1) < argc) {
			http_port = strtol(argv[++i], &end, 10);
			if (*end != '\0') {
				fprintf(stderr, "unrecognized value %s\n", argv[i]);
				help();
			}
		}
		else if ((strcmp(argv[i], "-d") == 0 ||
			strcmp(argv[i], "--device") == 0) && (i+1) < argc) {
			video_dev = argv[++i];
		} else if ((strcmp(argv[i], "-D") == 0 ||
			strcmp(argv[i], "--daemon") == 0)) {
			daemon(0, 1);
		} else if ((strcmp(argv[i], "-h") == 0 ||
			strcmp(argv[i], "--help") == 0)) {
			help();
		} else {
			fprintf(stderr, "unrecognized option %s\n", argv[i]);
			help();
		}
	}

	if (user && password) {
		add_user(user, password);
	}

#ifdef DEBUG
	printf("Configurations: \n");
	printf("\twidth: %d, height:%d, fps: %d\n", width, height, fps);
	printf("\tdevice: %s\n", video_dev);
	printf("\tmax connections: %d\n", max_connection);
	printf("\thttp port: %d\n", http_port);
	if (user && password) {
		printf("\tuser: %s\n\tpassword: %s\n", user, password);
	}
#endif

	return 0;
}

int main(int argc, char *argv[])
{
	struct event_base *base = NULL;
	struct http_server *hs = NULL;
	struct event *sigint_event = NULL;
	struct event *sigterm_event = NULL;

	parse_opt(argc, argv);
	
	/* create pid file */
	if (create_pid_file()) {
		return -1;
	}

	/* create event base */
	base = event_base_new();
	if (!base) {
		return -1;
	}
	
	/* install signal handle */
	sigint_event = evsignal_new(base, SIGINT, signal_quit_cb, (void *)base);
	if (sigint_event) {
		event_add(sigint_event, NULL);
	}
	sigterm_event = evsignal_new(base, SIGTERM, signal_quit_cb, (void *)base);
	if (sigterm_event) {
		event_add(sigterm_event, NULL);
	}

	/* init video device, and start capture */
	video_init(video_dev, width, height, fps, max_connection);
	video_start(base);

	/* init sound device, trigger device */
	//sound_init();
	//sound_start(base);

	/* create http server */
	hs = http_server_new(base, http_port);
	if (!hs) {
		dbprintf("Could not create new http server\n");
		goto close;
	}

	/* start dispatch */
	event_base_dispatch(base);
	/* fall throught when exit from dispatch loop */
close:
	if (hs) {
		http_server_free(hs);
	}

	/* untrigger and close device */
	//sound_stop();
	//sound_close();

	/* stop capture and close device */
	video_stop();
	video_close();

	/* uninstall signal handler */
	if (sigterm_event)
		evsignal_del(sigterm_event);
	if (sigint_event)
		evsignal_del(sigint_event);
	
	if (base)
		event_base_free(base);

	delete_pid_file();

	return 0;
}
