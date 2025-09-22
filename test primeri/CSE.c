#include <stdio.h>

int main() {
    int a = 5, b = 7;

    int x = a;      // load 1
    int y = a;      // load 2 (može se optimizovati)
    int z = b;      // load 3
    b = 10;         // store 1
    int w = b;      // load 4 (ne može se optimizovati sa load 3 zbog store)
    int u = x + y;  // binarna operacija (add)
    int v = y + x;  // binarna operacija (add, komutativna sa prethodnom, može se optimizovati)

    printf("%d\n", u + v + z + w);

    return 0;
}
