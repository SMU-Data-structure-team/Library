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
    cleanTitle(cleanedTitle);  // ���ڿ� ����

    SQLHDBC hDbc = initDB();
    if (hDbc == NULL) {
        printf("DB ���� ����\n");
        return;
    }

    SQLHSTMT hStmt;
    SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    if (!SQL_SUCCEEDED(ret)) {
        printf("SQL �ڵ� �Ҵ� ����\n");
        return;
    }

    const char* query = "INSERT INTO dbo.Reservations (username, bookTitle) VALUES (?, ?)";
    ret = SQLPrepareA(hStmt, (SQLCHAR*)query, SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        printf("SQL Prepare ����\n");
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
        printf("SQL �Ķ���� ���ε� ����\n");
        printSQLError(SQL_HANDLE_STMT, hStmt);
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return;
    }

    ret = SQLExecute(hStmt);
    if (SQL_SUCCEEDED(ret)) {
        //printf("������ ����Ǿ����ϴ�.\n");
    }
    else {
        printf("���� ���� ����: �ߺ� �����̰ų� ��Ÿ �����Դϴ�.\n");
        printSQLError(SQL_HANDLE_STMT, hStmt);
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
}

void deleteReservationFromDB(const char* username, const char* bookTitle) {
    char cleanedTitle[MAX_TITLE_LEN];
    strncpy(cleanedTitle, bookTitle, MAX_TITLE_LEN - 1);
    cleanedTitle[MAX_TITLE_LEN - 1] = '\0';

    cleanTitle(cleanedTitle);  // �ֵ���ǥ, �¿� ���� �� ����

    SQLHDBC hDbc = initDB();
    if (hDbc == NULL) {
        printf("DB ���� ����\n");
        return;
    }

    SQLHSTMT hStmt;
    SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    if (!SQL_SUCCEEDED(ret)) {
        printf("SQL �ڵ� �Ҵ� ����\n");
        return;
    }

    const char* query = "DELETE FROM dbo.Reservations WHERE username = ? AND bookTitle COLLATE Korean_Wansung_CI_AS = ?";

    ret = SQLPrepareA(hStmt, (SQLCHAR*)query, SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        printf("SQL Prepare ����\n");
        printSQLError(SQL_HANDLE_STMT, hStmt);
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return;
    }

    SQLLEN usernameLen = (SQLLEN)strlen(username);
    ret = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, usernameLen, 0, (SQLPOINTER)username, 0, &usernameLen);
    if (!SQL_SUCCEEDED(ret)) {
        printf("username ���ε� ����\n");
        printSQLError(SQL_HANDLE_STMT, hStmt);
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return;
    }

    wchar_t wBookTitle[256];
    MultiByteToWideChar(CP_ACP, 0, cleanedTitle, -1, wBookTitle, sizeof(wBookTitle) / sizeof(wchar_t));
    SQLLEN titleLen = (SQLLEN)(wcslen(wBookTitle) * sizeof(wchar_t));
    ret = SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, titleLen, 0, (SQLPOINTER)wBookTitle, 0, &titleLen);
    if (!SQL_SUCCEEDED(ret)) {
        printf("bookTitle ���ε� ����\n");
        printSQLError(SQL_HANDLE_STMT, hStmt);
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return;
    }

    ret = SQLExecute(hStmt);
    if (SQL_SUCCEEDED(ret)) {
        printf("������ �����Ǿ����ϴ�.\n");
    }
    else {
        printf("���� ���� ����\n");
        //printSQLError(SQL_HANDLE_STMT, hStmt);
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
}

// ���� ť ����Ʈ ���
ReservationQueue* reservationQueuesHead = NULL;

// ���� ť �Լ��� (ť �߰�/����/�˻�)
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
            printf("�̹� �����Ͻ� å�Դϴ�.\n");
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

    // ���� DB ���� ȣ��
    saveReservationToDB(username, queue->bookTitle);

    printf("���� ��Ͽ� �߰��Ǿ����ϴ�.\n");
    return 1;
}


char* peekReservation(ReservationQueue* queue) {
    return queue->front ? queue->front->username : NULL;
}

void dequeueReservation(ReservationQueue* queue) {
    if (!queue->front) return;

    ReservationNode* temp = queue->front;
    // ������ ���� �����صα� (���� ��)
    char username[50];
    strcpy(username, temp->username);

    queue->front = queue->front->next;
    if (!queue->front) queue->rear = NULL;

    // ���� DB ���� ȣ��
    deleteReservationFromDB(username, queue->bookTitle);

    free(temp);
}

//5�� 23�� �ۼ�,���� ���� ��Ȳ ��� 
void printUserLoans(const char* username) {
    printf("\n[���� ���� ���]\n");

    if (strlen(username) == 0) {
        printf("����� �̸��� �����ϴ�.\n");
        return;
    }

    SQLHDBC hDbc = initDB();
    SQLHSTMT hStmt;
    SQLRETURN ret;
    char query[256];

    ret = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    if (!SQL_SUCCEEDED(ret)) {
        printf("SQL �ڵ� �Ҵ� ����\n");
        return;
    }

    sprintf(query, "SELECT title FROM dbo.Loans WHERE username = ?");

    ret = SQLPrepareA(hStmt, (SQLCHAR*)query, SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        printf("SQL Prepare ����\n");
        printSQLError(SQL_HANDLE_STMT, hStmt);
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return;
    }

    ret = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 50, 0, (SQLPOINTER)username, 0, NULL);
    if (!SQL_SUCCEEDED(ret)) {
        printf("SQL �Ķ���� ���ε� ����\n");
        printSQLError(SQL_HANDLE_STMT, hStmt);
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return;
    }

    ret = SQLExecute(hStmt);
    if (!SQL_SUCCEEDED(ret)) {
        printf("SQL ���� ����\n");
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
        printf("������ å�� �����ϴ�.\n");
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
}

// ���� ����ǥ�� 2���� ġȯ�ϴ� �Լ�
void escapeQuotes(char* src, char* dest) {
    while (*src) {
        if (*src == '\'') {
            *dest++ = '\'';
            *dest++ = '\'';  // ���� ����ǥ 2����
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

//5��23��, �������� ���� , ���Ḯ��Ʈ �ص� �̸� ������ ���Ѱ� 
char currentUser[MAX_USER_ID] = "";
UserLoan* loanListHead = NULL;

// ���Ḯ��Ʈ�� ���� ��� �߰��ϴ� �Լ� (���� borrowBook �� �Ϻ� ���� �и�)
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

    // �ߺ� üũ
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

// ���Ḯ��Ʈ���� ���� ��� �����ϴ� �Լ�
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

//�����Լ�
void borrowBook(SQLHDBC hDbc, const char* username) {
    if (strlen(username) == 0) {
        printf("\n�α����� �ʿ��մϴ�.\n");
        return;
    }

    char answer[10];
    printf("���� ����� ���ðڽ��ϱ�? (y/n): ");
    while (getchar() != '\n');  // �Է� ���� ����
    fgets(answer, sizeof(answer), stdin);
    if (answer[0] == 'y' || answer[0] == 'Y') {
        printUserLoans(username);
    }

    char keyword[100];
    printf("\n������ å�� ������ �Է��ϼ���: ");
    fgets(keyword, sizeof(keyword), stdin);
    keyword[strcspn(keyword, "\n")] = '\0';

    char key[256];
    normalize(keyword, key);
    Book* results[100];
    int resultCount = 0;

    printf("\n�˻� ���:\n");
    for (int i = 0; i < bookCount; i++) {
        char normTitle[MAX_TITLE_LEN];
        normalize(books[i].title, normTitle);
        if (strstr(normTitle, key)) {
            printf("%d. %s\t%s\n", resultCount + 1, books[i].title, books[i].author);
            results[resultCount++] = &books[i];
        }
    }

    if (resultCount == 0) {
        printf("�˻� ����� �����ϴ�.\n");
        return;
    }

    int choice;
    if (resultCount == 1) {
        choice = 1;
    }
    else {
        while (1) {
            printf("������ å�� ��ȣ�� �����ϼ��� (1~%d): ", resultCount);
            if (scanf("%d", &choice) == 1 && choice >= 1 && choice <= resultCount) {
                while (getchar() != '\n');
                break;
            }
            else {
                printf("�߸��� �����Դϴ�. �ٽ� �Է����ּ���.\n");
                while (getchar() != '\n');
            }
        }
    }

    Book* selectedBook = results[choice - 1];

    // ���� ������ �ӽ� ����
    char cleanedTitle[MAX_TITLE_LEN];
    strncpy(cleanedTitle, selectedBook->title, sizeof(cleanedTitle) - 1);
    cleanedTitle[sizeof(cleanedTitle) - 1] = '\0';
    cleanTitle(cleanedTitle);  // ����ǥ �� ���� ����

    ReservationQueue* queue = findOrCreateReservationQueue(cleanedTitle);

    char* frontUser = peekReservation(queue);
    if (frontUser != NULL && strcmp(frontUser, username) != 0) {
        printf("���� ���� 1������ '%s'���Դϴ�. ������ �Ұ����մϴ�.\n", frontUser);
        return;
    }
    else if (frontUser != NULL && strcmp(frontUser, username) == 0) {
        printf("���� 1�����̹Ƿ� ���� �����մϴ�.\n");
        dequeueReservation(queue);
    }

    if (isBookBorrowedInDB(selectedBook->title)) {
        printf("�ش� å�� �̹� ���� ���Դϴ�.\n");
        return;
    }

    addLoanToList(username, cleanedTitle, selectedBook->author);

    SQLHSTMT hStmt;
    SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, hDbc, &hStmt);
    if (!SQL_SUCCEEDED(ret)) {
        printf("SQL �ڵ� �Ҵ� ����\n");
        removeLoanFromList(username, cleanedTitle);
        return;
    }

    char* query = "INSERT INTO dbo.Loans (username, title) VALUES (?, ?)";
    ret = SQLPrepareA(hStmt, (SQLCHAR*)query, SQL_NTS);
    if (!SQL_SUCCEEDED(ret)) {
        printf("SQL Prepare ����\n");
        removeLoanFromList(username, cleanedTitle);
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return;
    }

    ret = SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_VARCHAR, 50, 0, (SQLPOINTER)username, 0, NULL);
    ret |= SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_CHAR, SQL_WVARCHAR, 200, 0, (SQLPOINTER)cleanedTitle, 0, NULL);

    if (!SQL_SUCCEEDED(ret)) {
        printf("SQL �Ķ���� ���ε� ����\n");
        removeLoanFromList(username, cleanedTitle);
        printSQLError(SQL_HANDLE_STMT, hStmt);
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return;
    }

    ret = SQLExecute(hStmt);
    if (SQL_SUCCEEDED(ret)) {
        printf("å '%s'��(��) ���� �Ϸ� �Ͽ����ϴ�.\n", cleanedTitle);
        //printf("���� �̷��� DB�� ����Ǿ����ϴ�.\n");
    }
    else {
        SQLWCHAR wSqlState[6] = { 0 };
        SQLINTEGER nativeError;
        SQLWCHAR wMessage[256] = { 0 };
        SQLSMALLINT msgLen;

        if (SQL_SUCCEEDED(SQLGetDiagRecW(SQL_HANDLE_STMT, hStmt, 1, wSqlState, &nativeError, wMessage, sizeof(wMessage) / sizeof(SQLWCHAR), &msgLen))) {
            //wprintf(L"SQLSTATE: %s\n", wSqlState);
            //wprintf(L"���� �޽���: %s\n", wMessage);

            if (wcscmp(wSqlState, L"23000") == 0) {
                printf("�� �̹� ����� å�Դϴ�.\n�����Ͻðڽ��ϱ�? (y/n): ");
                fgets(answer, sizeof(answer), stdin);
                answer[strcspn(answer, "\n")] = '\0';

                // DB ���� ���������� ���Ḯ��Ʈ���� ���� ��� ����
                removeLoanFromList(username, cleanedTitle);

                if (answer[0] == 'y' || answer[0] == 'Y') {
                    int added = enqueueReservation(queue, username);
                    if (added == 0) {
                        printf("�̹� �����Ͻ� å�Դϴ�.\n");
                    }
                    else {
                        printf("������ �Ϸ�Ǿ����ϴ�.\n");
                    }
                }
            }
        }
        else {
            printf("SQL ���� �߻� (���� ������ �ҷ��� �� ����)\n");
        }

        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return;
    }

    SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
}
