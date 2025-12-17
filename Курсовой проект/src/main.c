#include "dag_scheduler.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Использование: %s <config.ini>\n", argv[0]);
        fprintf(stderr, "Пример: %s example.ini\n", argv[0]);
        return 1;
    }
    
    printf("Планировщик DAG джобов\n");
    printf("======================\n");
    
    DAG* dag = dag_create();
    if (!dag) {
        fprintf(stderr, "Ошибка создания DAG\n");
        return 1;
    }
    
    if (!parse_ini(argv[1], dag)) {
        fprintf(stderr, "Ошибка парсинга конфигурационного файла\n");
        dag_destroy(dag);
        return 1;
    }
    
    dag_execute(dag);
    
    dag_destroy(dag);
    
    return 0;
}