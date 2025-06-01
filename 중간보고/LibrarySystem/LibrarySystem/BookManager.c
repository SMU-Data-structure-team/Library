#define CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include "BookManager.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

//normalize, CSV로드함수, 도서검색함수 있음

Book books[MAX_BOOKS];
int bookCount = 0;

void normalize(const char *src, char *dest)//기호빈칸등전부없애서붙이는함수
{
    int j = 0;
    for (int i = 0; src[i] != '\0'; i++) {
        unsigned char c = (unsigned char)src[i];
        if (c >= 0x80) { 
            dest[j++] = src[i++];
            if (src[i] != '\0') dest[j++] = src[i];
        }
        else if (isalnum(c)) {
            dest[j++] = tolower(c);
        }
    }
    dest[j] = '\0';
}

int LimitAlphaNumer(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isalnum((unsigned char)str[i])) {
            return 0; 
        }
    }
    return 1; 
}

void loadBooksFromCSV(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("CSV 파일 열기 실패!\n"); //CSV파일에문제가있을때
        return;
    }
    char line[1024];
    fgets(line, sizeof(line), file); // 헤더스킵

    while (fgets(line, sizeof(line), file)) {
        char* token;
        char title[MAX_TITLE_LEN] = "";
        char author[MAX_AUTHOR_LEN] = "";

        token = strtok(line, ",");  // A열스킵 필요없어서
        if (!token) continue;

        token = strtok(NULL, ","); // B열스킵
        if (!token) continue;

        token = strtok(NULL, ","); // C열
        if (token) {
            strncpy(title, token, MAX_TITLE_LEN - 1);
            title[strcspn(title, "\r\n")] = '\0'; // 줄바꿈 제거
        }

        token = strtok(NULL, ","); // D열
        if (token) {
            strncpy(author, token, MAX_AUTHOR_LEN - 1);
            author[strcspn(author, "\r\n")] = '\0'; // 줄바꿈 제거
        }

        if (strlen(title) > 0 && strlen(author) > 0 && bookCount < MAX_BOOKS) {
            strncpy(books[bookCount].title, title, MAX_TITLE_LEN);
            strncpy(books[bookCount].author, author, MAX_AUTHOR_LEN);
            bookCount++;
        }
    }
    fclose(file);
}

void searchBook(const char* keyword) {
    char key[256];
    normalize(keyword, key);
    int found = 0;

    printf("검색 결과:\n");
    for (int i = 0; i < bookCount; i++) {
        char normTitle[MAX_TITLE_LEN];
        char normAuthor[MAX_AUTHOR_LEN];
        normalize(books[i].title, normTitle);
        normalize(books[i].author, normAuthor);

        if (strstr(normTitle, key) || strstr(normAuthor, key)) {
            printf("%s\t%s\n", books[i].title, books[i].author);
            found = 1;
        }
    }
    if (!found)
        printf( "검색 결과가 없습니다.\n");
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
    // 없으면 새로 생성
    ReservationQueue* newQueue = (ReservationQueue*)malloc(sizeof(ReservationQueue));
    strcpy(newQueue->bookTitle, bookTitle);
    newQueue->front = NULL;
    newQueue->rear = NULL;
    newQueue->next = reservationQueuesHead;
    reservationQueuesHead = newQueue;
    return newQueue;
}

int enqueueReservation(ReservationQueue* queue, const char* username) {
    // 중복 예약 방지
    ReservationNode* temp = queue->front;
    while (temp) {
        if (strcmp(temp->username, username) == 0) {
            printf("이미 예약하신 책입니다.\n");
            return 0;  // 실패
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

    printf("예약 목록에 추가되었습니다.\n");
    return 1;  // 성공
}


char* peekReservation(ReservationQueue* queue) {
    return queue->front ? queue->front->username : NULL;
}

void dequeueReservation(ReservationQueue* queue) {
    if (!queue->front) return;
    ReservationNode* temp = queue->front;
    queue->front = queue->front->next;
    if (!queue->front) queue->rear = NULL;
    free(temp);
}

//5월 23일 작성,현재 대출 현황 목록 
void printUserLoans(const char* username) {
    UserLoan* user = loanListHead;
    while (user && strcmp(user->username, username) != 0) {
        user = user->next;
    }

    printf("\n[현재 대출 목록]\n");
    if (user == NULL || user->borrowedHead == NULL) {
        printf("대출한 책이 없습니다.\n");
    }
    else {
        BorrowedBook* bbook = user->borrowedHead;
        int idx = 1;
        while (bbook) {
            printf("%d. %s\t%s\n", idx++, bbook->title, bbook->author);
            bbook = bbook->next;
        }
    }
}

//여기서부터 대출 관련 함수들
char currentUser[MAX_USER_ID] = "";
UserLoan* loanListHead = NULL;

// 대출 함수, DB이용해서, 예약리스트 사라지지 않도록 저장, 지금은 구현 안되었습니다.
void borrowBook(const char* username) {

    if (strlen(username) == 0) {
        printf("\n로그인이 필요합니다.\n");
        return;
    }

    //대출 목록 열람 선택
    char answer[10];
    printf("대출 목록을 보시겠습니까? (y/n): ");
    while (getchar() != '\n');  // 입력 버퍼 정리
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
                puts("대출 되었습니다.");
                break; // 올바른 입력이면 루프 탈출
            }
            else {
                printf("잘못된 선택입니다. 다시 입력해주세요.\n");
                while (getchar() != '\n'); // 입력 버퍼 정리
            }
        }
    }

    Book* selectedBook = results[choice - 1];

    // 1. 예약 큐 찾기 또는 새 생성
    ReservationQueue* queue = findOrCreateReservationQueue(selectedBook->title);

    // 2. 예약 큐에서 대출 가능 여부 판단
    char* frontUser = peekReservation(queue);

    if (frontUser == NULL) {
        // 예약자는 없지만 대출자가 있을 수 있음 → 이 경우에도 예약 가능해야 함

        // 모든 사용자 대출 중인지 확인
        UserLoan* userCheck = loanListHead;
        while (userCheck) {
            BorrowedBook* temp = userCheck->borrowedHead;
            while (temp != NULL) {
                if (strcmp(temp->title, selectedBook->title) == 0) {
                    printf("이미 '%s'님이 대출 중인 도서입니다. 대출 불가능합니다.\n", userCheck->username);

                    // 여기서 예약 받을지 물어보자
                    char answer[10];
                    printf("예약하시겠습니까? (y/n): ");
                    fgets(answer, sizeof(answer), stdin);
                    answer[strcspn(answer, "\n")] = '\0';

                    if (answer[0] == 'y' || answer[0] == 'Y') {
                        // enqueueReservation()의 결과를 확인해 중복이면 종료
                        int added = enqueueReservation(queue, username);
                        if (added == 0) {
                            // 중복 예약이라면 대출 시도 중단
                            printf("이미 예약하신 책 입니다.");
                            return;
                        }
                        return;
                    }
                    temp = temp->next;
                }
                userCheck = userCheck->next;
            }

            printf("예약자가 없으므로 대출 가능합니다.\n");
        }
    }

    // 3. 대출자 연결 리스트 찾아서 중복 대출 검사
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
    BorrowedBook* temp = user->borrowedHead;
    while (temp != NULL) {
        if (strcmp(temp->title, selectedBook->title) == 0) {
            printf("이미 대출한 책입니다.\n");
            return;
        }
        temp = temp->next;
    }

    // 4. 대출 내역 추가
    BorrowedBook* newBook = (BorrowedBook*)malloc(sizeof(BorrowedBook));
    strcpy(newBook->title, selectedBook->title);
    strcpy(newBook->author, selectedBook->author);
    newBook->next = user->borrowedHead;
    user->borrowedHead = newBook;

    printf("책 '%s'을(를) 대출 완료 하였습니다.\n", newBook->title);
}