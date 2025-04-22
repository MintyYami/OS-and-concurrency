#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

void * withdrawFunds();
void * addFunds();

int balance = 100;
pthread_t pWithdrawFunds1, pWithdrawFunds2, pAddFunds;

int main() {
    pthread_create((&pWithdrawFunds1), NULL, withdrawFunds, NULL);
    pthread_create((&pWithdrawFunds2), NULL, withdrawFunds, NULL);
    pthread_create(&pAddFunds, NULL, addFunds, NULL);

    pthread_join(pWithdrawFunds1, NULL);
    pthread_join(pWithdrawFunds2, NULL);
    pthread_join(pAddFunds, NULL);

    return 0;
}

void * withdrawFunds(void * p) {
    while(1) {
        if(balance >= 10) {
            balance -= 10;
            printf("Withdrawn £10. New balance: £%d\n", balance);
        } else if (balance >= 0 ){
            printf("Not enough funds to withdraw. Current balance: £%d\n", balance);
        } else {
            printf("*Balance is negative! Current balance: £%d\n", balance);
            break;
        }
    }
}
void * addFunds(void * p) {
    while(1) {
        if(balance >= 0) {
            balance += 1;
            printf("Add £1. New balance: £%d\n", balance);
        } else {
            printf("*Balance is negative! Current balance: £%d\n", balance);
            break;
        }
    }
}