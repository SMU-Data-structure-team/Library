#define CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#ifndef BOOKMANAGER_H
#define BOOKMANAGER_H
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>


#define MAX_BOOKS 1000
#define MAX_TITLE_LEN 100
#define MAX_AUTHOR_LEN 100
#define MAX_USER_ID 11

typedef struct {
    char title[MAX_TITLE_LEN];
    char author[MAX_AUTHOR_LEN];
} Book;

int LimitAlphaNumer(const char* str);
void loadBooksFromCSV(const char* filename);
void searchBook(const char* keyword);

extern Book books[MAX_BOOKS];
extern int bookCount;
void printSQLError(SQLSMALLINT handleType, SQLHANDLE handle);
void registerUser(SQLHDBC hDbc, const char* username, const char* password);
SQLHDBC initDB();
void testQuery(SQLHDBC hDbc);
extern SQLHENV hEnv;
void PrintError(SQLHANDLE handle, SQLSMALLINT handleType, const char* msg);
void testLoginQuery(SQLHDBC hDbc);
void loginUser(SQLHDBC hDbc, const char* username, const char* password, int* result);

#define MAX_USER_ID 20
//대출함수
// 전역 로그인 사용자 ID, 회원마다 대출이력저장하기 위해서 
extern char currentUser[MAX_USER_ID];

// 연결 리스트 구조체 선언
typedef struct BorrowedBook {
    wchar_t title[MAX_TITLE_LEN];
    wchar_t author[MAX_AUTHOR_LEN];
    struct BorrowedBook* next;
} BorrowedBook;

typedef struct UserLoan {
    char username[MAX_USER_ID];
    BorrowedBook* borrowedHead;
    struct UserLoan* next;
} UserLoan;

extern UserLoan* loanListHead;

// 대출 함수 선언
void borrowBook(SQLHDBC hDbc, const char* username);



//예약을 위한 큐 노드 생성과 처리등의 과정
// 예약 큐 노드
typedef struct ReservationNode {
    char username[MAX_USER_ID];
    struct ReservationNode* next;
} ReservationNode;

// 예약 큐 구조체
typedef struct ReservationQueue {
    char bookTitle[MAX_TITLE_LEN];
    ReservationNode* front;
    ReservationNode* rear;
    struct ReservationQueue* next;  // 여러 책 예약 큐 관리용 연결 리스트
} ReservationQueue;


#endif
