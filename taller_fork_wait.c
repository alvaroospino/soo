#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/times.h>

void error(char *msg) {
    perror(msg);
    exit(-1);
}

int leerNumeros(char *filename, int **vec) {
    int c, numero, totalNumeros;
    FILE *infile;

    infile = fopen(filename, "r");
    if (!infile) {
        error("Error al abrir el archivo de entrada");
    }

    fscanf(infile, "%d", &totalNumeros);

    *vec = (int *)calloc(totalNumeros, sizeof(int));
    if (!*vec) {
        error("Error en calloc al asignar memoria");
    }

    for (c = 0; c < totalNumeros; c++) {
        fscanf(infile, "%d", &numero);
        (*vec)[c] = numero;
    }

    fclose(infile);
    return c;
}

long long leerTotal(int n) {
    FILE *infile;
    long long total = 0;

    infile = fopen("out.txt", "r");
    if (!infile) {
        error("Error del padre al abrir el archivo de resultados");
    }

    for (int i = 0; i < n; i++) {
        long long sum;
        fscanf(infile, "%lld", &sum);
        total += sum;
    }
    fclose(infile);
    return total;
}


int main(int argc, char *argv[]) {
    int *vector;
    int cantidadNumeros;
    long long totalFinal;
    int numProcesos;
    struct timespec start, end;
    struct tms tms_start, tms_end;
    clock_t clock_start, clock_end;

    if (argc < 2) {
        printf("Uso: %s <numero_de_procesos>\n", argv[0]);
        return 1;
    }

    numProcesos = atoi(argv[1]);
    if (numProcesos <= 0) {
        printf("Número de procesos inválido. Usando 2 procesos por defecto.\n");
        numProcesos = 2;
    }

    cantidadNumeros = leerNumeros("input.txt", &vector);
    printf("Padre [%d]: Total de números leídos: %d\n", getpid(), cantidadNumeros);

    if (numProcesos > cantidadNumeros) {
        numProcesos = cantidadNumeros;
        printf("Número de procesos ajustado a la cantidad de números: %d\n", numProcesos);
    }

    // Medir tiempo inicio
    clock_gettime(CLOCK_MONOTONIC, &start);
    clock_start = times(&tms_start);

    // Preparar límites y procesos
    int **limites = malloc(numProcesos * sizeof(int*));
    for (int j = 0; j < numProcesos; j++) {
        limites[j] = malloc(2 * sizeof(int));
    }
    pid_t *pids = malloc(numProcesos * sizeof(pid_t));

    int chunk = cantidadNumeros / numProcesos;
    int remainder = cantidadNumeros % numProcesos;
    int start_idx = 0;
    for (int j = 0; j < numProcesos; j++) {
        int extra = (j < remainder) ? 1 : 0;
        limites[j][0] = start_idx;
        limites[j][1] = start_idx + chunk + extra;
        start_idx = limites[j][1];
    }

    remove("out.txt");

    for (int j = 0; j < numProcesos; j++) {
        printf("Padre [%d]: Hijo %d sumará del índice %d al %d\n", getpid(), j+1, limites[j][0], limites[j][1]-1);
    }

    for (int j = 0; j < numProcesos; j++) {
        pids[j] = fork();
        if (pids[j] == 0) {
            printf("Hijo %d [%d]: Iniciando suma.\n", j+1, getpid());
            long long suma = 0;
            for (int k = limites[j][0]; k < limites[j][1]; k++) {
                suma += vector[k];
            }
            FILE *outfile = fopen("out.txt", "a");
            if (!outfile) {
                error("Hijo: Error abriendo out.txt");
            }
            fprintf(outfile, "%lld\n", suma);
            fclose(outfile);
            printf("Hijo %d [%d]: Suma parcial %lld escrita en out.txt.\n", j+1, getpid(), suma);
            exit(0);
        } else if (pids[j] < 0) {
            error("Error en fork");
        }
    }

    for (int j = 0; j < numProcesos; j++) {
        wait(NULL);
    }
    printf("Padre [%d]: Todos los hijos han terminado.\n", getpid());

    totalFinal = leerTotal(numProcesos);

    // Medir tiempo fin
    clock_gettime(CLOCK_MONOTONIC, &end);
    clock_end = times(&tms_end);

    double wall_time = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
    double cpu_time = ((tms_end.tms_utime + tms_end.tms_stime) - (tms_start.tms_utime + tms_start.tms_stime)) / (double) sysconf(_SC_CLK_TCK);

    printf("\n====================================\n");
    printf("Resultado Final: La suma total es %lld\n", totalFinal);
    printf("Wall time (segundos): %.6f\n", wall_time);
    printf("CPU time (segundos): %.6f\n", cpu_time);
    printf("====================================\n");

    for (int j = 0; j < numProcesos; j++) free(limites[j]);
    free(limites);
    free(pids);
    free(vector);
    return 0;
}
