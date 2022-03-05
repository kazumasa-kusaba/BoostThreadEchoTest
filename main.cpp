#include <iostream>
#include <boost/thread.hpp>
#include <boost/thread/thread.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>

const std::string MAIN_UNIQUE_KEY              = "MAIN_UNIQUE_KEY";
const int         MAIN_MAX_MESSAGE_SIZE        = 500;
const std::string ECHO_CLIENT_UNIQUE_KEY       = "ECHO_CLIENT_UNIQUE_KEY";
const int         ECHO_CLIENT_MAX_MESSAGE_SIZE = 500;

enum COMMAND {
  SEND_MESSAGE = 0,
  EXIT_THREAD,
};

struct MESSAGE {
  COMMAND     command;
  std::string message;
};

void echo_client_function(void)
{
  try {
    boost::interprocess::message_queue echo_client_msgq(boost::interprocess::open_only, 
                                                        ECHO_CLIENT_UNIQUE_KEY.c_str()); 
    boost::interprocess::message_queue main_msgq(boost::interprocess::open_only, 
                                                 MAIN_UNIQUE_KEY.c_str()); 
    MESSAGE      message_to_echo_client, message_to_main;
    size_t       recv_size = 0;
    unsigned int priority  = 0;

    while (1) {
      // receive message
      echo_client_msgq.receive(&message_to_echo_client, sizeof(MESSAGE), recv_size, priority);

      std::cout << "test" << std::endl;
      
      // handle message
      switch (message_to_echo_client.command) {
        case SEND_MESSAGE:
          message_to_main.command = EXIT_THREAD;
          message_to_main.message = message_to_echo_client.message;
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
    boost::interprocess::message_queue::remove(ECHO_CLIENT_UNIQUE_KEY.c_str());
    // create message queue
    boost::interprocess::message_queue echo_client_msgq(boost::interprocess::create_only, 
                                                        ECHO_CLIENT_UNIQUE_KEY.c_str(),
                                                        ECHO_CLIENT_MAX_MESSAGE_SIZE,
                                                        sizeof(MESSAGE));

    // if the message queue left, it needs to be removed.
    boost::interprocess::message_queue::remove(MAIN_UNIQUE_KEY.c_str());
    // create message queue
    boost::interprocess::message_queue main_msgq(boost::interprocess::create_only, 
                                                 MAIN_UNIQUE_KEY.c_str(),
                                                 MAIN_MAX_MESSAGE_SIZE,
                                                 sizeof(MESSAGE));

    // start thread
    boost::thread thread_echo_client(&echo_client_function);

    std::string  buf;
    MESSAGE      message_to_echo_client, message_to_main;
    size_t       recv_size = 0;
    unsigned int priority  = 0;

    while (1) {
      std::cout << "Input: ";
      std::cin  >> buf;

      message_to_echo_client.command = SEND_MESSAGE;
      message_to_echo_client.message = buf;

      echo_client_msgq.send(&message_to_echo_client, sizeof(message_to_echo_client), 0);

      main_msgq.receive(&message_to_main, sizeof(MESSAGE), recv_size, priority);
      std::cout << "command: " << message_to_main.command << std::endl;
      std::cout << "message: " << message_to_main.message << std::endl;
    }

  } catch (boost::interprocess::interprocess_exception& e) {
      std::cerr << e.what() << std::endl;
      return -1;
  }

  return 0;
}

