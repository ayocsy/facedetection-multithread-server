#include "uqfacedetect.h"

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
    args->clientLimit = 0;
    args->maxSize = 0;
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
        fprintf(stderr, usageErrorMessage);
    } else if (exitStatus == EXIT_FILE_STATUS) {
        fprintf(stderr, fileErrorMessage);
    } else if (exitStatus == EXIT_CASCADE_STATUS) {
        fprintf(stderr, cascadeErrorMessage);
    } else if (exitStatus == EXIT_PORT_STATUS) {
        fprintf(stderr, portErrorMessage, args->port);
    }
    if (args) {
        if (args->port) {
            free(args->port);
        }
        if (args->faceCascade) {
            cvReleaseHaarClassifierCascade(&args->faceCascade);
        }
        if (args->eyesCascade) {
            cvReleaseHaarClassifierCascade(&args->eyesCascade);
        }
        free(args);
    }
    exit(exitStatus);
}

/*
 * check_cascade
 * -------------
 * Store  cascade classifiers for face and eye detection into the args struct.
 * Exits the program if either classifier cannot be loaded.
 *
 * REF: Example 2 from the A4 spec sheet.
 */
void check_cascade(Arguments* args)
{
    // Load the cascade
    args->faceCascade = (CvHaarClassifierCascade*)cvLoad(
            faceCascadeFilename, NULL, NULL, NULL);
    args->eyesCascade = (CvHaarClassifierCascade*)cvLoad(
            eyesCascadeFilename, NULL, NULL, NULL);
    if (!args->faceCascade || !args->eyesCascade) {
        cleanup_and_exit(args, EXIT_CASCADE_STATUS);
    }
}

/*
 * cleanup_opencv_resources
 * ------------------------
 * Frees all allocated OpenCV resources to avoid memory leaks.
 */
void cleanup_opencv_resources(IplImage* frame, IplImage* frameGray,
        CvHaarClassifierCascade* faceCascade,
        CvHaarClassifierCascade* eyesCascade, CvMemStorage* storage,
        IplImage* replace)
{
    if (frame) {
        cvReleaseImage(&frame);
    }
    if (frameGray) {
        cvReleaseImage(&frameGray);
    }
    if (faceCascade) {
        // cvReleaseHaarClassifierCascade(&faceCascade);
    }
    if (eyesCascade) {
        // cvReleaseHaarClassifierCascade(&eyesCascade);
    }
    if (storage) {
        cvReleaseMemStorage(&storage);
    }
    if (replace) {
        cvReleaseImage(&replace);
    }
}

/*
 * cleanup_roi_resources
 * ------------------------
 * Frees all allocated roi resources to avoid memory leaks.
 */
void cleanup_roi_resources(IplImage* faceROI, CvMemStorage* eyeStorage)
{
    if (faceROI) {
        cvReleaseImage(&faceROI);
    }
    if (eyeStorage) {
        cvReleaseMemStorage(&eyeStorage);
    }
}

/*
 * Helper function for replace_face() draw the replace face
 */
void draw_ellipses_and_eyes(IplImage* frame, IplImage* frameGray, CvRect* face,
        CvHaarClassifierCascade* eyesCascade)
{
    CvPoint center = {face->x + face->width / 2, face->y + face->height / 2};
    const CvScalar magenta = cvScalar(255, 0, 255, 0);
    const CvScalar blue = cvScalar(255, 0, 0, 0);

    cvEllipse(frame, center, cvSize(face->width / 2, face->height / 2), 0,
            ellipseStartAngle, ellipseEndAngle, magenta, lineThickness,
            lineType, shift);

    IplImage* faceROI = cvCreateImage(cvGetSize(frameGray), IPL_DEPTH_8U, 1);
    cvCopy(frameGray, faceROI, NULL);
    cvSetImageROI(faceROI, *face);

    CvMemStorage* eyeStorage = cvCreateMemStorage(0);
    cvClearMemStorage(eyeStorage);

    CvSeq* eyes = cvHaarDetectObjects(faceROI, eyesCascade, eyeStorage,
            haarScaleFactor, haarMinNeighbours, haarFlags,
            cvSize(haarMinSize, haarMinSize), cvSize(haarMaxSize, haarMaxSize));

    if (eyes->total == 2) {
        for (int j = 0; j < eyes->total; j++) {
            CvRect* eye = (CvRect*)cvGetSeqElem(eyes, j);
            CvPoint eyeCenter = {face->x + eye->x + eye->width / 2,
                    face->y + eye->y + eye->height / 2};
            int radius = cvRound((eye->width / 2 + eye->height / 2) / 2);
            cvCircle(frame, eyeCenter, radius, blue, lineThickness, lineType,
                    shift);
        }
    }

    cleanup_roi_resources(faceROI, eyeStorage);
}

/*
 * detect_faces
 * ------------
 * Detects faces and eyes in the given input image using cascade classifiers,
 * draws ellipses around them, and saves the annotated image to the output file.
 * Returns 0 on success, 1 if no faces found, or -1 on image load failure.
 *
 * REF: Example 2 from a4 spec
 */
int detect_faces(char* inputfile, char* outputfile,
        CvHaarClassifierCascade* faceCascade,
        CvHaarClassifierCascade* eyesCascade)
{
    IplImage* frame = cvLoadImage(inputfile, CV_LOAD_IMAGE_COLOR);
    if (!frame) {
        return -1;
    }
    IplImage* frameGray = cvCreateImage(cvGetSize(frame), IPL_DEPTH_8U, 1);
    cvCvtColor(frame, frameGray, CV_BGR2GRAY);
    cvEqualizeHist(frameGray, frameGray);
    CvMemStorage* storage = 0;
    storage = cvCreateMemStorage(0);
    cvClearMemStorage(storage);
    CvSeq* faces = cvHaarDetectObjects(frameGray, faceCascade, storage,
            haarScaleFactor, haarMinNeighbours, haarFlags,
            cvSize(haarMinSize, haarMinSize), cvSize(haarMaxSize, haarMaxSize));
    if (faces->total == 0) {
        cleanup_opencv_resources(
                frame, frameGray, faceCascade, eyesCascade, storage, NULL);
        return 1;
    }
    for (int i = 0; i < faces->total; i++) {
        CvRect* face = (CvRect*)cvGetSeqElem(faces, i);
        draw_ellipses_and_eyes(frame, frameGray, face, eyesCascade);
    }
    cvSaveImage(outputfile, frame, 0);
    cleanup_opencv_resources(
            frame, frameGray, faceCascade, eyesCascade, storage, NULL);
    return 0;
}

/*
 * Helper function for replace_face() draw the replace face
 */
void draw_replace_on_face(IplImage* frame, IplImage* replace, CvRect* face)
{
    IplImage* resized = cvCreateImage(cvSize(face->width, face->height),
            IPL_DEPTH_8U, replace->nChannels);
    cvResize(replace, resized, CV_INTER_AREA);

    char* frameData = frame->imageData;
    char* faceData = resized->imageData;

    for (int y = 0; y < face->height; y++) {
        for (int x = 0; x < face->width; x++) {
            int faceIndex = resized->widthStep * y + x * resized->nChannels;
            if (resized->nChannels == bgraChannels
                    && faceData[faceIndex + alphaIndex] == 0) {
                continue;
            }
            int frameIndex = frame->widthStep * (face->y + y)
                    + (face->x + x) * frame->nChannels;
            frameData[frameIndex + 0] = faceData[faceIndex + 0];
            frameData[frameIndex + 1] = faceData[faceIndex + 1];
            frameData[frameIndex + 2] = faceData[faceIndex + 2];
        }
    }

    cvReleaseImage(&resized);
}

/*
 * replace_faces
 * -------------
 * Detects faces in the input frame and replaces each one with the given
 * replacement image. Saves the modified image to the specified output file and
 * returns 0 on success, 1 if no faces were found, or -1 if the input images are
 * invalid.
 *
 * REF: Example 3 from a4 spec
 */
int replace_faces(IplImage* frame, IplImage* replace, char* outputFilename,
        CvHaarClassifierCascade* faceCascade)
{
    if (!frame) {
        cvReleaseHaarClassifierCascade(&faceCascade);
        return -1;
    }
    if (!replace) {
        cvReleaseImage(&frame);
        cvReleaseHaarClassifierCascade(&faceCascade);
        return -1;
    }
    IplImage* frameGray = cvCreateImage(cvGetSize(frame), IPL_DEPTH_8U, 1);
    cvCvtColor(frame, frameGray, CV_BGR2GRAY);
    cvEqualizeHist(frameGray, frameGray);
    CvMemStorage* storage = cvCreateMemStorage(0);
    cvClearMemStorage(storage);
    CvSeq* faces = cvHaarDetectObjects(frameGray, faceCascade, storage,
            haarScaleFactor, haarMinNeighbours, haarFlags,
            cvSize(haarMinSize, haarMinSize), cvSize(haarMaxSize, haarMaxSize));
    if (faces->total == 0) {
        cleanup_opencv_resources(
                frame, frameGray, faceCascade, NULL, storage, replace);
        return 1;
    }
    for (int i = 0; i < faces->total; i++) {
        CvRect* face = (CvRect*)cvGetSeqElem(faces, i);
        draw_replace_on_face(frame, replace, face);
    }
    cvSaveImage(outputFilename, frame, 0);
    cleanup_opencv_resources(
            frame, frameGray, faceCascade, NULL, storage, replace);
    return 0;
}

/*
 * check the temp file path can be written
 */
void check_image_file(Arguments* args)
{
    FILE* file = fopen(TEMP_IMAGE_PATH, "wb"); // open and check
    if (!file) {
        cleanup_and_exit(args, EXIT_FILE_STATUS);
    }
    fclose(file);
}

/*
 * Check if the arguement is empty, otherwise exit with usage error
 */
void check_emptystring(char* argument, Arguments* args)
{
    if (strcmp(argument, emptyString) == 0) {
        cleanup_and_exit(args, EXIT_USAGE_STATUS);
    }
}

/*
 * Parses and validates the client limit argument as a non-negative integer
 * less than or equal to MAX_CLIENTS. Exits with usage status on invalid input.
 */
int check_clientlimit(char* clientLimit, Arguments* args)
{
    check_emptystring(clientLimit, args); // Check if it's empty string
    char* ptr;
    // Convert to value
    long value = strtol(clientLimit, &ptr, baseTen);

    if (*ptr != '\0' || value < 0 || value > MAX_CLIENTS) {
        // Check if the argument is value and
        // if it' non negative or low than the fixed limit
        cleanup_and_exit(args, EXIT_USAGE_STATUS);
    }
    return (int)value;
}

/*
 * Parses and validates the max image size argument as a non-negative integer
 * less than or equal to UINT32_MAX. Returns UINT32_MAX if the input is zero.
 */
uint32_t check_maxsize(char* maxSize, Arguments* args)
{
    check_emptystring(maxSize, args); // Check if it's empty string
    char* ptr;
    // Convert to value
    long value = strtoul(maxSize, &ptr, baseTen);
    if (*ptr != '\0' || value < 0 || value > UINT32_MAX) {
        // Check if the argument is non negetive and lower than UINT32_t
        // Also check if it's a value
        cleanup_and_exit(args, EXIT_USAGE_STATUS);
    }
    if (value == 0) {
        // value be the max uint32_t if it's 0
        return UINT32_MAX;
    }
    return (uint32_t)value;
}

/*
 * read_exact_bytes
 * ----------------
 * Attempts to read the bytes from the given file descriptor into buffer
 * Prevent the case like unable to read the non one time sent
 * data bytes from the client.
 * Returns the number of bytes read, or -1 if EOF or an error occurs before
 * completion.
 */
ssize_t read_exact_bytes(int fd, void* buffer, size_t size)
{
    size_t readed = 0;
    while (readed < size) {
        // read until read the full size
        ssize_t readBytes = read(fd, (char*)buffer + readed, size - readed);
        // read the data bytes or the remain bytes from the path
        if (readBytes == 0 || readBytes < 0) {
            // Error or EOF
            return -1;
        }
        readed += readBytes; // increment the bytes read
    }
    return readed; // the total bytes read, should be equal to size
}

/*
 * read_prefix
 * -----------
 * Reads and validates the protocol prefix from the client.
 * If invalid or unreadable, sends the response file and closes the connection.
 * Returns true on success, false otherwise.
 */
bool read_prefix(int fd)
{
    uint32_t prefix;
    if (read_exact_bytes(fd, &prefix, sizeof(prefix)) != sizeof(prefix)) {
        // Not correct format
        send_error(fd, invalidErrorMessage);
        close(fd);
        return false;
    }
    if (prefix != PROTOCOL_PREFIX) {
        // pefix is not correct
        send_responsefile(fd); // Send the response file to client
        close(fd);
        return false;
    }
    return true;
}

/*
 * read_operation
 * --------------
 * Reads a single-byte operation type from the client and checks its validity
 * Sends an error message and closes the connection if invalid or unreadable.
 * Returns true on success, false otherwise.
 */
bool read_operation(int fd, uint8_t* operation)
{
    if (read(fd, operation, 1) != 1) {
        // Not correct format
        send_error(fd, invalidErrorMessage);
        close(fd);
        return false;
    }
    if (*operation != 0 && *operation != 1) {
        // wrong operation type
        send_error(fd, operationErrorMessage);
        close(fd);
        return false;
    }
    return true;
}

/*
 * read_image
 * ----------
 * Reads an image from the client over the given socket.
 * Validates the image size, ensures it's within the maximum allowed
 * Sends appropriate error messages and closes the connection if the image is
 * invalid. Returns true on success, false on failure.
 */
bool read_image(int fd, uint32_t* size, uint32_t maxSize, uint8_t** image)
{
    if (read_exact_bytes(fd, size, sizeof(uint32_t)) != sizeof(uint32_t)) {
        // wrong format for size
        send_error(fd, invalidErrorMessage);
        close(fd);
        return false;
    }
    if (*size == 0) {
        // the image has no bytes
        send_error(fd, imageErrorMessage);
        close(fd);
        return false;
    }
    if (*size > maxSize) {
        // image is larger than the fixed limit
        send_error(fd, bigImageErrorMessage);
        close(fd);
        return false;
    }
    *image = malloc(*size);
    if (!*image || read_exact_bytes(fd, *image, *size) != *size) {
        // Worng format for image
        send_error(fd, invalidErrorMessage);
        free(*image);
        close(fd);
        return false;
    }
    return true;
}

/*
 * save_and_detect_image
 * ---------------------
 * Saves the received image to disk and runs face and eye detection using
 * OpenCV. Returns the detection result via detectResult and sends error
 * messages to the client if detection fails. Ensures exclusive access to the
 * temporary image file using a mutex lock.
 */
bool save_and_detect_image(
        ClientInfo* clt, uint8_t* image, uint32_t imageSize, int* detectResult)
{
    bool error;
    // Avoid race condition
    pthread_mutex_lock(&clt->lock);
    FILE* file = fopen(TEMP_IMAGE_PATH, "wb");
    fwrite(image, 1, imageSize, file); // write the image data into temp file
    fclose(file);
    // Check the detect result
    *detectResult = detect_faces(TEMP_IMAGE_PATH, TEMP_IMAGE_PATH,
            clt->faceCascade, clt->eyesCascade);
    pthread_mutex_unlock(&clt->lock);
    if (*detectResult == -1) {
        // unable to read the file
        send_error(clt->clientfd, imageInvalidErrorMessage);
        error = true;
    } else if (*detectResult == 1) {
        // No face detect
        send_error(clt->clientfd, noFaceErrorMessage);
        error = true;
    }
    if (error) {
        return false;
    }
    return true;
}

/*
 * save_and_replace_image
 * ----------------------
 * Saves two input images to disk and uses them to perform face replacement
 * using OpenCV. Ensures thread-safe access to the shared temporary file via a
 * mutex lock. Returns true on success, or false after sending an appropriate
 * error message to the client.
 */
bool save_and_replace_image(ClientInfo* clt, uint8_t* image1,
        uint32_t image1Size, uint8_t* image2, uint32_t image2Size,
        int* replaceResult)
{
    bool error;
    // Avoid race condition
    pthread_mutex_lock(&clt->lock);
    FILE* file1 = fopen(TEMP_IMAGE_PATH, "wb");
    fwrite(image1, 1, image1Size, file1); // write the image1 into temp file
    fclose(file1);
    IplImage* frame = cvLoadImage(TEMP_IMAGE_PATH, CV_LOAD_IMAGE_COLOR);
    // Grab the frame from the path first

    FILE* file2 = fopen(TEMP_IMAGE_PATH, "wb");
    fwrite(image2, 1, image2Size, file2); // overwrite the image2 into temp file
    fclose(file2);
    IplImage* replace = cvLoadImage(TEMP_IMAGE_PATH, CV_LOAD_IMAGE_UNCHANGED);
    // Grab the frame from the path first

    // Check the replace result
    *replaceResult
            = replace_faces(frame, replace, TEMP_IMAGE_PATH, clt->faceCascade);
    pthread_mutex_unlock(&clt->lock);
    if (*replaceResult == -1) {
        // unable read file
        send_error(clt->clientfd, imageInvalidErrorMessage);
        error = true;
    } else if (*replaceResult == 1) {
        // No face detect
        send_error(clt->clientfd, noFaceErrorMessage);
        error = true;
    }
    if (error) {
        return false;
    }
    return true;
}

/*
 * handle_client
 * -------------
 * Handles the client's requests in a dedicated thread.
 * Reads and validates protocol messages,
 * performs face detection or replacement based on the request,
 * and sends back the processed image.
 * Manages memory and concurrency using mutex locks and performs cleanup on
 * failure.
 */
void* handle_client(void* arg)
{
    ClientInfo* clt = (ClientInfo*)arg;
    int fd = clt->clientfd;
    uint8_t operation;
    uint32_t image1Size;
    uint8_t* image1 = NULL;
    while (1) {
        // loop keep process until sth wrong
        // handle multi request
        if (!read_prefix(fd)) { // read prefix
            return NULL;
        }
        if (!read_operation(fd, &operation)) { // read operation type
            return NULL;
        }
        if (!read_image(fd, &image1Size, clt->maxSize, &image1)) {
            return NULL;
        }
        if (operation == REQUEST_DETECT) {
            int detectResult;
            if (!save_and_detect_image(
                        clt, image1, image1Size, &detectResult)) {
                free(image1);
                return NULL;
            }
        }
        if (operation == REQUEST_REPLACE) {
            uint32_t image2Size;
            uint8_t* image2 = NULL;
            if (!read_image(fd, &image2Size, clt->maxSize, &image2)) {
                free(image1);
                return NULL;
            }
            int replaceResult;
            if (!save_and_replace_image(clt, image1, image1Size, image2,
                        image2Size, &replaceResult)) {
                free(image1);
                free(image2);
                return NULL;
            }
            free(image2);
        }
        pthread_mutex_lock(&clt->lock); // avoid race condition
        send_client(fd); // send the image data to client if it reaach here
        pthread_mutex_unlock(&clt->lock);
        free(image1);
    }
    return NULL;
}

/*
 * run_server
 * ----------
 * Sets up a listening TCP socket on the specified port, prints the actual port
 * number, and enters an infinite loop to accept client connections. For each
 * accepted client, it spawns a new thread to handle the request using
 * handle_client().
 *
 * Exits the program with EXIT_PORT_STATUS if socket creation, binding, or
 * listening fails.
 * REF: net4.c from week 9 Lec
 * REF: server-multithreaded.c from week 10 Lec
 */
void run_server(Arguments* args, pthread_mutex_t lock)
{
    struct addrinfo* ai = 0;
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo)); // clearing hint
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP
    hints.ai_flags = AI_PASSIVE; // for server bind
    if (getaddrinfo("localhost", args->port, &hints, &ai) != 0) {
        cleanup_and_exit(args, EXIT_PORT_STATUS);
    }
    // Create and bind the server socket
    args->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (args->sockfd < 0
            || bind(args->sockfd, ai->ai_addr, ai->ai_addrlen) != 0) {
        freeaddrinfo(ai);
        cleanup_and_exit(args, EXIT_PORT_STATUS);
    }
    if (strcmp(args->port, "0") == 0) {
        // Port is ephemeral
        struct sockaddr_in ad;
        memset(&ad, 0, sizeof(struct sockaddr_in));
        socklen_t len = sizeof(struct sockaddr_in);
        if (getsockname(args->sockfd, (struct sockaddr*)&ad, &len) == 0) {
            // retrieves the port the OS picked
            fprintf(stderr, "%u\n", ntohs(ad.sin_port));
            fflush(stderr);
        }
    } else { // print the given port
        fprintf(stderr, "%s\n", args->port);
        fflush(stderr);
    }
    if (listen(args->sockfd, args->clientLimit) != 0) {
        freeaddrinfo(ai);
        cleanup_and_exit(args, EXIT_PORT_STATUS);
    }
    while (1) { // Repeatedly accept connections
        int clientfd = accept(args->sockfd, NULL, NULL); // accept connect
        ClientInfo* clt = malloc(sizeof(ClientInfo));
        // sem_wait(&args->clientSlot);
        clt->clientfd = clientfd;
        clt->maxSize = args->maxSize;
        clt->lock = lock;
        clt->faceCascade = args->faceCascade;
        clt->eyesCascade = args->eyesCascade;
        // clt->mutex = args->mutex;
        pthread_t tid; // spawn thread
        pthread_create(&tid, NULL, handle_client, clt);
        pthread_detach(tid); // detach the thread
    }
    freeaddrinfo(ai);
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
    if (argc < minArgsCount || argc > maxArgsCount) {
        // the argument count must at least be 3 or at most 4
        cleanup_and_exit(args, EXIT_USAGE_STATUS);
    }

    args->clientLimit = check_clientlimit(argv[clientLimitIndex], args);
    args->maxSize = check_maxsize(argv[maxSizeIndex], args);

    if (argc == maxArgsCount) {
        // Port is optional
        check_emptystring(argv[portIndex], args);
        args->port = strdup(argv[portIndex]);
    } else {
        args->port = strdup("0"); // can be free safely with strdup()
    }
    return args;
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
    pthread_mutex_t lock;
    pthread_mutex_init(&lock, NULL);
    Arguments* args = parse_arguments(argc, argv);
    // sem_init(&args->clientSlot, 0, args->clientLimit);
    check_cascade(args);
    check_image_file(args);
    run_server(args, lock);
    cleanup_and_exit(args, 0);
}
