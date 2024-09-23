#include<stdio.h>

int main() {
    long size = 2*1e9 + 1e8 + 4*1e7 + 1e6;
    long sum = 0;
    for (int i = 0; i<size; ++i) {
        sum += i;
    }
    printf("sum: %ld\n", sum);
}