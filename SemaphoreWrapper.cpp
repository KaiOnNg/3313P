#include "Semaphore.h"
#include <exception>

extern "C" {
    Semaphore* Semaphore_new(const char* name, int initialState = 0, bool createAsOwner = false) {
        try {
            return new Semaphore(std::string(name), initialState, createAsOwner);
        } catch (...) {
            return nullptr; // Consider proper exception handling or error reporting
        }
    }

    void Semaphore_signal(Semaphore* sem) {
        if (sem != nullptr) {
            sem->Signal();
        }
    }

    void Semaphore_wait(Semaphore* sem) {
        if (sem != nullptr) {
            sem->Wait();
        }
    }

    void Semaphore_delete(Semaphore* sem) {
        delete sem;
    }
}
