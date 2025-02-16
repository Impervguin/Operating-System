#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <stdlib.h>

#define WRITER_COUNT 5
#define READER_COUNT 4
#define COUNT (WRITER_COUNT + READER_COUNT)


HANDLE may_read;
HANDLE may_write;
HANDLE bmutex;

LONG read_queue = 0;
LONG write_queue = 0;
LONG active_readers = 0;

char symbol = 'a';

void start_read() {
    InterlockedIncrement(&read_queue);
    if (write_queue || !WaitForSingleObject(may_write, 0)) 
        WaitForSingleObject(may_read, INFINITE);
    SetEvent(may_read);
    WaitForSingleObject(bmutex, INFINITE);
    InterlockedIncrement(&active_readers);
    InterlockedDecrement(&read_queue);
    ReleaseMutex(bmutex);
}

void stop_read() {
    InterlockedDecrement(&active_readers);
    if (active_readers == 0) 
        SetEvent(may_write);
}

void start_write() {
    InterlockedIncrement(&write_queue);
    if (active_readers || !WaitForSingleObject(may_read, 0))
        WaitForSingleObject(may_write, INFINITE);
    
    InterlockedDecrement(&write_queue);
    
}

void stop_write() {
    if (write_queue)
        ResetEvent(may_write);
    else
        SetEvent(may_read);
}


DWORD32 WINAPI writer(PVOID param) {
    srand(GetCurrentThreadId());
    while (TRUE) {
        Sleep(rand() % 2000 + 1000);
        start_write();
        if (symbol == 'z')
            symbol = 'a';
        else
            symbol++;
        printf("%ld write %c\n", GetCurrentThreadId(), symbol);
        stop_write();
    }
    return 0;
}

DWORD32 WINAPI reader(PVOID param) {
    srand(GetCurrentThreadId());
    while (TRUE) {
        Sleep(rand() % 2000 + 1000);
        start_read();
        printf("%ld read %c\n", GetCurrentThreadId(), symbol);
        stop_read();
    }
    return 0;
}

int main() {
    DWORD32 thread_ids[COUNT];
    HANDLE threads[COUNT];

    if ((may_read = CreateEvent(NULL, FALSE, FALSE, NULL)) == NULL) {
        printf("event error\n");
        ExitProcess(1);
    }

    if ((may_write = CreateEvent(NULL, TRUE, FALSE, NULL)) == NULL) {
        printf("event error\n");
        ExitProcess(1);
    }

    if ((bmutex = CreateMutex(NULL, FALSE, NULL)) == NULL) {
        printf("mutex error\n");
        ExitProcess(1);
    }

    for (int i = 0; i < WRITER_COUNT; i++) {
        threads[i] =  (HANDLE) _beginthreadex(NULL, 0, writer, NULL, 0, thread_ids + i);
        if (threads[i] == NULL) {
            printf("thread error\n");
            ExitProcess(1);
        }
    }

    for (int i = WRITER_COUNT; i < COUNT; i++) {
        threads[i] =  (HANDLE) _beginthreadex(NULL, 0, reader, NULL, 0, thread_ids + i);
        if (threads[i] == NULL) {
            printf("thread error\n");
            ExitProcess(1);
        }
    } 

    for (int i = 0; i < COUNT; i++) {
        DWORD wres = WaitForMultipleObjects(COUNT, threads, FALSE, INFINITE);
        if (wres >= WAIT_OBJECT_0 && wres < WAIT_OBJECT_0 + COUNT) {
            printf("thread %ld finished\n", thread_ids[wres - WAIT_OBJECT_0]);
        } else if (wres == WAIT_FAILED) {
            printf("wait error\n");
            ExitProcess(1);
        } else {
            printf("wait unknown %ld\n", wres);
            ExitProcess(1);
        }
    }
    
    
    CloseHandle(may_read);
    CloseHandle(may_write);
    CloseHandle(bmutex);
    

}