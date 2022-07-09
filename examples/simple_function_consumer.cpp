#include <iostream>

using namespace std;

extern "C" {
    int add();
}

int main() {
    cout << "The sum from the external function is " << add() << endl;
}