#include "Network/Network.h"
#include "Network/ServerRegistry/ServerRegistry.h"
#include "MemoryPool/MemoryPool.h"
#include <functional>
#include <iostream>
#include <mcheck.h>
#include <thread>
int main() {
  muntrace();
  int input;
  G_MemoryPool->Init_MemoryPool(30000);
  if (!G_network->Init()) {
    std::cout << "Network initialization failed" << std::endl;
    muntrace();
    return -1;
  }
  std::thread epollserver_mainthread(
      std::bind(&Network::StartNetwork, G_network.get(), 2, 2));
  std::thread findserver(
      std::bind(&ServerRegistry::StartNetwork, G_ServerRegistry.get(), 1));

  while (1) {

  //  std::cout << "서버종료 하실거에요? 1번:";
    std::cin >> input;
    if (input == 1) {
      G_network->Release();
      if (epollserver_mainthread.joinable())
        epollserver_mainthread.join();
      G_ServerRegistry->Release();
      if (findserver.joinable())
        findserver.join();
      break;
    }
  }
  muntrace();
  return 0;
}