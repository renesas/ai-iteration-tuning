#ifndef MODEL_WRAPPER_H
#define MODEL_WRAPPER_H

#include <mera/model.h>
#include <stdint.h>
#include <stdbool.h>

//===== This area will be modified by Python ====
// Input tensors

// Output tensors

// Model input pointers

// Model output pointers


static inline void mera_invoke() {
    RunModel(false);
}

#endif // MODEL_WRAPPER_H
