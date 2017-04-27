#include <cstdio>
#include "../include/model.h"

int main(int argc, char **argv) {
    model lda;

    if (lda.model_status == MODEL_STATUS_EST || lda.model_status == MODEL_STATUS_ESTC) {
        // parameter estimation
        lda.estimate();
    }

    if (lda.model_status == MODEL_STATUS_INF) {
        // do inference
        lda.inference();
    }

    return 0;
}
