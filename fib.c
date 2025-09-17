#include <stdio.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Please give a number.\n");
        return 1;
    }
    int n;
    scanf("%d", &n);  
    int x = 0, y = 1, z;
    printf("Fibonacci up to %d terms:\n", n);
    for (int i = 0; i < n; i++) {
        printf("%d ", x);
        z = x + y;
        x = y;
        y = z;
    }
    printf("\n");
    return 0;
}