#define CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include "BookManager.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <wchar.h>


void saveReservationToDB(const char* username, const char* bookTitleRaw) {
    char cleanedTitle[MAX_TITLE_LEN];
    strncpy(cleanedTitle, bookTitleRaw, MAX_TITLE_LEN);
    cleanedTitle[MAX_TITLE_LEN - 1] = '\0';
    cleanTitle(cleanedTitle);  // 문자열 정제

    SQLHDBC hDbc = initDB();
    if (hDbc == NULL) {
        printf("DB 연결 실패\n");
        return;
    }

    SQLHSTMT hStmt;
    SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    if (!SQL_SUCCEEDED(ret)) {
        printf("SQL 핸들 할당 실패\n");
        return;
    }

    const char* query = "INSERT INTO dbo.Reservations (username, bookTitle) VALUES (?, ?)";
    ret = SQLPrepareA(hStmt, (SQLCHAR*)query, SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        printf("SQL Prepare 실패\n");
        printSQLError(SQL_HANDLE_STMT, hStmt);
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return;
    }

    SQLLEN usernameLen = SQL_NTS;
    ret = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 50, 0,
        (SQLPOINTER)username, 0, &usernameLen);

    wchar_t wTitle[256];
    MultiByteToWideChar(CP_ACP, 0, cleanedTitle, -1, wTitle, sizeof(wTitle) / sizeof(wchar_t));
    SQLLEN titleLen = SQL_NTS;
    ret |= SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 200, 0,
        (SQLPOINTER)wTitle, 0, &titleLen);

    if (!SQL_SUCCEEDED(ret)) {
        printf("SQL 파라미터 바인딩 실패\n");
        printSQLError(SQL_HANDLE_STMT, hStmt);
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return;
    }

    ret = SQLExecute(hStmt);
    if (SQL_SUCCEEDED(ret)) {
        //printf("예약이 저장되었습니다.\n");
    }
    else {
        printf("예약 저장 실패: 중복 예약이거나 기타 오류입니다.\n");
        printSQLError(SQL_HANDLE_STMT, hStmt);
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
}

void deleteReservationFromDB(const char* username, const char* bookTitle) {
    char cleanedTitle[MAX_TITLE_LEN];
    strncpy(cleanedTitle, bookTitle, MAX_TITLE_LEN - 1);
    cleanedTitle[MAX_TITLE_LEN - 1] = '\0';

    cleanTitle(cleanedTitle);  // 쌍따옴표, 좌우 공백 등 제거

    SQLHDBC hDbc = initDB();
    if (hDbc == NULL) {
        printf("DB 연결 실패\n");
        return;
    }

    SQLHSTMT hStmt;
    SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    if (!SQL_SUCCEEDED(ret)) {
        printf("SQL 핸들 할당 실패\n");
        return;
    }

    const char* query = "DELETE FROM dbo.Reservations WHERE username = ? AND bookTitle COLLATE Korean_Wansung_CI_AS = ?";

    ret = SQLPrepareA(hStmt, (SQLCHAR*)query, SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        printf("SQL Prepare 실패\n");
        printSQLError(SQL_HANDLE_STMT, hStmt);
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return;
    }

    SQLLEN usernameLen = (SQLLEN)strlen(username);
    ret = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, usernameLen, 0, (SQLPOINTER)username, 0, &usernameLen);
    if (!SQL_SUCCEEDED(ret)) {
        printf("username 바인딩 실패\n");
        printSQLError(SQL_HANDLE_STMT, hStmt);
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return;
    }

    wchar_t wBookTitle[256];
    MultiByteToWideChar(CP_ACP, 0, cleanedTitle, -1, wBookTitle, sizeof(wBookTitle) / sizeof(wchar_t));
    SQLLEN titleLen = (SQLLEN)(wcslen(wBookTitle) * sizeof(wchar_t));
    ret = SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, titleLen, 0, (SQLPOINTER)wBookTitle, 0, &titleLen);
    if (!SQL_SUCCEEDED(ret)) {
        printf("bookTitle 바인딩 실패\n");
        printSQLError(SQL_HANDLE_STMT, hStmt);
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return;
    }

    ret = SQLExecute(hStmt);
    if (SQL_SUCCEEDED(ret)) {
        printf("예약이 삭제되었습니다.\n");
    }
    else {
        printf("예약 삭제 실패\n");
        //printSQLError(SQL_HANDLE_STMT, hStmt);
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
}

// 예약 큐 리스트 헤드
ReservationQueue* reservationQueuesHead = NULL;

// 예약 큐 함수들 (큐 추가/삭제/검색)
ReservationQueue* findOrCreateReservationQueue(const char* bookTitle) {
    ReservationQueue* curr = reservationQueuesHead;
    while (curr) {
        if (strcmp(curr->bookTitle, bookTitle) == 0)
            return curr;
        curr = curr->next;
    }

    ReservationQueue* newQueue = (ReservationQueue*)malloc(sizeof(ReservationQueue));
    strcpy(newQueue->bookTitle, bookTitle);
    newQueue->front = NULL;
    newQueue->rear = NULL;
    newQueue->next = reservationQueuesHead;
    reservationQueuesHead = newQueue;
    return newQueue;
}

int enqueueReservation(ReservationQueue* queue, const char* username) {
    ReservationNode* temp = queue->front;
    while (temp) {
        if (strcmp(temp->username, username) == 0) {
            printf("이미 예약하신 책입니다.\n");
            return 0;
        }
        temp = temp->next;
    }

    ReservationNode* node = (ReservationNode*)malloc(sizeof(ReservationNode));
    strcpy(node->username, username);
    node->next = NULL;
    if (!queue->rear) {
        queue->front = queue->rear = node;
    }
    else {
        queue->rear->next = node;
        queue->rear = node;
    }

    // 예약 DB 저장 호출
    saveReservationToDB(username, queue->bookTitle);

    printf("예약 목록에 추가되었습니다.\n");
    return 1;
}


char* peekReservation(ReservationQueue* queue) {
    return queue->front ? queue->front->username : NULL;
}

void dequeueReservation(ReservationQueue* queue) {
    if (!queue->front) return;

    ReservationNode* temp = queue->front;
    // 예약자 정보 저장해두기 (삭제 전)
    char username[50];
    strcpy(username, temp->username);

    queue->front = queue->front->next;
    if (!queue->front) queue->rear = NULL;

    // 예약 DB 삭제 호출
    deleteReservationFromDB(username, queue->bookTitle);

    free(temp);
}

//5월 23일 작성,현재 대출 현황 목록 
void printUserLoans(const char* username) {
    printf("\n[현재 대출 목록]\n");

    if (strlen(username) == 0) {
        printf("사용자 이름이 없습니다.\n");
        return;
    }

    SQLHDBC hDbc = initDB();
    SQLHSTMT hStmt;
    SQLRETURN ret;
    char query[256];

    ret = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    if (!SQL_SUCCEEDED(ret)) {
        printf("SQL 핸들 할당 실패\n");
        return;
    }

    sprintf(query, "SELECT title FROM dbo.Loans WHERE username = ?");

    ret = SQLPrepareA(hStmt, (SQLCHAR*)query, SQL_NTS);
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
        printf("SQL 실행 실패\n");
        printSQLError(SQL_HANDLE_STMT, hStmt);
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return;
    }

    SQLCHAR title[MAX_TITLE_LEN];
    SQLBindCol(hStmt, 1, SQL_C_CHAR, title, sizeof(title), NULL);

    int count = 0;
    while (SQLFetch(hStmt) == SQL_SUCCESS) {
        printf("%d. %s\n", ++count, title);
    }

    if (count == 0) {
        printf("대출한 책이 없습니다.\n");
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
}

// 작은 따옴표를 2개로 치환하는 함수
void escapeQuotes(char* src, char* dest) {
    while (*src) {
        if (*src == '\'') {
            *dest++ = '\'';
            *dest++ = '\'';  // 작은 따옴표 2개로
        }
        else {
            *dest++ = *src;
        }
        src++;
    }
    *dest = '\0';
}

int isBookBorrowedInDB(const char* title) {
    SQLHDBC hDbc = initDB();
    if (!hDbc) return 0;

    SQLHSTMT hStmt;
    if (!SQL_SUCCEEDED(SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt))) {
        SQLDisconnect(hDbc);
        SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
        return 0;
    }

    char* query = "SELECT COUNT(*) FROM dbo.Loans WHERE title = ?";
    if (!SQL_SUCCEEDED(SQLPrepareA(hStmt, (SQLCHAR*)query, SQL_NTS))) goto cleanup;

    if (!SQL_SUCCEEDED(SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 200, 0, (SQLPOINTER)title, 0, NULL))) goto cleanup;

    if (!SQL_SUCCEEDED(SQLExecute(hStmt))) goto cleanup;

    int count = 0;
    SQLBindCol(hStmt, 1, SQL_C_SLONG, &count, 0, NULL);
    SQLFetch(hStmt);

cleanup:
    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
    SQLDisconnect(hDbc);
    SQLFreeHandle(SQL_HANDLE_DBC, hDbc);

    return count > 0;
}

//5월23일, 전역변수 정의 , 연결리스트 해드 이름 저장을 위한것 
char currentUser[MAX_USER_ID] = "";
UserLoan* loanListHead = NULL;

// 연결리스트에 대출 기록 추가하는 함수 (원래 borrowBook 내 일부 로직 분리)
int addLoanToList(const char* username, const char* title, const char* author) {
    UserLoan* user = loanListHead;
    while (user && strcmp(user->username, username) != 0) {
        user = user->next;
    }
    if (!user) {
        user = (UserLoan*)malloc(sizeof(UserLoan));
        strcpy(user->username, username);
        user->borrowedHead = NULL;
        user->next = loanListHead;
        loanListHead = user;
    }

    // 중복 체크
    BorrowedBook* temp = user->borrowedHead;
    while (temp) {
        if (strcmp(temp->title, title) == 0) {
            return 0; 
        }
        temp = temp->next;
    }

    BorrowedBook* newBook = (BorrowedBook*)malloc(sizeof(BorrowedBook));
    strcpy(newBook->title, title);
    strcpy(newBook->author, author);
    newBook->next = user->borrowedHead;
    user->borrowedHead = newBook;

    return 1;
}

// 연결리스트에서 대출 기록 삭제하는 함수
void removeLoanFromList(const char* username, const char* title) {
    UserLoan* user = loanListHead;
    while (user && strcmp(user->username, username) != 0) {
        user = user->next;
    }
    if (!user) return;

    BorrowedBook* prev = NULL;
    BorrowedBook* curr = user->borrowedHead;
    while (curr) {
        if (strcmp(curr->title, title) == 0) {
            if (prev) prev->next = curr->next;
            else user->borrowedHead = curr->next;
            free(curr);
            return;
        }
        prev = curr;
        curr = curr->next;
    }
}

//대출함수
void borrowBook(SQLHDBC hDbc, const char* username) {
    if (strlen(username) == 0) {
        printf("\n로그인이 필요합니다.\n");
        return;
    }

    char answer[10];
    printf("대출 목록을 보시겠습니까? (y/n): ");
    while (getchar() != '\n');  // 입력 버퍼 비우기
    fgets(answer, sizeof(answer), stdin);
    if (answer[0] == 'y' || answer[0] == 'Y') {
        printUserLoans(username);
    }

    char keyword[100];
    printf("\n대출할 책의 제목을 입력하세요: ");
    fgets(keyword, sizeof(keyword), stdin);
    keyword[strcspn(keyword, "\n")] = '\0';

    char key[256];
    normalize(keyword, key);
    Book* results[100];
    int resultCount = 0;

    printf("\n검색 결과:\n");
    for (int i = 0; i < bookCount; i++) {
        char normTitle[MAX_TITLE_LEN];
        normalize(books[i].title, normTitle);
        if (strstr(normTitle, key)) {
            printf("%d. %s\t%s\n", resultCount + 1, books[i].title, books[i].author);
            results[resultCount++] = &books[i];
        }
    }

    if (resultCount == 0) {
        printf("검색 결과가 없습니다.\n");
        return;
    }

    int choice;
    if (resultCount == 1) {
        choice = 1;
    }
    else {
        while (1) {
            printf("대출할 책의 번호를 선택하세요 (1~%d): ", resultCount);
            if (scanf("%d", &choice) == 1 && choice >= 1 && choice <= resultCount) {
                while (getchar() != '\n');
                break;
            }
            else {
                printf("잘못된 선택입니다. 다시 입력해주세요.\n");
                while (getchar() != '\n');
            }
        }
    }

    Book* selectedBook = results[choice - 1];

    // 제목 정제용 임시 변수
    char cleanedTitle[MAX_TITLE_LEN];
    strncpy(cleanedTitle, selectedBook->title, sizeof(cleanedTitle) - 1);
    cleanedTitle[sizeof(cleanedTitle) - 1] = '\0';
    cleanTitle(cleanedTitle);  // 따옴표 및 공백 제거

    ReservationQueue* queue = findOrCreateReservationQueue(cleanedTitle);

    char* frontUser = peekReservation(queue);
    if (frontUser != NULL && strcmp(frontUser, username) != 0) {
        printf("현재 예약 1순위는 '%s'님입니다. 대출이 불가능합니다.\n", frontUser);
        return;
    }
    else if (frontUser != NULL && strcmp(frontUser, username) == 0) {
        printf("예약 1순위이므로 대출 가능합니다.\n");
        dequeueReservation(queue);
    }

    if (isBookBorrowedInDB(selectedBook->title)) {
        printf("해당 책은 이미 대출 중입니다.\n");
        return;
    }

    addLoanToList(username, cleanedTitle, selectedBook->author);

    SQLHSTMT hStmt;
    SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    if (!SQL_SUCCEEDED(ret)) {
        printf("SQL 핸들 할당 실패\n");
        removeLoanFromList(username, cleanedTitle);
        return;
    }

    char* query = "INSERT INTO dbo.Loans (username, title) VALUES (?, ?)";
    ret = SQLPrepareA(hStmt, (SQLCHAR*)query, SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        printf("SQL Prepare 실패\n");
        removeLoanFromList(username, cleanedTitle);
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return;
    }

    ret = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 50, 0, (SQLPOINTER)username, 0, NULL);
    ret |= SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_WVARCHAR, 200, 0, (SQLPOINTER)cleanedTitle, 0, NULL);

    if (!SQL_SUCCEEDED(ret)) {
        printf("SQL 파라미터 바인딩 실패\n");
        removeLoanFromList(username, cleanedTitle);
        printSQLError(SQL_HANDLE_STMT, hStmt);
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return;
    }

    ret = SQLExecute(hStmt);
    if (SQL_SUCCEEDED(ret)) {
        printf("책 '%s'을(를) 대출 완료 하였습니다.\n", cleanedTitle);
        //printf("대출 이력이 DB에 저장되었습니다.\n");
    }
    else {
        SQLWCHAR wSqlState[6] = { 0 };
        SQLINTEGER nativeError;
        SQLWCHAR wMessage[256] = { 0 };
        SQLSMALLINT msgLen;

        if (SQL_SUCCEEDED(SQLGetDiagRecW(SQL_HANDLE_STMT, hStmt, 1, wSqlState, &nativeError, wMessage, sizeof(wMessage) / sizeof(SQLWCHAR), &msgLen))) {
            //wprintf(L"SQLSTATE: %s\n", wSqlState);
            //wprintf(L"오류 메시지: %s\n", wMessage);

            if (wcscmp(wSqlState, L"23000") == 0) {
                printf("※ 이미 대출된 책입니다.\n예약하시겠습니까? (y/n): ");
                fgets(answer, sizeof(answer), stdin);
                answer[strcspn(answer, "\n")] = '\0';

                // DB 저장 실패했으니 연결리스트에서 대출 기록 삭제
                removeLoanFromList(username, cleanedTitle);

                if (answer[0] == 'y' || answer[0] == 'Y') {
                    int added = enqueueReservation(queue, username);
                    if (added == 0) {
                        printf("이미 예약하신 책입니다.\n");
                    }
                    else {
                        printf("예약이 완료되었습니다.\n");
                    }
                }
            }
        }
        else {
            printf("SQL 오류 발생 (오류 정보를 불러올 수 없음)\n");
        }

        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return;
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
}
