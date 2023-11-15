/*
 * Tiny HTTP parser
 * NOTE: Tiny doesn't look at any of the headers,
 * it just looks at the method, path, and payload.
 */

#include <tiny.h>

static const struct translation {
	HTTP_method method;
	char name[8];
} methods[] = {
	{HTTP_GET, "GET"},
	{HTTP_POST, "POST"},
};

static const unsigned short num_methods = sizeof(methods) / sizeof(struct translation);

const char *status_codes[] = {
	[HTTP_STATUS_200] = "200 OK",
	[HTTP_STATUS_404] = "404 Not Found",
};

static size_t recv_data(int client_socket, uint8_t buffer[MAX_REQ_SIZE], int *error)
{
	ssize_t recvd;

	*error = 0;
	if ((recvd=recv(client_socket, buffer, MAX_REQ_SIZE, 0)) < 0) {
		perror(ERR "recv_data():recv()");
		*error = 1;
	}

	return (size_t) recvd;
}

static int send_data(int client_socket, HTTP_response response)
{
	char http_header[HTTP_HEADER_MAX_LEN];
	char utc_time[32];
	time_t current_time;
	size_t header_len;

	current_time = time(NULL);
	strftime(utc_time, sizeof(utc_time), "%a, %d %b %Y %H:%M:%S GMT", localtime(&current_time));
	sprintf(http_header,
		"HTTP/1.1 %s\r\n"
		"Date: %s\r\n"
		"Server: Tiny\r\n"
		"Connection: Keep-Alive\r\n"
		"Content-Type: text/html\r\n"
		"Server: Tiny\r\n\r\n",
		status_codes[response.status_code],
		utc_time
	);

	header_len = strlen(http_header);

	if (send(client_socket, http_header, header_len, 0) != header_len) {
		perror(ERR "send_data():send()");
		return -1;
	}

	if (send(client_socket, response.buffer, response.buffer_size, 0) != response.buffer_size) {
		perror(ERR "send_data():send()");
		return -1;
	}
	return 0;
}

/*
 * Make sure the next four elements of a buffer aren't null terminators
 */
#define _CHECK_ZERO4(x) (x[0]!='\0'&&x[1]!='\0'&&x[2]!='\0'&&x[3]!='\0')

/*
 * Compare the next four elements of the buffer with arguments
 */

#define _CMP4(x, a, b, c, d) (x[0]==a&&x[1]==b&&x[2]==c&&x[3]==d)

static int parse_request(HTTP_request *request, uint8_t *buffer, size_t buffer_size)
{
	size_t i = 0;
	const char delim[] = " \t\r\n";
	char *token;

	/*======= METHOD =======*/
	token = strtok((char *) buffer, delim);
	if (token == NULL) {
		return -1;
	}

	for (i = 0; i < num_methods; i++) {
		if (strcmp(methods[i].name, token) == 0) {
			DEBUG("HTTP method: %s\n", methods[i].name);
			request->method = methods[i].method;
			break;
		}
	}

	if (i == num_methods) {
		return -1;
	}

	/*======= PATH =======*/
	token = strtok(NULL, delim);
	if (token == NULL) {
		return -1;
	}

	DEBUG("HTTP path: %s\n", token);	
	strncpy((char *) request->path, token, MAX_PATH_LEN);

	/*======= PAYLOAD =======*/
	for (i = 0; i < buffer_size; i++) {
		if (_CHECK_ZERO4((&buffer[i]))) {
			if (_CMP4((&buffer[i]), 0x0d, 0x0a, 0x0d, 0x0a)) {
				i += 4;
				break;
			}
		}
	}

	DEBUG("=== I: %lu/%lu\n", i, buffer_size);
	request->payload = malloc(buffer_size - i + 1);
	if (UNLIKELY(request->payload == NULL)) {
		perror(ERR "parse_request():malloc()");
		return -1;
	}
	memcpy(request->payload, &buffer[i], buffer_size - i);
	request->payload[buffer_size - i] = '\0';

	DEBUG("HTTP payload: \"%s\"\n", (char *) &buffer[i]);
	return 0;
}

void handle_request(int client_socket)
{
	HTTP_request request;
	HTTP_response response;
	uint8_t buffer[MAX_REQ_SIZE];
	size_t buffer_size;
	int error;

	memset(&request, 0, sizeof(request));
	memset(&response, 0, sizeof(response));
	memset(buffer, 0, MAX_REQ_SIZE);

	buffer_size = recv_data(client_socket, buffer, &error);
	if (UNLIKELY(error)) {
		fprintf(stderr, ERR "recv'ing data failed.\n");
		return ;
	}

	DEBUG("recv'd buffer:\n%s", buffer);

	if (UNLIKELY(parse_request(&request, buffer, (size_t) buffer_size) < 0)) {
		fprintf(stderr, ERR "parsing request failed.\n");
		return ;
	}

	DEBUG("parsed request.\n");

	if (UNLIKELY(request_callback(request, &response) < 0)) {
		fprintf(stderr, ERR "request callback failed.\n");
		return ;
	}

	DEBUG("called request callback.\n");

	if (UNLIKELY(send_data(client_socket, response) < 0)) {
		fprintf(stderr, ERR "failed to send response.\n");
		return ;
	}

	DEBUG("sent data to client.\n");

	free(request.payload);
	free(response.buffer);
}
