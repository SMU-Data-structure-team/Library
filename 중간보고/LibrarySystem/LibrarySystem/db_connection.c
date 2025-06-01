#define CRT_SECURE_NO_WARNINGS
#undef UNICODE
#undef _UNICODE
#define _MBCS
#define SQL_NOUNICODEMAP
#include <stdio.h>
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <stdlib.h>
#include <string.h> 
#include <wincrypt.h>
#include <schannel.h>
#include "BookManager.h"
#define MESSAGE_BUFFER_SIZE 8192

// DB연결, 회원가입, 로그인함수 있음


//DB연결
SQLHDBC initDB() {
    SQLHENV hEnv = NULL;
    SQLHDBC hDbc = NULL;
    SQLRETURN ret;

    //printf("DB 연결 시도 중...\n");

    ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        printf("환경 핸들 할당 실패\n");
        return NULL;
    }

    ret = SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        printf("환경 속성 설정 실패\n");
        SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
        return NULL;
    }

    ret = SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        printf("연결 핸들 할당 실패\n");
        SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
        return NULL;
    }

    SQLCHAR connStr[] =
        "Driver={ODBC Driver 17 for SQL Server};"
        "Server=library-userdb.database.windows.net,1433;"
        "Database=userdb;"
        "Uid=SHI04;"
        "Pwd=Library2025;"
        "Encrypt=yes;"
        "TrustServerCertificate=yes;"
        "Connection Timeout=60;";

    //printf("연결 문자열: %s\n", connStr);
    ret = SQLDriverConnectA(hDbc, NULL, connStr, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE);
    //printf("SQLDriverConnect 결과: %d\n", ret);
    /*
    if (SQL_SUCCEEDED(ret)) {
        printf("DB 연결 성공\n");
        return hDbc;
    }
    else {
        printf("DB 연결 실패\n");
        printSQLError(SQL_HANDLE_DBC, hDbc);
        SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
        SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
        return NULL;
    }
    */
}

//회원가입
void registerUser(SQLHDBC hDbc, const char* username, const char* password) {
    SQLHSTMT hStmt;
    SQLINTEGER count = 0;
    char query[256];
    SQLRETURN ret;

    ret = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    if (!SQL_SUCCEEDED(ret)) {
        printf("핸들 할당 실패\n");
        return;
    }

    sprintf(query, "SELECT COUNT(*) FROM dbo.Users WHERE username = '%s'", username);
    ret = SQLExecDirectA(hStmt, (SQLCHAR*)query, SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        printf("중복 체크 쿼리 실패\n");
        printSQLError(SQL_HANDLE_STMT, hStmt);
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return;
    }

    ret = SQLFetch(hStmt);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        printf("데이터 페치 실패\n");
        printSQLError(SQL_HANDLE_STMT, hStmt);
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return;
    }
    SQLGetData(hStmt, 1, SQL_C_SLONG, &count, 0, NULL);
    SQLFreeStmt(hStmt, SQL_CLOSE);

    if (count > 0) {
        printf("이미 존재하는 사용자명입니다.\n");
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return;
    }

    sprintf(query, "INSERT INTO dbo.Users (username, [password]) VALUES ('%s', '%s')", username, password);
    printf("회원가입 쿼리: %s\n", query);
    ret = SQLExecDirectA(hStmt, (SQLCHAR*)query, SQL_NTS);
    if (SQL_SUCCEEDED(ret)) {
        printf("회원가입 성공\n");
    }
    else {
        printf("회원가입 실패\n");
        printSQLError(SQL_HANDLE_STMT, hStmt);
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
}

void loginUser(SQLHDBC hDbc, const char* username, const char* password, int* result) {
    SQLHSTMT hStmt;
    SQLRETURN ret;
    int count = 0;
    char query[512] = { 0 };

    snprintf(query, sizeof(query),
        "SELECT COUNT(*) FROM dbo.Users WHERE username='%s' AND [password]='%s'",
        username, password);
    //printf("최종 쿼리: %s\n", query);

    // 특수문자 헥스 출력(확인용)
    /*
    for (int i = 0; query[i] != '\0'; i++) {
        printf("%02X ", (unsigned char)query[i]);
    }
    printf("\n");
    */
    SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    ret = SQLExecDirectA(hStmt, (SQLCHAR*)query, SQL_NTS);

    if (SQL_SUCCEEDED(ret)) {
        ret = SQLBindCol(hStmt, 1, SQL_C_SLONG, &count, 0, NULL);
        if (SQL_SUCCEEDED(ret) && SQLFetch(hStmt) == SQL_SUCCESS) {
            if (count > 0) {
                printf("로그인 성공!\n");
                *result = 1;
            }
            else {
                printf("로그인 실패. 아이디 또는 비밀번호가 틀렸습니다.\n");
                *result = 0;
            }
        }
        else {
            printf("로그인 결과 처리 중 오류 발생\n");
            PrintError(hStmt, SQL_HANDLE_STMT, "SQLFetch or BindCol");
            *result = 0;
        }
    }
    else {
        printf("SQLExecute 오류 발생:\n");
        PrintError(hStmt, SQL_HANDLE_STMT, "SQLExecDirectA");
        *result = 0;
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
}

