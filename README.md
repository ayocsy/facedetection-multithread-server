# Face Detection Client–Server System (C · Socket Programming)

A **C-based client–server application** that performs face detection through a custom communication protocol, demonstrating **low-level networking, modular C design, and system programming** concepts.

---

## Overview

This project implements a **client–server architecture** where the client sends image data to a server, and the server performs **face detection** before returning the results.  
The system is written entirely in **C**, focusing on performance, memory control, and protocol design.

---

## System Design

- **Client**
  - Sends image data and requests to the server
  - Handles network communication and protocol encoding

- **Server**
  - Receives image data from clients
  - Performs face detection logic
  - Sends detection results back to the client

- **Protocol Layer**
  - Defines a custom binary protocol for communication
  - Ensures structured, reliable message exchange

---

## Key Components

- `faceclient.c / faceclient.h`  
  Client-side networking and request handling

- `facedetect.c / facedetect.h`  
  Face detection logic and processing

- `protocol.c / protocol.h`  
  Custom communication protocol implementation

- `Makefile`  
  Build automation for compiling the project

---

## Tech Stack & Concepts

- C (system-level programming)
- TCP socket programming
- Client–server architecture
- Custom protocol design
- Modular C code structure
- Memory management


```bash
make
