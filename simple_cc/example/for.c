int main() {
    for (int i = 0; i < 10; i = i + 1) {
        for (int j = 0; j < 10; j = j + 1) {
            if (i > j) {
                printf("%d > %d\n", i, j);
            } else {
                printf("%d < %d\n", i, j);
            }
        }
    }
}