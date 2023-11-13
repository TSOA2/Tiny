/*
 * ==== Modules ====
 * This file contains the callback function for a HTTP
 * request. This allows a little bit of modularity.
 *
 * typedef struct {
 * 		HTTP_method method;
 * 		uint8_t path[MAX_PATH_LEN + 1];
 * 		uint8_t *payload;
 * } HTTP_request;
 *
 * typedef struct {
 * 		HTTP_status_code status_code;
 * 		uint8_t *buffer;
 * 		size_t buffer_size;
 * } HTTP_response;
 *
 * The response buffer MUST be malloc'd, and is assumed
 * to be free'd by Tiny.
 * You do not have to free the request payload.
 *
 * Not all of these have been implemented, because I made
 * Tiny for a simple project that didn't need all of them.
 *
 * HTTP_method enum list:
 *  - HTTP_GET
 *  - HTTP_POST
 *
 * HTTP_status_code enum list:
 *  - HTTP_STATUS_200
 *  - HTTP_STATUS_404
 *
 * Function prototype:
 *
 * int request_callback(HTTP_request request, HTTP_response *response);
 *
 * Return -1 on failure, 0 on success.
 */

#include <tiny.h>

#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#define HOMEPAGE_FILE ("./media/test.html")

static uint8_t *get_file_contents(const char *name, size_t *file_size)
{
	int fd;
	struct stat file_stat;
	uint8_t *buffer;

	fd = open(name, O_RDONLY);
	if (UNLIKELY(fd < 0)) {
		perror(ERR "get_file_contents():open()");
		return NULL;
	}

	if (UNLIKELY(fstat(fd, &file_stat) < 0)) {
		perror(ERR "get_file_contents():fstat()");
		close(fd);
		return NULL;
	}

	buffer = malloc((size_t) file_stat.st_size + 1);
	if (UNLIKELY(buffer == NULL)) {
		perror(ERR "get_file_contents():malloc()");
		close(fd);
		return NULL;
	}

	if (UNLIKELY(read(fd, buffer, file_stat.st_size) != file_stat.st_size)) {
		perror(ERR "get_file_contents():read()");
		close(fd);
		free(buffer);
		return NULL;
	}

	buffer[file_stat.st_size] = '\0';
	*file_size = file_stat.st_size + 1;
	return buffer;
}

int request_callback(HTTP_request request, HTTP_response *response)
{
	/*
	 * Caching the contents of the file may be implemented later
	 */

	switch (request.method) {
		case HTTP_GET: {
			response->buffer = get_file_contents(HOMEPAGE_FILE, &response->buffer_size);
			if (UNLIKELY(response->buffer == NULL)) {
				return -1;
			}

			response->status_code = HTTP_STATUS_200;
			break;
		}
		case HTTP_POST: {
			DEBUG("recieved post request.\n");
			uint8_t msg[] = "Recieved a post request!\n";
			response->buffer = malloc(sizeof(msg));
			if (UNLIKELY(response->buffer == NULL)) {
				return -1;
			}

			response->buffer_size = sizeof(msg);
			memcpy(response->buffer, msg, sizeof(msg));
//			DEBUG("response buffer: %s\n", response->buffer);
			response->status_code = HTTP_STATUS_200;
			break;
		}
		default: return -1;
	}


	return 0;
}
