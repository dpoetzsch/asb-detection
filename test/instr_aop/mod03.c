#include <unistd.h>
#include <fcntl.h>

void f(int a, int b) {
    int d = a%b;
    ssize_t nwritten = write(STDOUT_FILENO, &d, sizeof(d));
}

int main() {
    int x = 4;
    int y = 5;
    int* yp = &y;
    f(x, (int) yp);
    return 0;
}
