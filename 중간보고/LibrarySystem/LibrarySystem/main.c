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

char loggedInUser[11] = "";//�α��� ������//�α��� ������ ������ username�� ����

int main() { //�Լ��߰������� �� ���� ����, ������Ͽ� �Լ� �߰��صα�!!
    loadBooksFromCSV("books.csv");
    puts("<������ �ý��ۿ� �����ϼ̽��ϴ�. �̿��Ͻ� ���񽺸� �������ּ���.>");

    while (1) {
        printf( "\n1. ȸ������\n2. �α���\n3. ���� �˻�\n4. ����\n5. �ݳ�\n6. ����\n>> ");
        int choice;
        //���� �̿��� �Է½� ���
        if (scanf("%d", &choice) != 1) {
            printf("�߸� �Է��ϼ̽��ϴ�. �ٽ� �Է����ּ���.\n");
            while (getchar() != '\n')
                continue;
        }
        
        //ȸ������
        if (choice == 1) { //�ڷΰ��� ��� ���
            char username[11];
            char password[50];
            char wpassword[50];
            puts("<ȸ������>");
            puts("��������: ����� ���ڸ� ��밡��"); //���߿� �߰���

            printf("���̵� �Է�(�ִ� 10��): ");
            while (getchar() != '\n');
            scanf("%10s", username);
            if (strlen(username) > 10) {
                puts("���� ���� �ʰ��߽��ϴ�\n");
                continue;
            }
           if (!LimitAlphaNumer(username)) {
                printf("����� ���ڸ� �Է� �����մϴ�.\n");
            }

            printf("��й�ȣ �Է�: ");
            while (getchar() != '\n');
            scanf("%49s", password);
            printf("��й�ȣ Ȯ��: ");
            scanf("%49s", wpassword);
            if (!LimitAlphaNumer(password)) {
                printf("����� ���ڸ� �Է� �����մϴ�.\n");
            }
            if (!LimitAlphaNumer(wpassword)) {
                printf("����� ���ڸ� �Է� �����մϴ�.\n");
            }
            if (strcmp(password, wpassword) == 0) {
                puts("��й�ȣ�� ��ġ�մϴ�.");
            }
            else {
                puts("��й�ȣ�� ��ġ���� �ʽ��ϴ�.");
                continue;
            }
            
            //DB���� �� ȸ������ �Լ� ����
            SQLHDBC hDbc = initDB();
            if (hDbc != NULL) {
                //testQuery(hDbc); //�׽�Ʈ���� ���߿� ����
                registerUser(hDbc, username, password);
                SQLDisconnect(hDbc);
                SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
            }
            else {
                puts("�����ͺ��̽� ���� ���з� ȸ�������� ������ �� �����ϴ�.");
            }

        }
        //�α���
        else if (choice == 2) {
            char username[11];
            char password[50];
            int loginResult = 0;
            puts("<�α���>");
            printf("���̵� �Է�: ");
            scanf("%10s", username);
            if (!LimitAlphaNumer(username)) {
                printf("����� ���ڸ� �Է� �����մϴ�.\n");
            }
            printf("��й�ȣ �Է�: ");
            scanf("%49s", password);
            if (!LimitAlphaNumer(password)) {
                printf("����� ���ڸ� �Է� �����մϴ�.\n");
            }
            //printf("������ �Է� Ȯ��: username='%s', password='%s'\n", username, password);

            //DB����
            SQLHDBC hDbc = initDB();
            if (hDbc) {
                //testLoginQuery(hDbc);//���߿� ����
                loginUser(hDbc, username, password, &loginResult);
                if (loginResult == 1) { //�α��μ�����loggedInUser�� ������������
                    strcpy(loggedInUser, username);//loggedInUser�� �� ���ڿ��̸� �α��� �� �� ����, ���� ������ �α��ε� ����
                }
                SQLDisconnect(hDbc);
                SQLFreeHandle(SQL_HANDLE_DBC, hDbc);
            }
        
        }
        //���� �˻�
        else if (choice == 3) {
            while (getchar() != '\n');
            printf("<���� �˻�>");
            printf("�˻��� �Է�: ");
            char keyword[100];
            if (fgets(keyword, sizeof(keyword), stdin)) {
                keyword[strcspn(keyword, "\n")] = '\0'; 
                searchBook(keyword);
            }
           
        }
        //����
        else if (choice == 4) { 
            borrowBook(currentUser);
        }
        //�ݳ�
        else if (choice == 5) {
            break; //�ӽ�
        }
        //����
        else if (choice == 6) {
            puts("������ �ý����� �����մϴ�.");
            break;
        }
       /* else {
            printf("�߸� �Է��ϼ̽��ϴ�. �ٽ� �������ּ���");
        }*///��ħ
    }
    SQLFreeHandle(SQL_HANDLE_ENV, hEnv);
    return 0;
}
