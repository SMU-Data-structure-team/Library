#define CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include "BookManager.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

//normalize, CSV�ε��Լ�, �����˻��Լ� ����

Book books[MAX_BOOKS];
int bookCount = 0;

void normalize(const char *src, char *dest)//��ȣ��ĭ�����ξ��ּ����̴��Լ�
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
        printf("CSV ���� ���� ����!\n"); //CSV���Ͽ�������������
        return;
    }
    char line[1024];
    fgets(line, sizeof(line), file); // �����ŵ

    while (fgets(line, sizeof(line), file)) {
        char* token;
        char title[MAX_TITLE_LEN] = "";
        char author[MAX_AUTHOR_LEN] = "";

        token = strtok(line, ",");  // A����ŵ �ʿ���
        if (!token) continue;

        token = strtok(NULL, ","); // B����ŵ
        if (!token) continue;

        token = strtok(NULL, ","); // C��
        if (token) {
            strncpy(title, token, MAX_TITLE_LEN - 1);
            title[strcspn(title, "\r\n")] = '\0'; // �ٹٲ� ����
        }

        token = strtok(NULL, ","); // D��
        if (token) {
            strncpy(author, token, MAX_AUTHOR_LEN - 1);
            author[strcspn(author, "\r\n")] = '\0'; // �ٹٲ� ����
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

    printf("�˻� ���:\n");
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
        printf( "�˻� ����� �����ϴ�.\n");
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
    // ������ ���� ����
    ReservationQueue* newQueue = (ReservationQueue*)malloc(sizeof(ReservationQueue));
    strcpy(newQueue->bookTitle, bookTitle);
    newQueue->front = NULL;
    newQueue->rear = NULL;
    newQueue->next = reservationQueuesHead;
    reservationQueuesHead = newQueue;
    return newQueue;
}

int enqueueReservation(ReservationQueue* queue, const char* username) {
    // �ߺ� ���� ����
    ReservationNode* temp = queue->front;
    while (temp) {
        if (strcmp(temp->username, username) == 0) {
            printf("�̹� �����Ͻ� å�Դϴ�.\n");
            return 0;  // ����
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

    printf("���� ��Ͽ� �߰��Ǿ����ϴ�.\n");
    return 1;  // ����
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

//5�� 23�� �ۼ�,���� ���� ��Ȳ ��� 
void printUserLoans(const char* username) {
    UserLoan* user = loanListHead;
    while (user && strcmp(user->username, username) != 0) {
        user = user->next;
    }

    printf("\n[���� ���� ���]\n");
    if (user == NULL || user->borrowedHead == NULL) {
        printf("������ å�� �����ϴ�.\n");
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

//���⼭���� ���� ���� �Լ���
char currentUser[MAX_USER_ID] = "";
UserLoan* loanListHead = NULL;

// ���� �Լ�, DB�̿��ؼ�, ���ฮ��Ʈ ������� �ʵ��� ����, ������ ���� �ȵǾ����ϴ�.
void borrowBook(const char* username) {

    if (strlen(username) == 0) {
        printf("\n�α����� �ʿ��մϴ�.\n");
        return;
    }

    //���� ��� ���� ����
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
                puts("���� �Ǿ����ϴ�.");
                break; // �ùٸ� �Է��̸� ���� Ż��
            }
            else {
                printf("�߸��� �����Դϴ�. �ٽ� �Է����ּ���.\n");
                while (getchar() != '\n'); // �Է� ���� ����
            }
        }
    }

    Book* selectedBook = results[choice - 1];

    // 1. ���� ť ã�� �Ǵ� �� ����
    ReservationQueue* queue = findOrCreateReservationQueue(selectedBook->title);

    // 2. ���� ť���� ���� ���� ���� �Ǵ�
    char* frontUser = peekReservation(queue);

    if (frontUser == NULL) {
        // �����ڴ� ������ �����ڰ� ���� �� ���� �� �� ��쿡�� ���� �����ؾ� ��

        // ��� ����� ���� ������ Ȯ��
        UserLoan* userCheck = loanListHead;
        while (userCheck) {
            BorrowedBook* temp = userCheck->borrowedHead;
            while (temp != NULL) {
                if (strcmp(temp->title, selectedBook->title) == 0) {
                    printf("�̹� '%s'���� ���� ���� �����Դϴ�. ���� �Ұ����մϴ�.\n", userCheck->username);

                    // ���⼭ ���� ������ �����
                    char answer[10];
                    printf("�����Ͻðڽ��ϱ�? (y/n): ");
                    fgets(answer, sizeof(answer), stdin);
                    answer[strcspn(answer, "\n")] = '\0';

                    if (answer[0] == 'y' || answer[0] == 'Y') {
                        // enqueueReservation()�� ����� Ȯ���� �ߺ��̸� ����
                        int added = enqueueReservation(queue, username);
                        if (added == 0) {
                            // �ߺ� �����̶�� ���� �õ� �ߴ�
                            printf("�̹� �����Ͻ� å �Դϴ�.");
                            return;
                        }
                        return;
                    }
                    temp = temp->next;
                }
                userCheck = userCheck->next;
            }

            printf("�����ڰ� �����Ƿ� ���� �����մϴ�.\n");
        }
    }

    // 3. ������ ���� ����Ʈ ã�Ƽ� �ߺ� ���� �˻�
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
            printf("�̹� ������ å�Դϴ�.\n");
            return;
        }
        temp = temp->next;
    }

    // 4. ���� ���� �߰�
    BorrowedBook* newBook = (BorrowedBook*)malloc(sizeof(BorrowedBook));
    strcpy(newBook->title, selectedBook->title);
    strcpy(newBook->author, selectedBook->author);
    newBook->next = user->borrowedHead;
    user->borrowedHead = newBook;

    printf("å '%s'��(��) ���� �Ϸ� �Ͽ����ϴ�.\n", newBook->title);
}