#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <stdbool.h>
#include <netdb.h>
#include "protocol.h"

// Argument insert flag
const char* const optionArgStart = "--";
const char* const detectFileArg = "--detectfilename";
const char* const outputFileArg = "--outputfilename";
const char* const replaceFileArg = "--replacefile";
const char* const emptyString = "";

// File type for file check easier
const int detectFileType = 69;
const int outputFileType = 99;

// The initial size for reading
const size_t initialSize = 1024;
const int dataBytes = 4;

// Error Message
const char* const usageErrorMessage
        = "Usage: ./uqfaceclient port [--detectfilename filename]"
          " [--outputfilename filename] [--replacefile filename]\n";

const char* const inputFileErrorMessageFormat
        = "uqfaceclient: cannot open the input file \"%s\" for reading\n";

const char* const outputFileErrorMessageFormat
        = "uqfaceclient: unable to open the output file \"%s\" for writing\n";

const char* const connectionErrorMessageFormat
        = "uqfaceclient: cannot connect to the server on port \"%s\"\n";

const char* const communicationErrorMessage
        = "uqfaceclient: unexpected communication error\n";

const char* const serverErrorMessageFormat
        = "uqfaceclient: received the following error message: \"%s\"\n";

// The Argument of the program
typedef struct {
    char* port;
    char* detectFileName;
    char* outputFileName;
    char* replaceFileName;
    char* errorMessage;
    int sockfd;
} Arguments;

// This enum contains the program exit status codes
typedef enum {
    EXIT_OK_STATUS = 0,
    EXIT_USAGE_STATUS = 17,
    EXIT_ERRORMESSAGE_STATUS = 12,
    EXIT_OUTPUTFILE_STATUS = 9,
    EXIT_INPUTFILE_STATUS = 20,
    EXIT_PORT_STATUS = 4,
    EXIT_COMMUNICATE_STATUS = 18
} ExitStatus;
