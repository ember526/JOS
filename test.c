#include <iostream>
#include <string.h>
using namespace std;
//class Book {
//public:
//    Book(char * t = " ") {strcpy(title, t);}
//    virtual char * Category()const {};
//    char * category()const;
//
//private:
//    char title[40];
//};
//
//class Novel : public Book {
//public:
//    Novel(char * t = " ") : Book(t) { }
//    char * Category()const {return "文学";}
//};

class Novel {
public:
    int member;
    Novel(int d = 15) : member(d) { }
    //Novel(const Novel &n) : member(n.member) { }
};

int main() {
    Novel b(150);
    cout << "b : " << b.member << endl;
    Novel b_new = b;
    cout << "b_new : " << b_new.member << endl;
    return 0;
}