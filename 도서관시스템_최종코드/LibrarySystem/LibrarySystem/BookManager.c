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

