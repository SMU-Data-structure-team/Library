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
//�����Լ�
// ���� �α��� ����� ID, ȸ������ �����̷������ϱ� ���ؼ� 
extern char currentUser[MAX_USER_ID];

// ���� ����Ʈ ����ü ����
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

// ���� �Լ� ����
void borrowBook(SQLHDBC hDbc, const char* username);



//������ ���� ť ��� ������ ó������ ����
// ���� ť ���
typedef struct ReservationNode {
    char username[MAX_USER_ID];
    struct ReservationNode* next;
} ReservationNode;

// ���� ť ����ü
typedef struct ReservationQueue {
    char bookTitle[MAX_TITLE_LEN];
    ReservationNode* front;
    ReservationNode* rear;
    struct ReservationQueue* next;  // ���� å ���� ť ������ ���� ����Ʈ
} ReservationQueue;


#endif
