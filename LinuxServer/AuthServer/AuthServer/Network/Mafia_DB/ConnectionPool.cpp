#include "ConnectionPool.h"
#include "mysql/mysql.h"
#include <mutex>
#include <iostream>
// 생성자: 지정 개수만큼 MYSQL 연결 
ConnectionPool::ConnectionPool(const std::string &host, const std::string &user,
                               const std::string &password,
                               const std::string &database, int count) {
  for (; count > 0; count--) {
    MYSQL *connection = mysql_init(NULL);
    if (mysql_real_connect(connection, host.c_str(), user.c_str(),
                           password.c_str(), database.c_str(), 0, NULL,
                           CLIENT_SSL)) {
      available_MYSQL.push(connection);
    }
  }
 
}
// 소멸자: 모든 연결 종료
ConnectionPool::~ConnectionPool() {

  while (!available_MYSQL.empty()) {
    mysql_close(available_MYSQL.front());
    available_MYSQL.pop();
  }
}
// MYSQL 연결 가져오기 (대기 가능)
MYSQL *ConnectionPool::Get_MYSQL() {
  std::unique_lock<std::mutex> lock(available_MYSQL_MTX);
  cv.wait(lock, [this] { return !available_MYSQL.empty(); });
  MYSQL *useMYSQL = available_MYSQL.front();
  available_MYSQL.pop();
  return useMYSQL;
}
// MYSQL 연결 반환
void ConnectionPool::return_MYSQL(MYSQL *returnSQL) {
  std::unique_lock<std::mutex> lock(available_MYSQL_MTX);
  available_MYSQL.push(returnSQL);
  cv.notify_one();
}