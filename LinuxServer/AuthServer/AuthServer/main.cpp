#include "Network/Network.h"
#include "MemoryPool/MemoryPool.h"
#include <functional>
#include <iostream>
#include <thread>
int main() {
     int input;
     G_MemoryPool->Init_MemoryPool(30000 );
    if(!G_network->ConnectSession())  {
      std::cout << "서버연결 실패"<<'\n';
      return 0;
      }
      std::thread epollserver_mainthread(
      std::bind(&Network::StartNetwork, G_network.get(), 2, 1));
        while (1) {

    std::cin >> input;
    if (input == 1) {
      G_network->Release();
      if (epollserver_mainthread.joinable())
        epollserver_mainthread.join();
      break;
    }
  }
}