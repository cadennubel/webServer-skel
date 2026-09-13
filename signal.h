void sig_handler(int signo) {
 DEBUG << "Caught signal #" << signo << ENDL;
 DEBUG << "Closing file descriptors 3-31." << ENDL;
 closefrom(3);
 exit(1);
}
