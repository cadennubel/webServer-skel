#include "webServer.h"
#include <signal.h>

// *************************************************************************
// * processConnect()
// *************************************************************************
void sig_handler(int signo) {
 DEBUG << "Caught signal #" << signo << ENDL;
 DEBUG << "Closing file descriptors 3-31." << ENDL;
 closefrom(3);
 exit(1);
}
int processConnection(int sockFd) {
  std::string filename, request, bodyBegin;
  size_t contentLength = 0;

  int connectionResponse = readHeader(sockFd, filename, request, bodyBegin, contentLength);

  if (connectionResponse == 400) {
    send400(sockFd);
  } else if (connectionResponse == 404) {
    send404(sockFd);
  } else if (connectionResponse == 200 && request == GET) {
    sendFile(sockFd, filename);
  } else if (connectionResponse == 200 && request == HEAD) {
    sendHeader(sockFd, filename);
  } else if (connectionResponse == 200 && request == POST) {
    saveFile(sockFd, filename, bodyBegin, contentLength);
  }
  return 0;
}


int main (int argc, char *argv[]) {

  // ********************************************************************
  // 1. Process the command line arguments
  // ********************************************************************
  int opt = 0;
  while ((opt = getopt(argc,argv,"d:")) != -1) {
    
    switch (opt) {
    case 'd':
      LOG_LEVEL = std::stoi(optarg);
      break;
    case ':':
    case '?':
    default:
      std::cout << "usage: " << argv[0] << " -d LOG_LEVEL" << std::endl;
      exit(-1);
    }
  }


  // Catch SIGINT and send it to sig_handler
  signal(SIGINT,sig_handler);
  // ********************************************************************
  // 2. Create and fill socket address structure.
  // ********************************************************************
  int sockFd;
  sockFd = socket(AF_INET, SOCK_STREAM, 0);
  DEBUG << "Calling Socket() assigned file descriptor " << sockFd << ENDL;
  if(sockFd < 0){
    FATAL << "Socket() failed: " << strerror(errno) << ENDL;
    exit(-1);
  }

  // ********************************************************************
  // * The bind() call takes a structure used to spefiy the details of the connection. 
  // *
  // * struct sockaddr_in servaddr;
  // *
  // On a cient it contains the address of the server to connect to. 
  // On the server it specifies which IP address and port to lisen for connections.
  // If you want to listen for connections on any IP address you use the
  // address INADDR_ANY
  // ********************************************************************

  uint16_t port = 1200;
  bool exitLoop = false;

  // 4. Loop until exit loop is true
  struct sockaddr_in server_addr;
  bzero(&server_addr, sizeof(server_addr)); 

  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = INADDR_ANY;
  server_addr.sin_port = htons(port);

  while (exitLoop == false) {
    server_addr.sin_port = htons(port);
    DEBUG << "Calling bind()" << ENDL;
    if (bind(sockFd, (struct sockaddr*)&server_addr,
               sizeof(server_addr)) < 0) {

          if (errno == EADDRINUSE) {
              ERROR << "Port " << port
                    << " is already in use, trying next port" << ENDL;
              port++;
              continue;
          }
          else {
              FATAL << "bind() failed: "
                    << strerror(errno) << ENDL;
              exit(-1);
          }
    }
      else {
          exitLoop = true;
          INFO << "Using port: " << port << ENDL;
      }
  }   
  //5. Listening
  DEBUG << "Calling listen()" << ENDL;
  int backLog = 10;
  int listened = 1;

  listened = listen(sockFd, backLog);

  if(listened < 0){
      FATAL << "listen() failed: "
            << strerror(errno) << ENDL;
      exit(-1);
  }


int quitProgram = 0;
while (!quitProgram){
    
    DEBUG << "Calling connFd = accept(fd,NULL,NULL)." << ENDL;
    int connFd = 0;
    if((connFd = accept(sockFd,NULL,NULL)) < 0){
        FATAL << "accept() failed:" 
              << strerror(errno) << ENDL;
              exit(-1);
    }
    DEBUG << "We have recieved a connection on " << connFd << ". Calling processConnection(" << connFd << ")" << ENDL;    
    quitProgram = processConnection(connFd);
    DEBUG << "processConnection returned " << quitProgram << " (should always be 0)" << ENDL;
    DEBUG << "Closing file descriptor " << connFd << ENDL;
    close(connFd);
}

  ERROR << "Program fell through to the end of main. A listening socket may have closed unexpectadly." << ENDL;
  closefrom(3);
}
