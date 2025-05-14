#include "BookManager.h"
#include <fstream>
#include <sstream>
#include <algorithm> // transform
#include <cctype> // isalnum 사용을 위해


vector<Book> books;

string normalize(const string& s) 
{
    string result;
    for (unsigned char c : s) { 
        if (isalnum(c))
            result += tolower(c);
    }
    return result;
}

void loadBooksFromCSV(const string& filename) {
    ifstream file(filename);
    if (!file.is_open()) {
        cout << "CSV 파일 열기 실패!" << endl;
        return;
    }
    string line;
    getline(file, line); // skip header

    while (getline(file, line)) {
        stringstream ss(line);
        string token;
        getline(ss, token, ','); // A열스킵 필요없는 정보라
        getline(ss, token, ',');  // B열스킵

        string title, author;
        
        getline(ss, title, ','); // C
        getline(ss, author, ',');// D

        books.push_back({ title, author });
    }
    file.close();
}



void searchBook(const string& keyword) {

    bool found = false;
    string key = normalize(keyword);

    cout << "검색 결과:\n";
    for (const auto& b : books) {
        if (normalize(b.title).find(key) != string::npos ||
            normalize(b.author).find(key) != string::npos) {
            cout << b.title << "\t" << b.author << "\n";
        found = true;
        }
    }
    if (!found)
        cout << "검색 결과가 없습니다.\n";
}
