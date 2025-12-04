#include "MemoryPool/MemoryPool.h"
#include "Network/Network.h"
#include <functional>
#include <iostream>
#include <mcheck.h>
#include <thread>
int main() {
  muntrace();
  int input;
  G_MemoryPool->MemoryPool_Init(30000);
  if (!G_network->Init()) {
    std::cout << "Network initialization failed" << std::endl;
    muntrace();
    return -1;
  }
  std::thread epollserver_mainthread(
      std::bind(&Network::StartNetwork, G_network.get(), 2, 2));


  while (1) {

    //std::cout << "서버종료 하실거에요? 1번:";
    std::cin >> input;
    if (input == 1) {
      G_network->Release();
      if (epollserver_mainthread.joinable())
        epollserver_mainthread.join();
     
      break;
    }
  }
  muntrace();
  return 0;
}