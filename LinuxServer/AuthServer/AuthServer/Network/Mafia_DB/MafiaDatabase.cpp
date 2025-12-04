#include "MafiaDatabase.h"
#include "../Packet/PacketStructure/PacketStructure.h"
#include "ConnectionPool.h"
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <memory>
#include <mysql/mysql.h>
#include <openssl/evp.h>
#include <string>
#include <sys/time.h>
// 생성자: DB 연결 풀 생성
MafiaDatabase::MafiaDatabase() {
    DB_POOL = std::make_unique<ConnectionPool>(DB_IP, DB_ID, DB_password, DB_TABLE, 15);
}

// 소멸자: DB 풀 해제
MafiaDatabase::~MafiaDatabase() {
    Release();
}
// 회원가입 처리
ResultType MafiaDatabase::Sign_up(const std::string &ID,
                                  const std::string &PW) {
  MYSQL *SQL = DB_POOL->Get_MYSQL();
  if (!SQL) {
    return ResultType::SignUp_Failed;
  }
  std::vector<char> escaped_id(ID.length() * 2 + 1);
  std::vector<char> escaped_pw(PW.length() * 2 + 1);

  std::string hash;
  // MYSQL 반환 처리 보장
  std::unique_ptr<ConnectionPool, std::function<void(ConnectionPool *)>>
      MYSQLguard(DB_POOL.get(), [SQL](ConnectionPool *pool) {
        if (pool) {
          pool->return_MYSQL(SQL); // 멤버 함수 호출
        }
      });
  // 고유 해쉬값 생성
  if (!generate_session_token(hash, ID)) {
    return ResultType::SignUp_Failed;
  }
  std::vector<char> escaped_hash(hash.length() * 2 + 1);

  // SQL 인젝션 방지
  mysql_real_escape_string(SQL, escaped_id.data(), ID.c_str(), ID.length());
  mysql_real_escape_string(SQL, escaped_pw.data(), PW.c_str(), PW.length());
  mysql_real_escape_string(SQL, escaped_hash.data(), hash.c_str(),
                           hash.length());
  std::string query =
      "INSERT INTO Mafia_users_Information (id, pw, hash_value) VALUES ('" +
      std::string(escaped_id.data()) + "', '" + std::string(escaped_pw.data()) +
      "', '" + std::string(escaped_hash.data()) + "')";
  if (mysql_query(SQL, query.c_str())) {
    unsigned int err = mysql_errno(SQL);
    if (err == 1062) {
      return ResultType::SignUp_AlreadyExists; // 중복 ID
    }

    return ResultType::SignUp_Failed; // 회원가입 실패
  }
  return ResultType::SignUp_Succeeded; // 회원가입 성공
}

// 로그인 처리
ResultType MafiaDatabase::Sign_in(const std::string &ID, const std::string &PW,
                                  std::string &hash) {
  MYSQL *SQL = DB_POOL->Get_MYSQL();
  if (!SQL) {
    return ResultType::Login_Failed;
  }
  std::unique_ptr<ConnectionPool, std::function<void(ConnectionPool *)>>
      MYSQLguard(DB_POOL.get(), [SQL](ConnectionPool *pool) {
        if (pool) {
          pool->return_MYSQL(SQL);
        }
      });
  char escaped_id[ID.length() * 2 + 1];
  char escaped_pw[PW.length() * 2 + 1];
  mysql_real_escape_string(SQL, escaped_id, ID.c_str(), ID.length());
  mysql_real_escape_string(SQL, escaped_pw, PW.c_str(), PW.length());
  std::string query =
      "SELECT hash_value FROM Mafia_users_Information WHERE id='" +
      std::string(escaped_id) + "' AND pw='" + std::string(escaped_pw) + "'";

  if (mysql_query(SQL, query.c_str())) {
    return ResultType::Login_InvalidCredentials;
  }
  MYSQL_RES *result = mysql_store_result(SQL);

  if (!result)
    return ResultType::Login_InvalidCredentials;
  if (mysql_num_rows(result) == 0) {
    mysql_free_result(result);
    return ResultType::Login_InvalidCredentials;
  }
  MYSQL_ROW row = mysql_fetch_row(result);
  
  hash = row[0];
  mysql_free_result(result);
  return ResultType::Login_Succeeded;
}
// 세션 토큰 생성
bool MafiaDatabase::generate_session_token(std::string &user_hash,
                                           const std::string &account) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  double microtime = tv.tv_sec + (tv.tv_usec / 1000000.0);
  int random_num = rand() % 8999999 + 1000000; // 1000000~9999999

  // $account . microtime(true) . mt_rand 조합
  std::string unique_string =
      account + std::to_string(microtime) + std::to_string(random_num);

  unsigned char hash[32];

  EVP_MD_CTX *ctx = EVP_MD_CTX_new();
  if (!ctx)
    return false;

  EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
  EVP_DigestUpdate(ctx, unique_string.c_str(), unique_string.length());
  EVP_DigestFinal_ex(ctx, hash, NULL);
  EVP_MD_CTX_free(ctx);

  char hex_str[65];
  sprintf(hex_str,
          "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x"
          "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
          hash[0], hash[1], hash[2], hash[3], hash[4], hash[5], hash[6],
          hash[7], hash[8], hash[9], hash[10], hash[11], hash[12], hash[13],
          hash[14], hash[15], hash[16], hash[17], hash[18], hash[19], hash[20],
          hash[21], hash[22], hash[23], hash[24], hash[25], hash[26], hash[27],
          hash[28], hash[29], hash[30], hash[31]);
  user_hash = std::string(hex_str);

  return true;
}
// DB 풀 해제
void MafiaDatabase::Release() {
  if (DB_POOL)
    DB_POOL.reset();
}