#include <cstdio>
#include "../include/model.h"

int main(int argc, char **argv) {
    model lda;
    lda.init(argc, argv);
    lda.inference();
    return 0;
}
