#define CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include "BookManager.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

//반납 함수

// 유니코드형 특수 따옴표 등 제거 + 좌우 공백 제거 + 중간 널 제거 
void cleanTitle(char* str) {
    char temp[MAX_TITLE_LEN];
    int j = 0;

    for (int i = 0; str[i] != '\0'; ++i) {
        if (str[i] != '"' && str[i] != '\0') {
            temp[j++] = str[i];
        }
    }
    temp[j] = '\0';

    int start = 0;
    while (isspace((unsigned char)temp[start])) start++;

    int end = strlen(temp) - 1;
    while (end >= start && isspace((unsigned char)temp[end])) end--;

    if (start > 0 || end < j - 1) {
        memmove(str, temp + start, end - start + 1);
        str[end - start + 1] = '\0';
    }
    else {
        strcpy(str, temp);
    }
}


void convertToUnicode(const char* src, wchar_t* dest, int destSize) {
    MultiByteToWideChar(CP_ACP, 0, src, -1, dest, destSize);
}

void convertToMultibyte(const wchar_t* src, char* dest, int destSize) {
    WideCharToMultiByte(CP_ACP, 0, src, -1, dest, destSize, NULL, NULL);
}


void returnBook(SQLHDBC hDbc, const char* username) {
    if (strlen(username) == 0) {
        printf("\n로그인이 필요합니다.\n");
        return;
    }

    SQLHSTMT hStmt;
    SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    if (!SQL_SUCCEEDED(ret)) {
        printf("SQL 핸들 할당 실패\n");
        return;
    }

    const char* selectQuery = "SELECT title FROM dbo.Loans WHERE username = ?";
    ret = SQLPrepareA(hStmt, (SQLCHAR*)selectQuery, SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        printf("SQL Prepare 실패\n");
        printSQLError(SQL_HANDLE_STMT, hStmt);
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return;
    }

    ret = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 50, 0, (SQLPOINTER)username, 0, NULL);
    if (!SQL_SUCCEEDED(ret)) {
        printf("SQL 파라미터 바인딩 실패\n");
        printSQLError(SQL_HANDLE_STMT, hStmt);
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return;
    }

    ret = SQLExecute(hStmt);
    if (!SQL_SUCCEEDED(ret)) {
        printf("대출 목록 조회 실패\n");
        printSQLError(SQL_HANDLE_STMT, hStmt);
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return;
    }

    char titles[100][256];
    int count = 0;
    SQLCHAR titleBuf[256];
    SQLLEN indicator;

    while (SQLFetch(hStmt) == SQL_SUCCESS && count < 100) {
        wchar_t wTitleBuf[256];
        SQLGetData(hStmt, 1, SQL_C_WCHAR, wTitleBuf, sizeof(wTitleBuf), &indicator);

        // wchar_t(유니코드) → 멀티바이트(char*) 변환
        WideCharToMultiByte(CP_ACP, 0, wTitleBuf, -1, titleBuf, sizeof(titleBuf), NULL, NULL);

        strncpy(titles[count], titleBuf, sizeof(titles[count]) - 1);
        titles[count][sizeof(titles[count]) - 1] = '\0';
        cleanTitle(titles[count]);

        printf("%d. %s\n", count + 1, titles[count]);
        count++;
    }

    if (count == 0) {
        printf("대출 중인 책이 없습니다.\n");
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return;
    }

    int choice = 0;
    while (1) {
        printf("반납할 책 번호를 선택하세요 (1~%d): ", count);
        if (scanf("%d", &choice) == 1 && choice >= 1 && choice <= count) {
            while (getchar() != '\n');
            break;
        }
        else {
            printf("잘못된 선택입니다. 다시 입력해주세요.\n");
            while (getchar() != '\n');
        }
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);

   // printf("[DEBUG] 삭제 시도: username = '%s', title = '%s'\n", username, titles[choice - 1]);

    ret = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    if (!SQL_SUCCEEDED(ret)) {
        printf("SQL 핸들 할당 실패\n");
        return;
    }

    // COLLATE 추가 → 정확히 같은 문자열 비교 (유니코드 포함)
    const char* deleteQuery =
        "DELETE FROM dbo.Loans WHERE username = ? AND title COLLATE Korean_Wansung_CI_AS = ?";
    ret = SQLPrepareA(hStmt, (SQLCHAR*)deleteQuery, SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        printf("SQL Prepare 실패\n");
        printSQLError(SQL_HANDLE_STMT, hStmt);
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return;
    }

    SQLLEN usernameLen = SQL_NTS;
    SQLLEN titleLen = SQL_NTS;
    ret = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 0, 0,
        (SQLPOINTER)username, 0, &usernameLen);

    wchar_t wTitleDelete[256];
    MultiByteToWideChar(CP_ACP, 0, titles[choice - 1], -1, wTitleDelete, 256);

    ret = SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 0, 0,
        (SQLPOINTER)wTitleDelete, 0, &titleLen);


    if (!SQL_SUCCEEDED(ret)) {
        printf("SQL 파라미터 바인딩 실패\n");
        printSQLError(SQL_HANDLE_STMT, hStmt);
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return;
    }

    //printf("[DEBUG] 삭제 전 실제 쿼리 대상 title (정제 후): '%s'\n", titles[choice - 1]);

    // (Optional) 삭제 전에 DB에서 다시 같은 title 있는지 확인할 수 있음
    //printf("[DEBUG] DB 내 username='%s' 의 title 목록 다시 조회:\n", username);
    // 여기에 SELECT 쿼리 삽입해서 재확인해도 좋음

    ret = SQLExecute(hStmt);
    //printf("[DEBUG] SQLExecute 결과: %d\n", ret);

    if (SQL_SUCCEEDED(ret)) {
        printf("'%s' 책 반납 완료되었습니다.\n", titles[choice - 1]);
    }
    else {
        printf("반납 처리 중 오류가 발생했습니다.\n");
        printSQLError(SQL_HANDLE_STMT, hStmt);
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
}
