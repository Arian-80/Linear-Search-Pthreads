#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

struct ComputeInfo {
    const int* array;
    int itemToSearch;
    long long start;
    long long end;
    pthread_barrier_t syncBarrier;
};

long long linear_search(struct ComputeInfo computeInfo) {
    long long start = computeInfo.start;
    long long end = computeInfo.end;
    pthread_barrier_wait(&computeInfo.syncBarrier);
    int itemToSearch = computeInfo.itemToSearch;
    const int* array = computeInfo.array;
    for (long long i = start; i < end; i++) {
        if (itemToSearch == array[i]) {
            return i;
        }
    }
    return -1;
}

long long setupThreads(struct ComputeInfo* computeInfo, int threadCount,
                size_t arraySize, void* function) {
    if (threadCount < 1 || threadCount > arraySize) return -1;
    pthread_t* threads = (pthread_t*) malloc(threadCount * sizeof(pthread_t));
    if (!threads) return -1;

    long long portion = (long long) arraySize / threadCount;
    long long remainder = (long long) arraySize % threadCount;
    if (pthread_barrier_init(&computeInfo->syncBarrier, NULL, 2)) return -1;

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
            return -1;
        }
        pthread_barrier_wait(&computeInfo->syncBarrier);
    }
    pthread_barrier_destroy(&computeInfo->syncBarrier);
    long long result = LONG_LONG_MAX;
    long long returnVal;
    for (int i = 0; i < threadCount; i++) {
        pthread_join(threads[i], (void**) &returnVal);
        if (returnVal != -1 && returnVal < result) { // Store first occurrence
            result = returnVal;
        }
    }
    free(threads);
    return result;
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
    size_t size = 1000000000;
    int item = 12;
    int* array = (int*) malloc(size * sizeof(int));
    array[456318842] = item;
    long long index = parallel_linear_search(array, item, 8, size);
    if (index >= 0) {
        printf("First occurrence of item [%d] is at index: %lli\n", item, index);
    }
    return 0;
}
