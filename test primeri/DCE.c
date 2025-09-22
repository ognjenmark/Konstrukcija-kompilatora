#include <stdio.h>

int main() {
    int a = 5;
    int b = 10;
    int c = a + b;

    int d = 20;  // ovo je mrtav kod jer 'd' se nigde ne koristi

    if (c > 10) {
        printf( "c je vece od 10\n");
    }

    return 0;
}