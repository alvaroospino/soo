#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

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
    int n = 2;
    if (argc > 1) {
        n = atoi(argv[1]);
        if (n <= 0) n = 2;
    }
    int **limites = malloc(n * sizeof(int*));
    for (int i = 0; i < n; i++) {
        limites[i] = malloc(2 * sizeof(int));
    }
    pid_t *pids = malloc(n * sizeof(pid_t));

    remove("out.txt");

    cantidadNumeros = leerNumeros("input.txt", &vector);
    printf("Padre [%d]: Total de números leídos: %d\n", getpid(), cantidadNumeros);

    int chunk = cantidadNumeros / n;
    int remainder = cantidadNumeros % n;
    int start = 0;
    for (int i = 0; i < n; i++) {
        int extra = (i < remainder) ? 1 : 0;
        limites[i][0] = start;
        limites[i][1] = start + chunk + extra;
        start = limites[i][1];
    }

    for (int i = 0; i < n; i++) {
        printf("Padre [%d]: Hijo %d sumará del índice %d al %d\n", getpid(), i+1, limites[i][0], limites[i][1]-1);
    }

    for (int i = 0; i < n; i++) {
        pids[i] = fork();
        if (pids[i] == 0) {
            printf("Hijo %d [%d]: Iniciando suma.\n", i+1, getpid());
            long long suma = 0;
            for (int j = limites[i][0]; j < limites[i][1]; j++) {
                suma += vector[j];
            }
            FILE *outfile = fopen("out.txt", "a");
            if (!outfile) {
                error("Hijo: Error abriendo out.txt");
            }
            fprintf(outfile, "%lld\n", suma);
            fclose(outfile);
            printf("Hijo %d [%d]: Suma parcial %lld escrita en out.txt.\n", i+1, getpid(), suma);
            exit(0);
        } else if (pids[i] < 0) {
            error("Error en fork");
        }
    }

    for (int i = 0; i < n; i++) {
        wait(NULL);
    }
    printf("Padre [%d]: Todos los hijos han terminado.\n", getpid());

    long long totalFinal = leerTotal(n);
    printf("\n====================================\n");
    printf("Resultado Final: La suma total es %lld\n", totalFinal);
    printf("====================================\n");

    free(vector);
    for (int i = 0; i < n; i++) free(limites[i]);
    free(limites);
    free(pids);
    return 0;
}
