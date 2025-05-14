#include "BookManager.h"

int main() {
    loadBooksFromCSV("books.csv");

    while (true) {
        cout << "\n1. ���� �˻�\n2. ����\n>> ";
        int choice;
        cin >> choice;

       if (choice == 1) {
           cin.ignore(); // ���� �Է� ���� ����
           cout << "�˻��� �Է�: ";
           string keyword;
           getline(cin, keyword); // ���� ���� �Է�
           searchBook(keyword);
        }
        else if (choice == 2) break;
    
        else {
            cout << "�߸� �Է��ϼ̽��ϴ�. �ٽ� �������ּ���.\n";
            cin.clear(); // ���� �÷��� Ŭ����
            cin.ignore(1000, '\n'); // �߸��� �Է� ����
            }
    }
    return 0;
}
