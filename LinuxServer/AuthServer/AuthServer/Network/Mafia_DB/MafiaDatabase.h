#ifndef MAFIADATABASE_H
#define MAFIADATABASE_H
#pragma once
#include <memory>
#include <string>
class ConnectionPool;
enum class ResultType : uint8_t ;
class MafiaDatabase {
public:
    MafiaDatabase();                  // 생성자: DB 풀 초기화
    ~MafiaDatabase();                 // 소멸자: 리소스 해제

    ResultType Sign_up(const std::string& ID, const std::string& PW);     // 회원가입
    ResultType Sign_in(const std::string& ID, const std::string& PW, std::string& hash); // 로그인
    void Release();                    // DB 풀 해제

private:
    bool generate_session_token(std::string& user_hash, const std::string& account); // 세션 토큰 생성

private:
    std::unique_ptr<ConnectionPool> DB_POOL;  // DB 연결 풀
    const std::string DB_IP = "172.30.1.7";
    const std::string DB_ID = "tlkj12";
    const std::string DB_password = "Dl940725!@#";
    const std::string DB_TABLE = "Mafia_DB";
};

#endif