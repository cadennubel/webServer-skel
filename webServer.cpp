#include "webServer.h"
#include <signal.h>
// **************************************************************************************
// * processRequest,
//   - Return HTTP code to be sent back
//   - Set filename if appropriate. Filename syntax is valided but existance is not verified.
// **************************************************************************************
void sig_handler(int signo) {
 DEBUG << "Caught signal #" << signo << ENDL;
 DEBUG << "Closing file descriptors 3-31." << ENDL;
 closefrom(3);
 exit(1);
}

int readHeader(int sockFd, std::string &filename, std::string &request, std::string &bodyBegin, size_t &contentLength) {

    std::string container;
    int returnCode = 400;
    char buffer[10];
    bool endHeader = false;

    // Read everything up to and including the header.
    while (!endHeader) {

        ssize_t bytesRead = read(sockFd, buffer, 10);
        container.append(buffer, bytesRead);

        size_t headerEnd = container.find("\r\n\r\n");

        if (headerEnd != std::string::npos) {
            endHeader = true;

            if(container.size() > headerEnd + 4){
                bodyBegin = container.substr(headerEnd + 4);
            }
        }

        DEBUG << "Header: " << container << ENDL;
    }

    // Find the first part of the request.
    size_t lineEnd = container.find("\r\n");
    std::string lineString = container.substr(0, lineEnd);

    size_t endRequest = lineString.find(' ');
    std::string getPostHead = lineString.substr(0, endRequest);

  if(getPostHead == "GET" || getPostHead == "POST" || getPostHead == "HEAD"){
        request = getPostHead;
    }
    else{
        ERROR << "Error not a valid request Get, Head, Post" << ENDL;
        return returnCode;
    }

    // Get Content-Length for POST requests.
    if(request == "POST"){

        size_t lengthStart = container.find("Content-Length:");

        if(lengthStart == std::string::npos){
            ERROR << "POST request missing Content-Length" << ENDL;
            return 400;
        }

        std::stringstream ss(container.substr(lengthStart + 15));
        ss >> contentLength;
    }

    size_t endurl = lineString.find(' ', endRequest + 1);

    std::string url = lineString.substr(endRequest + 1,
                                        endurl - endRequest - 1);

    filename = url.substr(1);

    INFO << "Filename is " << filename << ENDL;


    std::regex pattern("(file[0-9]\\.html|image[0-9]\\.jpg)");

    if (std::regex_match(filename, pattern)) {
        filename = "data/" + filename;
        returnCode = 200;
        DEBUG << "Code 200" << ENDL;
    }
    else{
        returnCode = 404;
        DEBUG << "CODE 404 Regex Error" << ENDL;
    }

    return returnCode;
}
  
void sendLine(int sockFd, std::string &stringToSend){
//Convert the std::string to an array that is 2 bytes longer than the string
size_t length = stringToSend.length();
char line[length + 2];
stringToSend.copy(line, length);
//replace the last two bytes with carriage return and line feed.
line[length] = '\r';
line[length+1] = '\n';
//use write to senf the array.
DEBUG << "Writing" << ENDL;
write(sockFd, line, (length+2));
return;
}

void send404(int sockFd){
  std::string message404 = "HTTP/1.0 404 Not Found";
  std::string contentType ="content-type: text/html";
  std::string noText = "";
  std::string friendlyMessage = "Your request is not able to be completed from this server.";
  std::string contentLength = "Content-Length: " + std::to_string(body.length());
  sendLine(sockFd, message404);
  sendLine(sockFd, contentType);
  sendLine(sockFd, noText);
  sendLine(sockFd, friendlyMessage);
  sendLine(sockFd, noText);
  return;
}

void send400(int sockFd){
  std::string message400 = "HTTP/1.0 400 Bad Request";
  std::string noText = "";
  sendLine(sockFd, message400);
  sendLine(sockFd, noText);
  return;
}

void sendFile(int sockFd, const std::string &filename){
  std::string message200 = "HTTP/1.0 200 OK";
  std::string noText = "";
  std::string contentImage = "Content-Type: image/jpeg";
  std::string contentFile = "Content-Type: text/html";
  std::string contentType;

  struct stat fileStat;
  if(stat(filename.c_str(), &fileStat) < 0){
    ERROR <<"File not found " << filename << ENDL;
    send404(sockFd);
    return;
  }
  size_t fileSize = fileStat.st_size;

  if(filename.find("html") != std::string::npos){
    contentType = contentFile;
  }
  else if(filename.find("jpg") != std::string::npos){
    contentType = contentImage;
  }
  std::string contentLength = "Content-Length: " + std::to_string(fileSize);
  INFO << "Sending 200 " << filename << " | file size: " << fileSize << " bytes" << ENDL;

  sendLine(sockFd, message200);
  sendLine(sockFd, contentType);
  sendLine(sockFd, contentLength);
  sendLine(sockFd, noText); 


  int fileFd = open(filename.c_str(), O_RDONLY);
  if (fileFd < 0){
    ERROR << "Failed to open file " << filename << ENDL;
    return;
  }

char *buffer = new char[10];
size_t bytesSent = 0;

while (bytesSent != fileSize) {
    bzero(buffer, 10);

    ssize_t bytesRead = read(fileFd, buffer, 10);

    if (bytesRead <= 0) {
        break;
    }

    ssize_t bytesWritten = write(sockFd, buffer, bytesRead);

    if (bytesWritten < 0) {
        ERROR << "Failed to write to socket" << ENDL;
        break;
    }

    bytesSent += bytesWritten;
}

delete[] buffer;
close(fileFd);
return;
}

void sendHeader(int sockFd, const std::string &filename) {
  std::string message200 = "HTTP/1.0 200 OK";
  std::string noText = "";
  std::string contentImage = "Content-Type: image/jpeg";
  std::string contentFile = "Content-Type: text/html";
  std::string contentType;

  struct stat fileStat;
  if (stat(filename.c_str(), &fileStat) < 0) {
    ERROR << "File not found " << filename << ENDL;
    send404(sockFd);
    return;
  }
  size_t fileSize = fileStat.st_size;

  if (filename.find("html") != std::string::npos) {
    contentType = contentFile;
  }
  else if (filename.find("jpg") != std::string::npos) {
    contentType = contentImage;
  }

  std::string contentLength = "Content-Length: " + std::to_string(fileSize);

  INFO << "Sending HEAD 200 " << filename << " | file size: " << fileSize << " bytes" << ENDL;

  sendLine(sockFd, message200);
  sendLine(sockFd, contentType);
  sendLine(sockFd, contentLength);
  sendLine(sockFd, noText);

  return;
}

void saveFile(int sockFd, const std::string &filename, const std::string &bodyBegin, size_t contentLength){
  std::string message200 = "HTTP/1.0 200 OK";
  std::string message201 = "HTTP/1.0 201 Created";
  std::string noText = "";

  bool fileExists = false;
  struct stat fileStat;

  if (stat(filename.c_str(), &fileStat) == 0){
    fileExists = true;
  }

  int fileFd = open(filename.c_str(), O_TRUNC | O_CREAT | O_WRONLY, 0644);

  if(fileFd < 0){
    ERROR << "Failed to open file in POST " << filename << ENDL;
    send400(sockFd);
    return;
  }

  size_t bytesSaved = 0;
  if (!bodyBegin.empty()) {
    ssize_t written = write(fileFd, bodyBegin.data(), bodyBegin.size());
    if (written > 0) {
      bytesSaved = written;
    }
  }

  char buffer[BUFFER_SIZE];
  while (bytesSaved < contentLength) {
    size_t bytesLeft = contentLength - bytesSaved;

    size_t bytesToRead;
    if (bytesLeft < BUFFER_SIZE){
      bytesToRead = bytesLeft;
    }
    else{
      bytesToRead = BUFFER_SIZE;
    }

    ssize_t bytesRead = read(sockFd, buffer, bytesToRead);
    if(bytesRead <= 0){
      ERROR << "Reading failed in POST" << ENDL;
      break;
    }
    ssize_t bytesWritten = write(fileFd, buffer, bytesRead);
    if (bytesWritten < 0){
      ERROR << "Writing failed in POST" << ENDL;
      break;
    }
    bytesSaved += bytesWritten;
  }
  close(fileFd);

  INFO << "Saved " << bytesSaved << " bytes to " << filename << ENDL;

  std::string contentLengthHeader = "Content-Length: " + std::to_string(bytesSaved);

  if(fileExists){
    sendLine(sockFd, message200);
  }
  else{
    sendLine(sockFd, message201);
  }
  sendLine(sockFd, contentLengthHeader);
  sendLine(sockFd, noText);

  return;
}
  


// *************************************************************************
// * processConnect()
// *************************************************************************
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


  // Ccdatch SIGINT and send it to sig_handler
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
  // ********************************************************************\
  

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
