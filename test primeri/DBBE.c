#include <stdio.h>

int main() {
    printf("Reachable block\n");

    // Dead block: ovaj labela se nikada ne koristi, blok je mrtav
    goto skip;

    dead_block:
        printf("Unreachable block\n");

    skip:
    return 0;
}