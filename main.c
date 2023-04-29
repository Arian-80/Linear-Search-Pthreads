#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

struct ComputeInfo {
    const int* array;
    int itemToSearch;
    long long foundIndex;
    long long start;
    long long end;
    pthread_barrier_t syncBarrier;
    pthread_mutex_t foundIndexMutex;
};

void linear_search(struct ComputeInfo computeInfo) {
    long long start = computeInfo.start;
    long long end = computeInfo.end;
    pthread_barrier_wait(&computeInfo.syncBarrier);
    int itemToSearch = computeInfo.itemToSearch;
    const int* array = computeInfo.array;
    for (long long i = start; i < end; i++) {
        if (computeInfo.foundIndex < i) {
            return;
        }
        if (itemToSearch == array[i]) {
            pthread_mutex_lock(&computeInfo.foundIndexMutex);
            if (computeInfo.foundIndex > i) computeInfo.foundIndex = i;
            pthread_mutex_unlock(&computeInfo.foundIndexMutex);
            return;
        }
    }
}

long long setupThreads(struct ComputeInfo* computeInfo, int threadCount,
                size_t arraySize, void* function) {
    if (threadCount < 1 || threadCount > arraySize) return -1;
    pthread_t* threads = (pthread_t*) malloc(threadCount * sizeof(pthread_t));
    if (!threads) return -1;

    long long portion = (long long) arraySize / threadCount;
    long long remainder = (long long) arraySize % threadCount;
    if (pthread_barrier_init(&computeInfo->syncBarrier, NULL, 2)) {
        free(threads);
        return -1;
    }
    if (pthread_mutex_init(&computeInfo->foundIndexMutex, NULL)) {
        pthread_barrier_destroy(computeInfo->syncBarrier);
        free(threads);
        return -1;
    }
    computeInfo->foundIndex = LONG_LONG_MAX;
    for (int i = 0; i < threadCount; i++) {
        if (i < remainder) {
            computeInfo->start = i * (portion + 1);
            computeInfo->end = computeInfo->start + portion + 1;
        }
        else {
            computeInfo->start = portion*(i - remainder) + remainder*(portion + 1);
            computeInfo->end = computeInfo->start + portion;
        }
        if (pthread_create(&threads[i], NULL, function,
                            computeInfo)) {
            for (int j = 0; j < i; j++) {
                pthread_join(threads[j], NULL);
            }
            pthread_barrier_destroy(&computeInfo->syncBarrier);
            pthread_mutex_destroy(&computeInfo->foundIndexMutex);
            free(threads);
            return -1;
        }
        pthread_barrier_wait(&computeInfo->syncBarrier);
    }
    pthread_barrier_destroy(&computeInfo->syncBarrier);
    for (int i = 0; i < threadCount; i++) pthread_join(threads[i], NULL);
    pthread_mutex_destroy(&computeInfo->foundIndexMutex);
    free(threads);
    return computeInfo->foundIndex;
}

long long parallel_linear_search(const int* intArray, int item,
                           int threadCount, size_t arraySize) {
    struct ComputeInfo computeInfo;
    computeInfo.array = intArray;
    computeInfo.itemToSearch = item;
    long long result = setupThreads(&computeInfo, threadCount,
                              arraySize,(void*) linear_search);
    if (result < 0) { // Error
        printf("An error has occurred. Please try again.");
        return -1;
    }
    else if (result == LONG_LONG_MAX) {
        printf("Item not found.\n");
        return -1;
    }
    return result;
}

int main() {
    for (int k = 1; k < 9; k++) {
        if (k == 3) k = 4;
        if (k == 5) k = 6;
        if (k == 7) k = 8;
        size_t size = 1000000000;
        int item = 12845612;
        int *array = (int *) calloc(size, sizeof(int));
        if (!array) return -1;
        array[size-1] = item;
        array[size-2] = item;
        clock_t start, end;
        start = clock();
        long long index = parallel_linear_search(array, item, k, size);
        end = clock();
        if (index >= 0) {
            printf("First occurrence of item [%d] is at index: %lli\n", item, index);
        }
        printf("Time taken: %g seconds.\n", (double)(end-start)/CLOCKS_PER_SEC);
        FILE *f = fopen("times.txt", "a");
        fprintf(f, "%g,", (double)(end-start)/CLOCKS_PER_SEC);
        free(array);
    }
    return 0;
}
