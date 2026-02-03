#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <opencv2/imgcodecs/imgcodecs_c.h>
#include <opencv2/imgproc/imgproc_c.h>
#include <opencv2/objdetect/objdetect_c.h>
#include "protocol.h"

#define MAX_CLIENTS 10000

// Base for converting char* to long
const int baseTen = 10;

// Max and Min argc insert
const int minArgsCount = 3;
const int maxArgsCount = 4;

// Arugment index
const int clientLimitIndex = 1;
const int maxSizeIndex = 2;
const int portIndex = 3;

// empty strinf and number bytes of the data
const char* const emptyString = "";
const int dataBytes = 4;

// Error message that sned to client
const char* const usageErrorMessage
        = "Usage: ./uqfacedetect clientlimit maxsize [portnumber]\n";
const char* const fileErrorMessage
        = "uqfacedetect: unable to open the image file for writing\n";
const char* const cascadeErrorMessage
        = "uqfacedetect: cannot load a cascade classifier\n";
const char* const operationErrorMessage = "invalid operation type";
const char* const imageErrorMessage = "image is 0 bytes";
const char* const bigImageErrorMessage = "image too large";
const char* const invalidErrorMessage = "invalid message";
const char* const imageInvalidErrorMessage = "invalid image";
const char* const noFaceErrorMessage = "no faces detected in image";
const char* const portErrorMessage
        = "uqfacedetect: cannot listen on given port \"%s\"\n";

// the cascade file name for OpenCv
const char* const faceCascadeFilename = "/local/courses/csse2310/resources/a4/"
                                        "haarcascade_frontalface_alt2.xml";
const char* const eyesCascadeFilename = "/local/courses/csse2310/resources/a4/"
                                        "haarcascade_eye_tree_eyeglasses.xml";

// OpenCV parameters
const float haarScaleFactor = 1.1;
const int haarMinNeighbours = 4;
const int haarFlags = 0;
const int haarMinSize = 0;
const int haarMaxSize = 1000;
const int ellipseStartAngle = 0;
const int ellipseEndAngle = 360;
const int lineThickness = 4;
const int lineType = 8;
const int shift = 0;
const int bgraChannels = 4;
const int alphaIndex = 3;

// The Argument of the program
typedef struct {
    int clientLimit;
    uint32_t maxSize;
    char* port;
    int sockfd;
    sem_t clientSlot;
    CvHaarClassifierCascade* faceCascade;
    CvHaarClassifierCascade* eyesCascade;
} Arguments;

// The info of the client
typedef struct {
    int clientfd;
    uint32_t maxSize;
    pthread_mutex_t lock;
    sem_t clientSlot;
    CvHaarClassifierCascade* faceCascade;
    CvHaarClassifierCascade* eyesCascade;
} ClientInfo;

// This enum contains the program exit status codes
typedef enum {
    EXIT_OK_STATUS = 0,
    EXIT_USAGE_STATUS = 12,
    EXIT_FILE_STATUS = 20,
    EXIT_CASCADE_STATUS = 9,
    EXIT_PORT_STATUS = 14
} ExitStatus;
