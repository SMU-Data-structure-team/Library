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

//SQL ���� ���,���� �׽�Ʈ, ����Ʈ����, �α�������Ȯ�� �Լ� ����

void printSQLError(SQLSMALLINT handleType, SQLHANDLE handle) {

    SQLCHAR sqlState[15];
    SQLCHAR message[MESSAGE_BUFFER_SIZE];
    SQLINTEGER nativeError;
    SQLSMALLINT textLength;
    int i = 1;
    SQLRETURN ret;

    while ((ret = SQLGetDiagRec(handleType, handle, i,
        sqlState, &nativeError,
        message, MESSAGE_BUFFER_SIZE, &textLength)) == SQL_SUCCESS) {
        sqlState[5] = '\0';  // SQLSTATE�� 5�ڸ��̹Ƿ� �����ϰ� null-terminate

        printf("���� %d:\n", i);
        printf("  SQLSTATE: %.5s\n", sqlState);
        printf("  Native Error: %d\n", nativeError);
        printf("  Message: %.*s\n", (int)textLength, (char*)message);

        i++;
    }

    if (i == 1) {
        // ���� �޽����� ���ٸ� �߰� ����
        ret = SQLGetDiagField(handleType, handle, 1, SQL_DIAG_MESSAGE_TEXT,
            message, MESSAGE_BUFFER_SIZE, &textLength);
        if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
            printf("�߰� ���� �޽���: %.*s\n", (int)textLength, (char*)message);
        }
        else {
            printf("���� ������ �������� ���߽��ϴ�.\n");
        }
    }
}

void testQuery(SQLHDBC hDbc) {
    SQLHSTMT hStmt;
    SQLRETURN ret;

    ret = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    if (!SQL_SUCCEEDED(ret)) {
        printf("Statement �ڵ� �Ҵ� ����\n");
        return;
    }

    ret = SQLExecDirectA(hStmt, (SQLCHAR*)"SELECT 1", SQL_NTS);
    //ret = SQLExecDirectA(hStmt, (SQLCHAR*)"SELECT DB_NAME()", SQL_NTS);

    if (SQL_SUCCEEDED(ret)) {
        printf("���� ���� ����\n");
    }
    else {
        printf("���� ���� ����\n");
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
}

void PrintError(SQLHANDLE handle, SQLSMALLINT handleType, const char* msg) {
    SQLCHAR sqlState[15];
    SQLCHAR errorMsg[256];
    SQLINTEGER nativeError;
    SQLSMALLINT textLength;
    SQLRETURN ret;
    int i = 1;

    printf("%s ���� �߻�:\n", msg);

    while ((ret = SQLGetDiagRecA(handleType, handle, i, sqlState, &nativeError, errorMsg, sizeof(errorMsg), &textLength)) != SQL_NO_DATA) {
        if (SQL_SUCCEEDED(ret)) {
            sqlState[5] = '\0';
            printf("  SQLState: %s\n", sqlState);
            printf("  NativeError: %d\n", nativeError);
            printf("  Message: %s\n", errorMsg);
        }
        i++;
    }
}

void testLoginQuery(SQLHDBC hDbc) {
    SQLHSTMT hStmt;
    SQLRETURN ret;

    const char query[] = "SELECT COUNT(*) FROM dbo.Users WHERE username=? AND [password]=?";
    const char* testUsername = "dd";
    const char* testPassword = "dd";
    SQLLEN uLen = SQL_NTS;
    SQLLEN pLen = SQL_NTS;
    int count = 0;

    ret = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    if (!SQL_SUCCEEDED(ret)) {
        printf("Statement �ڵ� �Ҵ� ����\n");
        return;
    }

    ret = SQLPrepare(hStmt, (SQLCHAR*)query, SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        PrintError(hStmt, SQL_HANDLE_STMT, "SQLPrepare");
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return;
    }

    ret = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 0, 0,
        (SQLPOINTER)testUsername, 0, &uLen);
    if (!SQL_SUCCEEDED(ret)) {
        PrintError(hStmt, SQL_HANDLE_STMT, "BindParam username");
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return;
    }

    ret = SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 0, 0,
        (SQLPOINTER)testPassword, 0, &pLen);
    if (!SQL_SUCCEEDED(ret)) {
        PrintError(hStmt, SQL_HANDLE_STMT, "BindParam password");
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return;
    }

    ret = SQLExecute(hStmt);
    if (!SQL_SUCCEEDED(ret)) {
        PrintError(hStmt, SQL_HANDLE_STMT, "SQLExecute");
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return;
    }

    ret = SQLBindCol(hStmt, 1, SQL_C_SLONG, &count, 0, NULL);
    if (!SQL_SUCCEEDED(ret)) {
        PrintError(hStmt, SQL_HANDLE_STMT, "BindCol");
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return;
    }

    if (SQLFetch(hStmt) == SQL_SUCCESS) {
        printf("�α��� �׽�Ʈ ���� ���� ���: %d�� ��ġ\n", count);
    }
    else {
        printf("��� ���� �Ǵ� Fetch ����\n");
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
}
