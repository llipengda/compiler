int main() {
    int count = 0;
    int sum = 0;

    printf("Prime numbers between 1 and 100:\n");

    for (int i = 2; i <= 100; i = i + 1) {
        int is_prime = 1;
        int j = 2;

        while (j * j <= i && is_prime) {
            if ((i / j) * j == i) {
                is_prime = 0;
            }
            j = j + 1;
        }

        if (is_prime) {
            printf("%d ", i);
            count = count + 1;
            sum = sum + i;
        }
    }

    printf("\nTotal prime count: %d\n", count);
    printf("Sum of all primes: %d\n", sum);
}
