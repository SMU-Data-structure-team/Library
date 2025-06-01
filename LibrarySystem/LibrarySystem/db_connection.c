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

// DB����, ȸ������, �α����Լ� ����


//DB����
SQLHDBC initDB() {
    SQLHENV hEnv = NULL;
    SQLHDBC hDbc = NULL;
    SQLRETURN ret;

    //printf("DB ���� �õ� ��...\n");

    ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &hEnv);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        printf("ȯ�� �ڵ� �Ҵ� ����\n");
        return NULL;
    }

    ret = SQLSetEnvAttr(hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        printf("ȯ�� �Ӽ� ���� ����\n");
        SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
        return NULL;
    }

    ret = SQLAllocHandle(SQL_HANDLE_DBC, hEnv, &hDbc);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        printf("���� �ڵ� �Ҵ� ����\n");
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

    //printf("���� ���ڿ�: %s\n", connStr);
    ret = SQLDriverConnectA(hDbc, NULL, connStr, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_COMPLETE);
    //printf("SQLDriverConnect ���: %d\n", ret);
    /*
    if (SQL_SUCCEEDED(ret)) {
        printf("DB ���� ����\n");
        return hDbc;
    }
    else {
        printf("DB ���� ����\n");
        printSQLError(SQL_HANDLE_DBC, hDbc);
        SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
        SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
        return NULL;
    }
    */
}

//ȸ������
void registerUser(SQLHDBC hDbc, const char* username, const char* password) {
    SQLHSTMT hStmt;
    SQLINTEGER count = 0;
    char query[256];
    SQLRETURN ret;

    ret = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    if (!SQL_SUCCEEDED(ret)) {
        printf("�ڵ� �Ҵ� ����\n");
        return;
    }

    sprintf(query, "SELECT COUNT(*) FROM dbo.Users WHERE username = '%s'", username);
    ret = SQLExecDirectA(hStmt, (SQLCHAR*)query, SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        printf("�ߺ� üũ ���� ����\n");
        printSQLError(SQL_HANDLE_STMT, hStmt);
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return;
    }

    ret = SQLFetch(hStmt);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
        printf("������ ��ġ ����\n");
        printSQLError(SQL_HANDLE_STMT, hStmt);
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return;
    }
    SQLGetData(hStmt, 1, SQL_C_SLONG, &count, 0, NULL);
    SQLFreeStmt(hStmt, SQL_CLOSE);

    if (count > 0) {
        printf("�̹� �����ϴ� ����ڸ��Դϴ�.\n");
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return;
    }

    sprintf(query, "INSERT INTO dbo.Users (username, [password]) VALUES ('%s', '%s')", username, password);
    printf("ȸ������ ����: %s\n", query);
    ret = SQLExecDirectA(hStmt, (SQLCHAR*)query, SQL_NTS);
    if (SQL_SUCCEEDED(ret)) {
        printf("ȸ������ ����\n");
    }
    else {
        printf("ȸ������ ����\n");
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
    //printf("���� ����: %s\n", query);

    // Ư������ �� ���(Ȯ�ο�)
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
                printf("�α��� ����!\n");
                *result = 1;
            }
            else {
                printf("�α��� ����. ���̵� �Ǵ� ��й�ȣ�� Ʋ�Ƚ��ϴ�.\n");
                *result = 0;
            }
        }
        else {
            printf("�α��� ��� ó�� �� ���� �߻�\n");
            PrintError(hStmt, SQL_HANDLE_STMT, "SQLFetch or BindCol");
            *result = 0;
        }
    }
    else {
        printf("SQLExecute ���� �߻�:\n");
        PrintError(hStmt, SQL_HANDLE_STMT, "SQLExecDirectA");
        *result = 0;
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
}

