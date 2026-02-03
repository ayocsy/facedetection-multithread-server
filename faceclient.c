#include "uqfaceclient.h"

/* init_arguments_struct()
 * ---------------
 * Initialise an Arguments struct with default values.
 *
 * Returns: pointer to the malloc'd struct
 * Global variables modified: none
 * Errors: none
 *
 * REF: a1 solution
 */
Arguments* init_arguments_struct(void)
{
    Arguments* args = malloc(sizeof(Arguments));
    args->port = 0;
    args->detectFileName = NULL;
    args->outputFileName = NULL;
    args->replaceFileName = NULL;
    args->errorMessage = NULL;
    args->sockfd = 0;
    return args;
}

/**
 * setup_sigpipe_handler()
 * -----------------------
 * Blocking the signal SiGPIPE
 *
 * Ref: Week 6 lecture example (sig.c)
 */
void setup_sigpipe_handler()
{
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    ;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
}

/* cleanup_and_exit()
 * ---------------
 * Print a message to either stdout or stderr, free vars and args structs
 * they exist, and exit with the given exit status.
 *
 * args: pointer to an Arguments struct containing command line information
 * exitStatus: exit status of the program
 *
 * Returns: nothing
 * Global variables modified: none
 * Errors: print the appropriate message and exit with the appropriate
 * exit status
 *
 * REF: a1 solution
 */
void cleanup_and_exit(Arguments* args, int exitStatus)
{
    if (exitStatus == EXIT_USAGE_STATUS) {
        fprintf(stderr, "%s", usageErrorMessage);
    } else if (exitStatus == EXIT_INPUTFILE_STATUS) {
        if (args->detectFileName) {
            fprintf(stderr, inputFileErrorMessageFormat, args->detectFileName);
        } else if (args->replaceFileName) {
            fprintf(stderr, inputFileErrorMessageFormat, args->replaceFileName);
        }
    } else if (exitStatus == EXIT_OUTPUTFILE_STATUS) {
        fprintf(stderr, outputFileErrorMessageFormat, args->outputFileName);
    } else if (exitStatus == EXIT_PORT_STATUS) {
        fprintf(stderr, connectionErrorMessageFormat, args->port);
    } else if (exitStatus == EXIT_COMMUNICATE_STATUS) {
        fprintf(stderr, communicationErrorMessage);
    } else if (exitStatus == EXIT_ERRORMESSAGE_STATUS) {
        fprintf(stderr, serverErrorMessageFormat, args->errorMessage);
    }

    if (args) {
        // free variable
        if (args->detectFileName) {
            free(args->detectFileName);
        }
        if (args->outputFileName) {
            free(args->outputFileName);
        }
        if (args->replaceFileName) {
            free(args->replaceFileName);
        }
        if (args->errorMessage) {
            free(args->errorMessage);
        }
        if (args->sockfd) {
            close(args->sockfd);
        }
        free(args);
    }
    // exit the status
    exit(exitStatus);
}

/*
 * Check if the arguement is empty, otherwise return argument
 */
char* check_emptystring(char* argument, Arguments* args)
{
    if (strcmp(argument, emptyString) == 0) {
        cleanup_and_exit(args, EXIT_USAGE_STATUS);
    }
    return argument;
}

/* check_file()
 * --------------------------
 * check of the file can read or written
 *
 * fname: the filename of the file
 * args: the arguments struct
 */
void check_file(char* fileName, int fileType, Arguments* args)
{
    FILE* file;
    if (fileType == detectFileType) {
        file = fopen(fileName, "rb"); // Input image file
        if (!file) {
            cleanup_and_exit(args, EXIT_INPUTFILE_STATUS);
        }
    } else if (fileType == outputFileType) {
        file = fopen(fileName, "wb"); // Output file
        if (!file) {
            cleanup_and_exit(args, EXIT_OUTPUTFILE_STATUS);
        }
    }
    fclose(file);
}

/*
 * connect_to_server
 * -----------------
 * Tries to connect to the server at localhost on the given port string.
 *
 * port: A string representing a port number or service name
 * args: the arguments struct
 * REF: net2.c from Lec note week9
 */
void connect_to_server(char* port, Arguments* args)
{
    struct addrinfo* ai;
    struct addrinfo hints;
    // Clear the hints structure
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP
    int err;
    if ((err = getaddrinfo("localhost", port, &hints, &ai))) {
        // check if the addres work out
        cleanup_and_exit(args, EXIT_PORT_STATUS);
    }
    args->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    // 0 == use default protocol
    if (args->sockfd < 0) {
        // Socket create fail
        freeaddrinfo(ai);
        cleanup_and_exit(args, EXIT_PORT_STATUS);
    }
    if (connect(args->sockfd, ai->ai_addr, ai->ai_addrlen) != 0) {
        // Connect fail
        freeaddrinfo(ai);
        close(args->sockfd);
        cleanup_and_exit(args, EXIT_PORT_STATUS);
    }
    freeaddrinfo(ai);
}

/*
 * read_stdin_to_buffer
 * --------------------
 * Reads binary data from standard input until EOF and stores it in a
 * dynamically allocated buffer. The size of the data read is stored in
 * imageSize.
 */
uint8_t* read_stdin_to_buffer(uint32_t* imageSize)
{
    size_t size = 0;
    size_t space = initialSize;
    uint8_t* buffer = malloc(space);

    int byte;
    while ((byte = fgetc(stdin)) != EOF) {
        if (size >= space) {
            // check if running out of space
            space *= 2; // Double the space for storing the data
            buffer = realloc(buffer, space);
        }
        buffer[size++] = (uint8_t)byte; // Store the data into buffer
    }
    *imageSize = (uint32_t)size; // Get the size
    return buffer;
}

/*
 * Constructs and sends a face detection or replacement request to the server.
 * Reads input images from file or stdin, packs them using the protocol, and
 * sends the resulting buffer through the socket specified in args.
 */
void build_image_process(Arguments* args)
{
    int operation = (args->replaceFileName) ? REQUEST_REPLACE : REQUEST_DETECT;
    // get the operation type
    uint32_t image1Size = 0, image2Size = 0;
    uint8_t* image1 = NULL;
    uint8_t* image2 = NULL;
    if (args->detectFileName) {
        // from the argument
        image1 = read_file_to_buffer(args->detectFileName, &image1Size);
    } else {
        // from the stdin
        image1 = read_stdin_to_buffer(&image1Size);
    }
    if (operation && args->replaceFileName) {
        // replace file
        image2 = read_file_to_buffer(args->replaceFileName, &image2Size);
    }

    uint8_t* finalBuffer = NULL;
    size_t finalSize = 0;
    // Pack the image and opration detail
    protocol_pack_request(operation, image1, image1Size, image2, image2Size,
            &finalBuffer, &finalSize);
    free(image1);
    if (image2) {
        free(image2);
    }
    // Send to the server
    ssize_t send = write(args->sockfd, finalBuffer, finalSize);
    if (send < 0) {
        // Sending fail
        free(finalBuffer);
        cleanup_and_exit(args, EXIT_COMMUNICATE_STATUS);
    }
    free(finalBuffer);
}

/*
 * parse_arguments
 * ----------------
 * Parses and validates command-line arguments for uqfaceclient.
 * Stores values in an Arguments struct and checks file access.
 * Exits with appropriate status on usage, file, or port errors.
 * Returns: pointer to populated Arguments struct.
 */
Arguments* parse_arguments(int argc, char** argv)
{
    Arguments* args = init_arguments_struct();
    int i = 1; // Start after program name
    while (i < argc) {
        if (i == 1 && (bool)check_emptystring(argv[i], args)) {
            args->port = argv[1]; // Port must be the first index
        } else if (strcmp(argv[i], detectFileArg) == 0) { // --detfile
            if (args->detectFileName || ++i >= argc) {
                cleanup_and_exit(args, EXIT_USAGE_STATUS);
            }
            args->detectFileName = strdup(check_emptystring(argv[i], args));
        } else if (strcmp(argv[i], outputFileArg) == 0) { // --output
            if (args->outputFileName || (++i >= argc)) {
                cleanup_and_exit(args, EXIT_USAGE_STATUS);
            }
            args->outputFileName = strdup(check_emptystring(argv[i], args));
        } else if (strcmp(argv[i], replaceFileArg) == 0) { // --replace
            if (args->replaceFileName || (++i >= argc)) {
                cleanup_and_exit(args, EXIT_USAGE_STATUS);
            }
            args->replaceFileName = strdup(check_emptystring(argv[i], args));
        } else {
            cleanup_and_exit(args, EXIT_USAGE_STATUS);
        }
        i++; // Incremnet index
    }
    if (!args->port) {
        // No port given
        cleanup_and_exit(args, EXIT_USAGE_STATUS);
    }
    if (args->detectFileName) {
        // Check if it's readable
        check_file(args->detectFileName, detectFileType, args);
    }
    if (args->replaceFileName) {
        // Check if it's readable
        check_file(args->replaceFileName, detectFileType, args);
    }
    if (args->outputFileName) {
        // Check if it can be written
        check_file(args->outputFileName, outputFileType, args);
    }
    connect_to_server(args->port, args);
    return args;
}

void handle_response(Arguments* args)
{
    uint32_t prefix;
    uint8_t operation;
    uint32_t size;

    if ((read(args->sockfd, &prefix, PREFIX_BYTES) != PREFIX_BYTES)
            || prefix != PROTOCOL_PREFIX) {
        // check the prefix
        cleanup_and_exit(args, EXIT_COMMUNICATE_STATUS);
    }
    if (read(args->sockfd, &operation, OPERATION_BYTES) != 1) {
        // Read the operation type
        cleanup_and_exit(args, EXIT_COMMUNICATE_STATUS);
    }
    if (read(args->sockfd, &size, dataBytes) != dataBytes) {
        // Read the data size
        cleanup_and_exit(args, EXIT_COMMUNICATE_STATUS);
    }
    uint8_t* result = malloc(size + 1);
    if (!result || read(args->sockfd, result, size) != size) {
        // Read the data
        free(result);
        cleanup_and_exit(args, EXIT_COMMUNICATE_STATUS);
    }
    if (operation == REQUEST_OUTPUT) { // Output image
        FILE* output = args->outputFileName ? fopen(args->outputFileName, "wb")
                                            : stdout;
        // Get the output file
        fwrite(result, 1, size, output);
        // Write the data intot the output file
        if (output != stdout) {
            // Prevent close the stdout
            fclose(output);
        }
        free(result);
    } else if (operation == ERROR_MESSAGE) { // Error message
        args->errorMessage = strdup((char*)result);
        free(result);
        cleanup_and_exit(args, EXIT_ERRORMESSAGE_STATUS);
    } else {
        // Other unknow operation type
        free(result);
        cleanup_and_exit(args, EXIT_COMMUNICATE_STATUS);
    }
}

/**
 * main()
 * -------
 * Entry point for uqparallel. Parses command-line arguments,
 * processes input tasks, and exits with the appropriate status.
 *
 * argc: number of command-line arguments
 * argv: array of command-line arguments
 *
 * Returns: program exit status (0 on success, or error code)
 */
int main(int argc, char** argv)
{
    setup_sigpipe_handler();
    Arguments* args = parse_arguments(argc, argv);
    build_image_process(args);
    handle_response(args);
    cleanup_and_exit(args, 0);
}
