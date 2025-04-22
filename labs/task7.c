#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

void * withdrawFunds();
void * addFunds();

int balance = 100;
pthread_t pWithdrawFunds1, pWithdrawFunds2, pAddFunds;
pthread_mutex_t lock;

int main() {
    pthread_mutex_init(&lock,NULL);

    pthread_create((&pWithdrawFunds1), NULL, withdrawFunds, NULL);
    pthread_create((&pWithdrawFunds2), NULL, withdrawFunds, NULL);
    pthread_create(&pAddFunds, NULL, addFunds, NULL);

    sleep(7);

    pthread_mutex_destroy(&lock);
    return 0;
}

void * withdrawFunds(void * p) {
    while(1) {
        pthread_mutex_lock(&lock);
        if(balance >= 10) {
            balance -= 10;
            printf("Withdrawn £10. New balance: £%d\n", balance);
        } else if (balance >= 0 ){
            printf("Not enough funds to withdraw. Current balance: £%d\n", balance);
        } else {
            printf("*Balance is negative! Current balance: £%d\n", balance);
        }
        pthread_mutex_unlock(&lock);
    }
}
void * addFunds(void * p) {
    while(1) {
        pthread_mutex_lock(&lock);
        if(balance >= 0) {
            balance += 1;
            printf("Add £1. New balance: £%d\n", balance);
        } else {
            printf("*Balance is negative! Current balance: £%d\n", balance);
        }
        pthread_mutex_unlock(&lock);
    }
}