#include <stdio.h>      // Requerido para funciones de entrada/salida estándar (printf, fopen, etc.)
#include <stdlib.h>     // Requerido para exit(), calloc() para gestión de memoria
#include <unistd.h>     // Requerido para llamadas al sistema POSIX como fork(), getpid(), getppid()
#include <sys/types.h>  // Requerido para tipos de datos como pid_t (identificador de proceso)
#include <sys/wait.h>   // Requerido para la función wait()

/**
 * @brief Función utilitaria para manejar y reportar errores antes de terminar el programa.
 * @param msg El mensaje de error a mostrar.
 */
void error(char *msg) {
    perror(msg); // Imprime el mensaje de error del sistema junto con el mensaje personalizado
    exit(-1);    // Termina el programa con un código de error
}

/**
 * @brief Lee una lista de enteros desde un archivo.
 * El formato del archivo es: un número inicial 'n' que indica la cantidad de enteros,
 * seguido de 'n' enteros, cada uno en una nueva línea.
 * @param filename El nombre del archivo de entrada.
 * @param vec Un doble puntero al array de enteros que será asignado y llenado.
 * @return El número total de enteros leídos.
 */
int leerNumeros(char *filename, int **vec) {
    int c, numero, totalNumeros;
    FILE *infile;

    // Abre el archivo en modo lectura ("r")
    infile = fopen(filename, "r");
    if (!infile) {
        error("Error al abrir el archivo de entrada");
    }

    // Lee el primer número, que indica la cantidad total de enteros
    fscanf(infile, "%d", &totalNumeros);

    // Asigna memoria dinámicamente para el vector usando calloc (inicializa en cero)
    *vec = (int *)calloc(totalNumeros, sizeof(int));
    if (!*vec) {
        error("Error en calloc al asignar memoria");
    }

    // Lee cada número del archivo y lo guarda en el vector
    for (c = 0; c < totalNumeros; c++) {
        fscanf(infile, "%d", &numero);
        (*vec)[c] = numero;
    }

    fclose(infile); // Cierra el archivo
    return c;       // Retorna la cantidad de números leídos
}

/**
 * @brief Lee las dos sumas parciales del archivo "out.txt", las suma y retorna el total.
 * @return La suma final.
 */
int leerTotal() {
    FILE *infile;
    int sumap1 = 0, sumap2 = 0, total = 0;

    // Abre el archivo de resultados en modo lectura
    infile = fopen("out.txt", "r");
    if (!infile) {
        error("Error del padre al abrir el archivo de resultados");
    }

    // Lee las dos sumas parciales escritas por los procesos hijos
    fscanf(infile, "%d", &sumap1);
    fscanf(infile, "%d", &sumap2);

    // Calcula el total
    total = sumap1 + sumap2;
    fclose(infile); // Cierra el archivo
    return total;   // Retorna el resultado final
}


int main() {
    int *vector;
    int cantidadNumeros;
    int limites[2][2]; // Matriz para guardar los límites de suma para cada hijo
    int delta;
    pid_t pidHijo1, pidHijo2; // Variables para almacenar los IDs de los procesos hijos

    // Antes de cada ejecución, elimina el archivo out.txt para evitar leer datos antiguos [cite: 212, 215]
    remove("out.txt");

    // 1. El padre lee los números del archivo de entrada y los carga en 'vector' 
    cantidadNumeros = leerNumeros("input.txt", &vector);
    printf("Padre [%d]: Total de números leídos: %d\n", getpid(), cantidadNumeros);

    // 2. El padre calcula los rangos (índices) sobre los cuales trabajará cada hijo [cite: 112]
    delta = cantidadNumeros / 2;
    limites[0][0] = 0;               // Inicio para el hijo 1
    limites[0][1] = delta;           // Fin para el hijo 1
    limites[1][0] = delta;           // Inicio para el hijo 2
    limites[1][1] = cantidadNumeros; // Fin para el hijo 2

    printf("Padre [%d]: Hijo 1 sumará del índice %d al %d\n", getpid(), limites[0][0], limites[0][1]);
    printf("Padre [%d]: Hijo 2 sumará del índice %d al %d\n", getpid(), limites[1][0], limites[1][1]);

    // 3. El padre crea el primer proceso hijo [cite: 129]
    pidHijo1 = fork();

    // fork() devuelve un valor negativo si falla [cite: 22]
    if (pidHijo1 < 0) {
        error("Error en fork");
    }

    // fork() devuelve 0 al proceso hijo [cite: 21]
    if (pidHijo1 == 0) {
        // --- Lógica del Proceso Hijo 1 ---
        printf("Hijo 1 [%d]: Iniciando suma.\n", getpid());
        long long sumaParcial1 = 0;
        
        // 1. Recorre la porción del vector que le fue asignada [cite: 133]
        for (int i = limites[0][0]; i < limites[0][1]; i++) {
            sumaParcial1 += vector[i]; // 2. Calcula la suma parcial [cite: 134]
        }

        // 3. Escribe el resultado en el archivo de salida [cite: 135]
        FILE *outfile = fopen("out.txt", "a"); // "a" para modo 'append' (añadir al final)
        if (!outfile) {
            error("Hijo 1: Error abriendo el archivo de salida");
        }
        fprintf(outfile, "%lld\n", sumaParcial1);
        fclose(outfile);
        
        printf("Hijo 1 [%d]: Suma parcial %lld escrita en out.txt.\n", getpid(), sumaParcial1);
        exit(0); // El hijo 1 termina su ejecución exitosamente

    } else {
        // --- Lógica del Proceso Padre (continúa) ---
        // fork() devuelve el PID del hijo al proceso padre [cite: 20]
        // 3. El padre crea el segundo proceso hijo
        pidHijo2 = fork();

        if (pidHijo2 < 0) {
            error("Error en fork");
        }

        if (pidHijo2 == 0) {
            // --- Lógica del Proceso Hijo 2 ---
            printf("Hijo 2 [%d]: Iniciando suma.\n", getpid());
            long long sumaParcial2 = 0;

            // 1. Recorre la segunda mitad del vector
            for (int i = limites[1][0]; i < limites[1][1]; i++) {
                sumaParcial2 += vector[i]; // 2. Calcula su suma parcial
            }

            // 3. Escribe su resultado en el archivo de salida
            FILE *outfile = fopen("out.txt", "a");
            if (!outfile) {
                error("Hijo 2: Error abriendo el archivo de salida");
            }
            fprintf(outfile, "%lld\n", sumaParcial2);
            fclose(outfile);
            
            printf("Hijo 2 [%d]: Suma parcial %lld escrita en out.txt.\n", getpid(), sumaParcial2);
            exit(0); // El hijo 2 termina su ejecución
        }
    }

    // 4. El padre espera a que ambos hijos terminen su ejecución [cite: 130]
    // La llamada a wait() bloquea al padre hasta que un hijo finalice [cite: 99]
    wait(NULL);
    wait(NULL);
    printf("Padre [%d]: Ambos hijos han terminado.\n", getpid());

    // 5. El padre lee los resultados parciales y muestra el total final [cite: 131, 108]
    int totalFinal = leerTotal();
    printf("\n====================================\n");
    printf("Resultado Final: La suma total es %d\n", totalFinal);
    printf("====================================\n");

    // Libera la memoria que fue asignada dinámicamente para el vector
    free(vector);

    return 0; // Termina el programa principal
}