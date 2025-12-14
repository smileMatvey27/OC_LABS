#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include "mathlib.h"

int main() {
    char *libs[] = {"./lib1.so", "./lib2.so"};
    int cur = 0; 

    void *handle = dlopen(libs[cur], RTLD_LAZY);
    if (!handle) {
        printf("Ошибка загрузки библиотеки: %s\n", dlerror());
        return 1;
    }

    int (*PrimeCountFunc)(int, int) = dlsym(handle, "PrimeCount");
    float (*EFunc)(int) = dlsym(handle, "E");

    if (!PrimeCountFunc || !EFunc) {
        printf("Ошибка загрузки символов: %s\n", dlerror());
        dlclose(handle);
        return 1;
    }

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

        if (cmd == 0) {
            dlclose(handle);
            cur = 1 - cur; 

            handle = dlopen(libs[cur], RTLD_LAZY);
            if (!handle) {
                printf("Ошибка загрузки библиотеки: %s\n", dlerror());
                return 1;
            }

            PrimeCountFunc = dlsym(handle, "PrimeCount");
            EFunc = dlsym(handle, "E");

            if (!PrimeCountFunc || !EFunc) {
                printf("Ошибка загрузки символов: %s\n", dlerror());
                dlclose(handle);
                return 1;
            }

            printf("Библиотека переключена на: %s\n", libs[cur]);
        }
        else if (cmd == 1) {
            if (scanf("%d %d", &arg1, &arg2) != 2) {
                printf("Ошибка: для команды 1 требуется два аргумента A и B.\n");
                continue;
            }
            printf("PrimeCount(%d, %d) = %d\n", arg1, arg2, PrimeCountFunc(arg1, arg2));
        }
        else if (cmd == 2) {
            if (scanf("%d", &arg1) != 1) {
                printf("Ошибка: для команды 2 требуется один аргумент x.\n");
                continue;
            }
            printf("E(%d) = %f\n", arg1, EFunc(arg1));
        }
        else {
            printf("Нет такой команды. Используйте 1, 2, 0 или -1 для выхода.\n");
        }
    }

    dlclose(handle);
    return 0;
}