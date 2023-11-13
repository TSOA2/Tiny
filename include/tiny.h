#ifndef TINY_HEADER
#define TINY_HEADER

#define ENABLE_DEBUG 1

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>

#define ERR "\033[38;5;124m[E]\033[39m\t"
#define WARN "\033[38;5;166m[W]\033[39m\t"
#define STAT "\033[38;5;27m[I]\033[39m\t"
#define DBG "\033[38;5;92m[D]\033[39m\t"

#if ENABLE_DEBUG
#define DEBUG(x, ...) \
	do { \
		time_t t = time(NULL); \
		char buf[20]; \
		strftime(buf, 20, "%Y-%m-%d %H:%M:%S", localtime(&t)); \
		fprintf(stderr, DBG "%s | " x, buf, ##__VA_ARGS__); \
	} while (0)

#else
#define DEBUG(x, ...) {}
#endif

/*
 * MAX_REQ_SIZE = the max amount of memory (in bytes) that
 * Tiny will spend on a request.
 */

#define MAX_REQ_SIZE (1024)

#define HTTP_HEADER_MAX_LEN (256)

/*
 * Will these LIKELY help with performance? UNLIKELY,
 * but there's no hurt in using them (to check errors).
 *
 * Example:
 * if (UNLIKELY(function() < 0)) { <---- (-1 = error)
 * 		...
 * }
 */

// #define LIKELY(x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)


#define MAX_PATH_LEN (4096)

typedef enum {
	HTTP_GET,
	HTTP_POST,
	/*
	 * ...
	 */
} HTTP_method;

typedef enum {
	HTTP_STATUS_200,
	HTTP_STATUS_404,
	/*
	 * ...
	 */
} HTTP_status_code;


typedef struct {
	HTTP_method method;
	uint8_t path[MAX_PATH_LEN + 1];
	uint8_t *payload;
} HTTP_request;

void handle_request(int client_socket);

typedef struct {
	HTTP_status_code status_code;
	uint8_t *buffer;
	size_t buffer_size;
} HTTP_response;

/*
 * Module function prototypes
 */

int request_callback(HTTP_request, HTTP_response *);
#endif // TINY_HEADER
