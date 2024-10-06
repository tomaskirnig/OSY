#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "libSta.h"

void output(int L, int N) {
    srand(time(NULL));  

    for (int i = 0; i < L; i++) {
        // int sum = 0; 

        for (int j = 0; j < N; j++) {
            int num = rand() % 1001;  // <0, 1000>

            if (j == N - 1) {
                printf("%d=\n", num);
            } else {
                char sign = (rand() % 2 == 0) ? '+' : '-';
                printf("%d%c", num, sign);
            }
            // sum += num;  
        }

        // printf("%d\n", sum);  
    }
    // for (int j = 0; j < N; j++){
    //     printf("%d ", 1);
    // }
    // printf("%d\n", N-1);
}

void outputF(int L, int N){
    srand(time(NULL));  

    for (int i = 0; i < L; i++) {
        // float sum = 0; 

        for (int j = 0; j < N; j++) {
            float num = (rand() % 1001) / 10.0;

            if (j == N - 1) {
                printf("%.2f=\n", num);
            } else {
                char sign = (rand() % 2 == 0) ? '+' : '-';
                printf("%.2f%c", num, sign);
            }
            // sum += num;  
        }

        // printf("%.2f\n", sum); 
    }
    
    // for (int j = 0; j < N; j++){
    //     printf("%.2f ", 1.0); 
    // }
    // printf("%.2f\n", (float)(N-1));
}
