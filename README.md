# Server-Client Socket Communication with Multithreading

This repository showcases an implementation of server-client communication using sockets, where the server interacts with multiple clients simultaneously. The project focuses on a scenario inspired by the Internet of Things (IoT), where a server manages multiple equipment (sensors) concurrently. Both the client and server components are developed in the C programming language using the standard C socket library.

## Introduction

This project demonstrates how communication between a server and multiple clients can be achieved using socket programming. The chosen theme involves an IoT scenario where a server is responsible for managing multiple equipment (sensors) concurrently. The development of both the client and server is carried out using the C programming language and the standard C socket library.

## Development

Throughout the development process, with guidance from the provided materials on Moodle, a client code was developed. This client code not only processes user input messages but also receives messages and information via sockets, in the case of interactions with the server. The server component had to handle various types of interactions, including adding new equipment, ensuring that the equipment being added doesn't exceed the server's capacity (limited to 15), notifying all other connected equipment about a new addition, managing removal, listing, and reading equipment. To simulate a database, a static array of 15 positions was used to store equipment information.

## Challenges

One of the main challenges faced during development was managing message broadcasting. Using the broadcast IP address resulted in a "permission denied" error. Additionally, handling multithreading in C using the pthread.h library was an interesting challenge.

## Getting Started

To run the server-client applications:

1. Clone this repository:
   ```sh
   git clone https://github.com/luisgsilva950/research-computer-network-socket-multithreading.git
   cd research-computer-network-socket-multithreading
   make all
   make test
