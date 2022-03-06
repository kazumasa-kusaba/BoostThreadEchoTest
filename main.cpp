#include <iostream>
#include <boost/thread.hpp>
#include <boost/thread/thread.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>

const std::string MAIN_UNIQUE_KEY                   = "MAIN_UNIQUE_KEY";
const int         MAIN_MAX_MESSAGE_SIZE             = 500;
const std::string ECHO_BACK_CLIENT_UNIQUE_KEY       = "ECHO_BACK_CLIENT_UNIQUE_KEY";
const int         ECHO_BACK_CLIENT_MAX_MESSAGE_SIZE = 500;

enum COMMAND {
  SEND_MESSAGE = 0,
  EXIT_THREAD,
};

struct MESSAGE {
  COMMAND     command;
  std::string message;
};

void echo_back_client_entry(void)
{
  try {
    boost::interprocess::message_queue echo_back_client_msgq(
        boost::interprocess::open_only, 
        ECHO_BACK_CLIENT_UNIQUE_KEY.c_str()); 
    boost::interprocess::message_queue main_msgq(
        boost::interprocess::open_only, 
        MAIN_UNIQUE_KEY.c_str()); 
    MESSAGE      message_to_ECHO_BACK_CLIENT, message_to_main;
    size_t       recv_size = 0;
    unsigned int priority  = 0;

    while (1) {
      // receive message
      echo_back_client_msgq.receive(
          &message_to_ECHO_BACK_CLIENT,
          sizeof(MESSAGE),
          recv_size, 
          priority);

      // handle message
      switch (message_to_ECHO_BACK_CLIENT.command) {
        case SEND_MESSAGE:
          //std::cout << "command: " << message_to_ECHO_BACK_CLIENT.command << std::endl;
          //std::cout << "message: " << message_to_ECHO_BACK_CLIENT.message << std::endl;
          message_to_main.command = SEND_MESSAGE;
          message_to_main.message = message_to_ECHO_BACK_CLIENT.message;
          std::reverse(message_to_main.message.begin(), message_to_main.message.end());
          main_msgq.send(&message_to_main, sizeof(message_to_main), 0);
          break;
        case EXIT_THREAD:
          return;
        default:
          break;
      }
    }
  } catch (boost::interprocess::interprocess_exception& e) {
      std::cerr << e.what() << std::endl;
      exit(1);
  } catch (...) {
      std::cerr << "ERROR" << std::endl;
      exit(1);
  }
}

int main(void)
{
  try {
    // if the message queue left, it needs to be removed.
    boost::interprocess::message_queue::remove(ECHO_BACK_CLIENT_UNIQUE_KEY.c_str());
    // create message queue 
    boost::interprocess::message_queue echo_back_client_msgq(
        boost::interprocess::create_only, 
        ECHO_BACK_CLIENT_UNIQUE_KEY.c_str(),
        ECHO_BACK_CLIENT_MAX_MESSAGE_SIZE,
        sizeof(MESSAGE));

    // if the message queue left, it needs to be removed.
    boost::interprocess::message_queue::remove(MAIN_UNIQUE_KEY.c_str());
    // create message queue
    boost::interprocess::message_queue main_msgq(
        boost::interprocess::create_only, 
        MAIN_UNIQUE_KEY.c_str(),
        MAIN_MAX_MESSAGE_SIZE,
        sizeof(MESSAGE));

    // start thread
    boost::thread thread_echo_back_client(&echo_back_client_entry);

    std::string  buf;
    MESSAGE      message_to_ECHO_BACK_CLIENT, message_to_main;
    size_t       recv_size = 0;
    unsigned int priority  = 0;

    while (1) {
      std::cout << "Command: ";
      std::cin  >> buf;

      switch (std::strtol(buf.c_str(), nullptr, 10)) {
        case SEND_MESSAGE:
          std::cout << "Message: ";
          std::cin  >> buf;
          // build a message to ECHO_BACK_CLIENT
          message_to_ECHO_BACK_CLIENT.command = SEND_MESSAGE;
          message_to_ECHO_BACK_CLIENT.message = buf;
          // send a message to ECHO_BACK_CLIENT
          echo_back_client_msgq.send(
              &message_to_ECHO_BACK_CLIENT,
              sizeof(message_to_ECHO_BACK_CLIENT), 
              0);
          // receive a message from ECHO_BACK_CLIENT
          main_msgq.receive(&message_to_main, sizeof(MESSAGE), recv_size, priority);
          std::cout << "Message: " << message_to_main.message << std::endl;
          break;
        case EXIT_THREAD:
          // build a message to ECHO_BACK_CLIENT
          message_to_ECHO_BACK_CLIENT.command = EXIT_THREAD;
          // send a message to ECHO_BACK_CLIENT
          echo_back_client_msgq.send(
              &message_to_ECHO_BACK_CLIENT,
              sizeof(message_to_ECHO_BACK_CLIENT), 
              0);
          // MEMO: this code didn't work properly.
          //       the following error occured.
          //       ---------
          //       >>main(5926,0x70000681a000) malloc: *** error for object 0x7ffee008b9b8: pointer being freed was not allocated
          //       >>main(5926,0x70000681a000) malloc: *** set a breakpoint in malloc_error_break to debug
          //       ---------
          //       the thread has already finished so null pointer access occured??
          //thread_echo_back_client.join();
          //std::cout << "Thread finished!!" << std::endl;
          break;
        default:
          break;
      }
    }
  } catch (boost::interprocess::interprocess_exception& e) {
      std::cerr << e.what() << std::endl;
      exit(1);
  }

  return 0;
}

