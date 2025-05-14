#pragma once
#ifndef BOOKMANAGER_H
#define BOOKMANAGER_H
#include <iostream>
#include <vector>
#include <string>
using namespace std;

struct Book {
    string title;
    string author;
};

void loadBooksFromCSV(const string& filename);
void searchBook(const string& keyword);

extern vector<Book> books;


#endif
