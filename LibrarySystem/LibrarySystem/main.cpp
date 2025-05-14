#include "BookManager.h"

int main() {
    loadBooksFromCSV("books.csv");

    while (true) {
        cout << "\n1. 도서 검색\n2. 종료\n>> ";
        int choice;
        cin >> choice;

       if (choice == 1) {
           cin.ignore(); // 이전 입력 버퍼 비우기
           cout << "검색어 입력: ";
           string keyword;
           getline(cin, keyword); // 공백 포함 입력
           searchBook(keyword);
        }
        else if (choice == 2) break;
    
        else {
            cout << "잘못 입력하셨습니다. 다시 선택해주세요.\n";
            cin.clear(); // 오류 플래그 클리어
            cin.ignore(1000, '\n'); // 잘못된 입력 제거
            }
    }
    return 0;
}
