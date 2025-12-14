#include <stdio.h>
#include "mathlib.h"

int main() {
    int cmd;
    int arg1, arg2;

    while (1) {
        if (scanf("%d", &cmd) == EOF) {
            printf("Выход.\n");
            break;
        }

        if (cmd == -1) {
            printf("Выход.\n");
            break;
        }

        if (cmd == 1) {
            if (scanf("%d %d", &arg1, &arg2) != 2) {
                printf("Ошибка: для команды 1 требуется два аргумента A и B.\n");
                continue;
            }
            printf("PrimeCount(%d, %d) = %d\n", arg1, arg2, PrimeCount(arg1, arg2));
        }
        else if (cmd == 2) {
            if (scanf("%d", &arg1) != 1) {
                printf("Ошибка: для команды 2 требуется один аргумент x.\n");
                continue;
            }
            printf("E(%d) = %f\n", arg1, E(arg1));
        }
        else if (cmd == 0) {
            printf("В статической версии переключение библиотек недоступно.\n");
        }
        else {
            printf("Нет такой команды. Используйте 1, 2, 0 или -1 для выхода.\n");
        }
    }

    return 0;
}