#include <stdio.h>
#include <stdlib.h>

// Recursive function to calculate the nth Fibonacci number
long long fibonacci(int n) {
    if (n <= 1) {
        return n;
    }
    return fibonacci(n - 1) + fibonacci(n - 2);
}

int main(int argc, char *argv[]) {
    // Check if the user provided an argument
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <number>\n", argv[0]);
        return 1;
    }

    // Convert the argument to an integer
    int n = atoi(argv[1]);

    // Check for invalid input
    if (n < 0) {
        fprintf(stderr, "Please provide a non-negative integer.\n");
        return 1;
    }

    // Calculate the nth Fibonacci number
    long long result = fibonacci(n);

    // Print the result
    printf("Fibonacci number %d is %lld\n", n, result);

    return 0;
}	
