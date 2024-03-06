#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

/*
Instituto Tecnológico de Costa Rica
Sistemas Operativos
Sebastián de Jesús Bogantes Rodríguez 2020028437
Angello Vividea García 2019156710
Semestre 1
2024
*/


// Compilación: gcc -o paralelismo1 paralelismo1.c -lpthread


// Número de hilos, hice pruebas en una máquina virtual con 2 CPU
// Si no se tiene cuidado genera una sobrecarga y eso afecta al rendimiento
#define NUM_THREADS 2

// Definición de estructura para pasar datos a los hilos
typedef struct {
    int *arr;
    int left;
    int right;
    int moda;
    int frecuencia;
} datos_hilo;

// Prototipos de funciones
void generarNumeros(int n, int numeros[]);
void clonarArreglo(int n, int *original, int *clon);
void merge(int arr[], int l, int m, int r);
void mergeSort(int arr[], int l, int r);
void* threadMergeSort(void* arg);
void mergeSortParalelo(int *arr, int n);
void imprimirVector(int *arr, int n);
void* calcularModa(void* arg);

// Random
void generarNumeros(int n, int numeros[]) {
    srand(time(NULL));
    for (int i = 0; i < n; i++) {
        numeros[i] = rand() % 2001 - 1000;
    }
}

// Clona el arreglo
void clonarArreglo(int n, int *original, int *clon) {
    for (int i = 0; i < n; i++) {
        clon[i] = original[i];
    }
}

// Imprime el vector
void imprimirVector(int *arr, int n) {
    printf("[ ");
    for (int i = 0; i < n; i++) {
        printf("%d", arr[i]);
        if (i < n - 1) {
            printf(", ");
        }
    }
    printf(" ]\n");
}


// Merge básico
void merge(int arr[], int l, int m, int r) {
    int i, j, k;
    int n1 = m - l + 1;
    int n2 = r - m;

    // Asigna memoria dinámicamente para los subarreglos temporales
    int *L = (int *)malloc(n1 * sizeof(int));
    int *R = (int *)malloc(n2 * sizeof(int));

    if (L == NULL || R == NULL) {
        fprintf(stderr, "Error al asignar memoria.\n");
        exit(EXIT_FAILURE); 
    }

    // Copia los datos a los subarreglos temporales L[] y R[]
    for (i = 0; i < n1; i++)
        L[i] = arr[l + i];
    for (j = 0; j < n2; j++)
        R[j] = arr[m + 1 + j];

    // Fusiona los subarreglos temporales de vuelta
    i = 0; 
    j = 0; 
    k = l; 
    while (i < n1 && j < n2) {
        if (L[i] <= R[j]) {
            arr[k] = L[i];
            i++;
        } else {
            arr[k] = R[j];
            j++;
        }
        k++;
    }

    while (i < n1) {
        arr[k] = L[i];
        i++;
        k++;
    }

    while (j < n2) {
        arr[k] = R[j];
        j++;
        k++;
    }

    free(L);
    free(R);
}


// Mergesort normal
void mergeSort(int arr[], int l, int r) {
    if (l < r) {
        int m = l + (r - l) / 2;
        mergeSort(arr, l, m);
        mergeSort(arr, m + 1, r);
        merge(arr, l, m, r);
    }
}

// Ejecuta Merge Sort en un segmento del arreglo en un hilo propio.
void* threadMergeSort(void* arg) {
    datos_hilo* datos = (datos_hilo*)arg;
    mergeSort(datos->arr, datos->left, datos->right);
    pthread_exit(NULL);
}


// Mergesort con paralelismo no recursivo
void mergeSortParalelo(int *arr, int n) {
    int tamañoSegmento = n / NUM_THREADS;
    pthread_t threads[NUM_THREADS];
    datos_hilo datos[NUM_THREADS];
    
    // Divide el vector en segmentos, crea hilos y los inicializa
    for (int i = 0; i < NUM_THREADS; ++i) {
        datos[i].arr = arr;
        datos[i].left = i * tamañoSegmento;
        if (i == NUM_THREADS - 1) {
            datos[i].right = n - 1;
        } else {
            datos[i].right = (i + 1) * tamañoSegmento - 1;
        }

        pthread_create(&threads[i], NULL, threadMergeSort, (void*)&datos[i]);
    }

    // Espera a que los hilos terminen
    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_join(threads[i], NULL);
    }

    // Fusión final de los segmentos ordenados 
    for (int i = 1; i < NUM_THREADS; i *= 2) {
        for (int j = 0; j < NUM_THREADS; j += 2*i) {
            if (j + i < NUM_THREADS) {
                int left = j * tamañoSegmento;
                int right = ((j + 2*i) * tamañoSegmento - 1 < n) ? (j + 2*i) * tamañoSegmento - 1 : n-1;
                int middle = left + (right - left) / 2; // Error corregido aquí
                merge(arr, left, middle, right);
            }
        }
    }
}

// Calcula la moda en cada hilo
void* calcularModa(void* arg) {
    datos_hilo* datos = (datos_hilo*) arg;
    int frecuenciaActual = 1;
    datos->moda = datos->arr[datos->left];
    datos->frecuencia = 1;
    
    // Actualiza la frecuencia si hay una más alta
    for (int i = datos->left + 1; i <= datos->right; ++i) {
        if (datos->arr[i] == datos->arr[i - 1]) {
            frecuenciaActual++;
            if (frecuenciaActual > datos->frecuencia) {
                datos->moda = datos->arr[i];
                datos->frecuencia = frecuenciaActual;
            }
        } else {
            frecuenciaActual = 1;
        }
    }
    
    pthread_exit(NULL);
}

int main() {
    int n;
    printf("Digite la cantidad de números que desea generar aleatoriamente:\n");
    scanf("%d", &n);

    if (n > 0) {
        int *numeros = (int *)malloc(n * sizeof(int));
        generarNumeros(n, numeros);

        int *numerosClonados = (int *)malloc(n * sizeof(int));
        clonarArreglo(n, numeros, numerosClonados);

        // Imprime el vector clonado antes de ordenar
        printf("Vector copiado antes de ordenar:\n");
        imprimirVector(numerosClonados, n);

        // Ordena el vector original usando Merge Sort Recursivo
        clock_t inicioRecursivo = clock();
        mergeSort(numeros, 0, n - 1);
        clock_t finRecursivo = clock();
        printf("Vector original ordenado con Merge Sort Recursivo:\n");
        imprimirVector(numeros, n);
        double tiempoRecursivo = (double)(finRecursivo - inicioRecursivo) / CLOCKS_PER_SEC;
        printf("\n");

        // Ordena el vector clonado usando Merge Sort Paralelo
        clock_t inicioParalelo = clock();
        mergeSortParalelo(numerosClonados, n);
        clock_t finParalelo = clock();
        printf("Vector clonado ordenado con Merge Sort Paralelo:\n");
        imprimirVector(numerosClonados, n);
        double tiempoParalelo = (double)(finParalelo - inicioParalelo) / CLOCKS_PER_SEC;

        // Impresión de los tiempos de ordenamientos de ambos merge
        printf("Tiempo Merge Sort Recursivo: %f segundos\n", tiempoRecursivo);
        printf("Tiempo Merge Sort Paralelo: %f segundos\n", tiempoParalelo);

        // Calcula la moda usando hilos
        pthread_t hilosModa[NUM_THREADS];
        datos_hilo datosModa[NUM_THREADS];
        int tamañoSegmento = n / NUM_THREADS;

        for (int i = 0; i < NUM_THREADS; i++) {
            datosModa[i].arr = numerosClonados;
            datosModa[i].left = i * tamañoSegmento;
            datosModa[i].right = (i + 1) * tamañoSegmento - 1;
            if (i == NUM_THREADS - 1) {
                datosModa[i].right = n - 1;
            }

            pthread_create(&hilosModa[i], NULL, calcularModa, &datosModa[i]);
        }

        // Espera a que se finalizen todos lo hilos
        for (int i = 0; i < NUM_THREADS; i++) {
            pthread_join(hilosModa[i], NULL);
        }

        // Saca la moda del vector 
        int modaGlobal = datosModa[0].moda;
        int frecuenciaGlobal = datosModa[0].frecuencia;
        for (int i = 1; i < NUM_THREADS; i++) {
            if (datosModa[i].frecuencia > frecuenciaGlobal) {
                frecuenciaGlobal = datosModa[i].frecuencia;
                modaGlobal = datosModa[i].moda;
            }
        }

        printf("La moda es: %d, con una frecuencia de: %d\n", modaGlobal, frecuenciaGlobal);

        free(numeros);
        free(numerosClonados);
    }

    return 0;
}