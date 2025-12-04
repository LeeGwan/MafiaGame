#ifndef CONNECTIONPOOL_H
#define CONNECTIONPOOL_H
#pragma once
#include <mutex>
#include <queue>
#include <string>
#include <condition_variable>
typedef struct MYSQL;

class ConnectionPool {
public:
  ConnectionPool(const std::string &host, const std::string &user,
                 const std::string &password, const std::string &database,
                 int count);// 생성자: 커넥션 풀 생성
  ~ConnectionPool();        // 소멸자: 모든 연결 종료
    MYSQL* Get_MYSQL();     // 연결 가져오기
    void   return_MYSQL(MYSQL* returnSQL); // 연결 반환
private:
  std::queue<MYSQL*> available_MYSQL;       // 사용 가능한 MYSQL 큐
  std::mutex available_MYSQL_MTX;           // 뮤텍스
  std::condition_variable cv;               // 대기 조건 변수

};

#endif