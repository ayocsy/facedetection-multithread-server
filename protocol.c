#include "protocol.h"

/**
 * read_file_to_buffer
 * -------------------
 * Reads a binary file into memory and returns a pointer to the data.
 *
 * filename: The name of the file to read
 * imageSize: Pointer to a uint32_t that will be set to the number of bytes read
 *
 * Returns: Pointer to malloc'd buffer containing binary data, or NULL on error.
 *          Caller is responsible for freeing the buffer.
 *
 * REF: The code of getting the file size is inspired by
 * REF:
 * https://stackoverflow.com/questions/238603/how-can-i-get-a-files-size-in-c
 */
uint8_t* read_file_to_buffer(char* filename, uint32_t* imageSize)
{
    FILE* file = fopen(filename, "rb"); // Read in binary
    fseek(file, 0, SEEK_END); // Move to the end of the file
    size_t size = ftell(file); // Get the current file pointer
    fseek(file, 0, SEEK_SET); // Move back to the top

    uint8_t* buffer = malloc(size); // Allocate memory on buffer
    size_t read = fread(buffer, 1, size, file); // Read the data into buffer
    if (read != (size_t)size) {
        // Read all fail
        free(buffer);
        return NULL;
    }
    fclose(file);
    *imageSize = (uint32_t)size; // store the size
    return buffer;
}

/*
 * protocol_pack_request
 * ---------------------
 * Builds a binary protocol request message containing one or two images
 * and an operation code.
 * The packed result is allocated and returned via ResultBuffer and ResultSize.
 */
void protocol_pack_request(uint8_t operation, const uint8_t* image1,
        uint32_t image1Size, const uint8_t* image2, uint32_t image2Size,
        uint8_t** resultBuffer, size_t* resultSize)
{
    // Get the total Size first
    size_t totalSize
            = IMAGE_BYTES + OPERATION_BYTES + PREFIX_BYTES + image1Size;
    if (operation == REQUEST_REPLACE) {
        // Add the extra size space if it's replace operation type
        totalSize += (IMAGE_BYTES + image2Size);
    }
    uint8_t* buffer = malloc(totalSize);
    size_t index = 0;
    uint32_t prefix = PROTOCOL_PREFIX;
    // Write the protocol prefix
    memcpy(buffer, &prefix, PREFIX_BYTES);
    index += PREFIX_BYTES; // Move pointer
    // Write the opertaion type
    buffer[index++] = operation;
    // Write the image Size
    memcpy(buffer + index, &image1Size, IMAGE_BYTES);
    index += IMAGE_BYTES;
    // Write the image1 data
    memcpy(buffer + index, image1, image1Size);
    index += image1Size;

    if (operation == REQUEST_REPLACE && image2) {
        // Write the image2 size
        // For the replace operation only
        memcpy(buffer + index, &image2Size, IMAGE_BYTES);
        index += IMAGE_BYTES;
        // Write the image2 data
        memcpy(buffer + index, image2, image2Size);
        index += image2Size;
    }
    *resultBuffer = buffer; // Store the final data
    *resultSize = totalSize; // Store the memories size
}

/*
 * send_responsefile
 * -----------------
 * Reads the contents of the RESPONSE_FILE and sends it directly to the given
 * socket. Used to respond to wrong prefix protocol requests.
 */
void send_responsefile(int fd)
{
    int responseFile = open(RESPONSE_FILE, O_RDONLY);
    char buffer[INITIAL_SIZE];
    ssize_t readBytes;
    while ((readBytes = read(responseFile, buffer, sizeof(buffer))) > 0) {
        // read and get the data from the file until EOF
        write(fd, buffer, readBytes); // Send to the client
    }
    close(responseFile);
}

/*
 * send_error
 * ----------
 * Sends a protocol-compliant error message with the given text to the client.
 * The message is packed using the standard protocol format.
 */
void send_error(int fd, const char* message)
{
    uint8_t* resultBuffer = NULL;
    size_t resultSize = 0;
    uint8_t operation = ERROR_MESSAGE; // 3
    uint32_t length = strlen(message) * sizeof(char);

    // Pack the message and operation detial
    protocol_pack_request(operation, (uint8_t*)message, length, NULL, 0,
            &resultBuffer, &resultSize);
    // Send to the client
    write(fd, resultBuffer, resultSize);
    free(resultBuffer);
}

/*
 * send_client
 * -----------
 * Reads the processed image from a temporary file and sends it to the client.
 * Packs the image using the protocol with the REQUEST_OUTPUT operation.
 */
void send_client(int fd)
{
    uint8_t* resultBuffer = NULL;
    size_t resultSize = 0;
    uint8_t* image1;
    uint32_t image1Size;

    // Get the image data from TEM_IMAGE_PATH
    image1 = read_file_to_buffer(TEMP_IMAGE_PATH, &image1Size);
    uint8_t operation = REQUEST_OUTPUT;

    // Pack the message and operation detia
    protocol_pack_request(
            operation, image1, image1Size, NULL, 0, &resultBuffer, &resultSize);

    write(fd, resultBuffer, resultSize); // Send to the client
    free(resultBuffer);
    free(image1);
}
