#include <iostream>
#include <boost/thread.hpp>
#include <boost/thread/thread.hpp>
#include <boost/interprocess/ipc/message_queue.hpp>

const std::string ECHO_CLIENT_UNIQUE_KEY       = "ECHO_CLIENT_UNIQUE_KEY";
const int         ECHO_CLIENT_MAX_MESSAGE_SIZE = 500;
const std::string MAIN_UNIQUE_KEY              = "MAIN_UNIQUE_KEY";
const int         MAIN_MAX_MESSAGE_SIZE        = 4;

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
    boost::interprocess::message_queue msgq(boost::interprocess::open_only, 
                                            ECHO_CLIENT_UNIQUE_KEY.c_str()); 
    MESSAGE      message;
    size_t       recv_size = 0;
    unsigned int priority  = 0;

    while (1) {
      // receive message
      msgq.receive(&message, sizeof(MESSAGE), recv_size, priority);
      std::cout << "command: " << message.command << std::endl;
      std::cout << "message: " << message.message << std::endl;
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
  // create message queue
  try {
    // if the message queue left, it needs to be removed.
    boost::interprocess::message_queue::remove(ECHO_CLIENT_UNIQUE_KEY.c_str());
    boost::interprocess::message_queue msgq(
      boost::interprocess::create_only, 
      ECHO_CLIENT_UNIQUE_KEY.c_str(),
      ECHO_CLIENT_MAX_MESSAGE_SIZE,
      sizeof(MESSAGE));

    // start thread
    boost::thread thread_echo_client(&echo_client_function);

    while (1) {
      std::string buf;

      std::cout << "Input: ";
      std::cin  >> buf;

      MESSAGE message;
      message.command = SEND_MESSAGE;
      message.message = buf;

      msgq.send(&message, sizeof(message), 0);
    }

  } catch (boost::interprocess::interprocess_exception& e) {
      std::cerr << e.what() << std::endl;
      return -1;
  }

  return 0;
}

