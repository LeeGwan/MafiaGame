#include "MemoryPool/MemoryPool.h"
#include "Network/Network.h"
#include "Network/SessionServerConnector/SessionServerConnector.h"
#include "Network/DedicatedManger/DedicatedManger.h"
#include <functional>
#include <iostream>
#include <mcheck.h>
#include <thread>
int main() {
  muntrace();
  int input;
  G_MemoryPool->MemoryPool_Init(30000,50000);
  if (!G_network->Init()) {
    std::cout << "Network initialization failed" << std::endl;
    muntrace();
    return -1;
  }
  G_DedicatedManger->initDedicatedManger();
  std::thread epollserver_mainthread(
      std::bind(&Network::StartNetwork, G_network.get(), 2, 2));

  std::thread epoll_Dediserverthread(
      std::bind(&DedicatedManger::StartNetwork, G_DedicatedManger.get(), 1, 1));
  while (1) {

    //std::cout << "서버종료 하실거에요? 1번:";
    std::cin >> input;
    if (input == 1) {
      G_SessionServerConnector->Release();
      G_network->Release();
      if (epollserver_mainthread.joinable())
        epollserver_mainthread.join();
      G_DedicatedManger->Release();
        if (epoll_Dediserverthread.joinable())
        epoll_Dediserverthread.join();
      break;
    }
  }
  muntrace();
  return 0;
}