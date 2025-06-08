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

