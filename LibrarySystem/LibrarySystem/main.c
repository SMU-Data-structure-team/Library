#define CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h> 
#include <ctype.h>
#include "BookManager.h"

void loadBooksFromCSV(const char* filename);
void searchBook(const char* keyword);
void printSQLError(SQLSMALLINT handleType, SQLHANDLE handle);
SQLHDBC initDB();
void testQuery(SQLHDBC hDbc);
void registerUser(SQLHDBC hDbc, const char* username, const char* password);
SQLHENV hEnv;
void loginUser(SQLHDBC hDbc, const char* username, const char* password, int* result);

char loggedInUser[11] = "";//로그인 유지용//로그인 성공시 변수에 username을 저장

int main() { //함수추가했으면 꼭 파일 맨위, 헤더파일에 함수 추가해두기!!
    loadBooksFromCSV("books.csv");
    puts("<도서관 시스템에 접속하셨습니다. 이용하실 서비스를 선택해주세요.>");

    while (1) {
        printf( "\n1. 회원가입\n2. 로그인\n3. 도서 검색\n4. 대출\n5. 반납\n6. 종료\n>> ");
        int choice;
        //정수 이외의 입력시 경고
        if (scanf("%d", &choice) != 1) {
            printf("잘못 입력하셨습니다. 다시 입력해주세요.\n");
            while (getchar() != '\n')
                continue;
        }
        
        //회원가입
        if (choice == 1) { //뒤로가기 기능 고민
            char username[11];
            char password[50];
            char wpassword[50];
            puts("<회원가입>");
            puts("글자제한: 영어와 숫자만 사용가능"); //나중에 추가함

            printf("아이디 입력(최대 10자): ");
            while (getchar() != '\n');
            scanf("%10s", username);
            if (strlen(username) > 10) {
                puts("글자 수를 초과했습니다\n");
                continue;
            }
           if (!LimitAlphaNumer(username)) {
                printf("영어와 숫자만 입력 가능합니다.\n");
            }

            printf("비밀번호 입력: ");
            while (getchar() != '\n');
            scanf("%49s", password);
            printf("비밀번호 확인: ");
            scanf("%49s", wpassword);
            if (!LimitAlphaNumer(password)) {
                printf("영어와 숫자만 입력 가능합니다.\n");
            }
            if (!LimitAlphaNumer(wpassword)) {
                printf("영어와 숫자만 입력 가능합니다.\n");
            }
            if (strcmp(password, wpassword) == 0) {
                puts("비밀번호가 일치합니다.");
            }
            else {
                puts("비밀번호가 일치하지 않습니다.");
                continue;
            }
            
            //DB연결 및 회원가입 함수 실행
            SQLHDBC hDbc = initDB();
            if (hDbc != NULL) {
                //testQuery(hDbc); //테스트쿼리 나중에 삭제
                registerUser(hDbc, username, password);
                SQLDisconnect(hDbc);
                SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
            }
            else {
                puts("데이터베이스 연결 실패로 회원가입을 진행할 수 없습니다.");
            }

        }
        //로그인
        else if (choice == 2) {
            char username[11];
            char password[50];
            int loginResult = 0;
            puts("<로그인>");
            printf("아이디 입력: ");
            scanf("%10s", username);
            if (!LimitAlphaNumer(username)) {
                printf("영어와 숫자만 입력 가능합니다.\n");
            }
            printf("비밀번호 입력: ");
            scanf("%49s", password);
            if (!LimitAlphaNumer(password)) {
                printf("영어와 숫자만 입력 가능합니다.\n");
            }
            //printf("디버깅용 입력 확인: username='%s', password='%s'\n", username, password);

            //DB연결
            SQLHDBC hDbc = initDB();
            if (hDbc) {
                //testLoginQuery(hDbc);//나중에 삭제
                loginUser(hDbc, username, password, &loginResult);
                if (loginResult == 1) { //로그인성공시loggedInUser에 유저네임저장
                    strcpy(loggedInUser, username);//loggedInUser가 빈 문자열이면 로그인 안 된 상태, 값이 있으면 로그인된 상태
                }
                SQLDisconnect(hDbc);
                SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
            }
        
        }
        //도서 검색
        else if (choice == 3) {
            while (getchar() != '\n');
            printf("<도서 검색>");
            printf("검색어 입력: ");
            char keyword[100];
            if (fgets(keyword, sizeof(keyword), stdin)) {
                keyword[strcspn(keyword, "\n")] = '\0'; 
                searchBook(keyword);
            }
           
        }
        //대출
        else if (choice == 4) { 
            borrowBook(currentUser);
        }
        //반납
        else if (choice == 5) {
            break; //임시
        }
        //종료
        else if (choice == 6) {
            puts("도서관 시스템을 종료합니다.");
            break;
        }
       /* else {
            printf("잘못 입력하셨습니다. 다시 선택해주세요");
        }*///겹침
    }
    SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
    return 0;
}
