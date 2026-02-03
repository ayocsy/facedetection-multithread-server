#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

// The prefix and size of the data bytes
#define PROTOCOL_PREFIX 0x23107231
#define PREFIX_BYTES 4
#define OPERATION_BYTES 1
#define IMAGE_BYTES 4

// The temporary image path
#define TEMP_IMAGE_PATH "/tmp/imagefile.jpg"

// ALL the operation type
#define REQUEST_DETECT 0
#define REQUEST_REPLACE 1
#define REQUEST_OUTPUT 2
#define ERROR_MESSAGE 3

// the initial size for reading
#define INITIAL_SIZE 1024

// Response File
#define RESPONSE_FILE "/local/courses/csse2310/resources/a4/responsefile"

void protocol_pack_request(uint8_t operation, const uint8_t* image1,
        uint32_t image1Size, const uint8_t* image2, uint32_t image2Size,
        uint8_t** resultBuffer, size_t* resultSize);

void send_responsefile(int fd);
void send_error(int fd, const char* message);
uint8_t* read_file_to_buffer(char* filename, uint32_t* imageSize);
void send_client(int fd);
