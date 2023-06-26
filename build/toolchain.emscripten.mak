CC = emcc
CXX = emcc
LD = emcc
CPP = cpp

EMFLAGS = -s PRECISE_F32=1 -s ASYNCIFY

ifeq ($(DEBUG),1)
EMFLAGS += --profiling-funcs
EMFLAGS += -s ASSERTIONS=1
EMFLAGS += -s SAFE_HEAP=1
EMFLAGS += -s STACK_OVERFLOW_CHECK=1
EMFLAGS += -s DEMANGLE_SUPPORT=1
EMFLAGS += -s MAIN_MODULE=1
EMFLAGS += -g
EMFLAGS += -O3
else
EMFLAGS += -s MAIN_MODULE=2
endif

EMFLAGS += -DXCAS

# EMFLAGS += -s ERROR_ON_UNDEFINED_SYMBOLS=0

# Configure EMFLAGS
EMFLAGS += -s WASM=1 -s SINGLE_FILE=1
EMFLAGS += -Wno-unused-command-line-argument

# Configure LDFLAGS
EMSCRIPTEN_MODULARIZE ?= 1
LDFLAGS += -s MODULARIZE=$(EMSCRIPTEN_MODULARIZE) -s 'EXPORT_NAME="Epsilon"' --memory-init-file 0 -lidbfs.js

SFLAGS += $(EMFLAGS)
LDFLAGS += $(EMFLAGS) -Oz -s EXPORTED_RUNTIME_METHODS='["UTF8ToString"]' -s EXTRA_EXPORTED_RUNTIME_METHODS='["ccall", "cwrap","Pointer_stringify","writeStringToMemory"]' -s EXPORTED_FUNCTIONS='["_main", "_IonSimulatorKeyboardKeyDown", "_IonSimulatorKeyboardKeyUp", "_IonSimulatorEventsPushEvent", "_IonSoftwareVersion", "_IonPatchLevel", "_IonDisplayForceRefresh","_load_nwstate","_restore_calc_history","_copy_to_fs","_save_state","_save_calc_history"]'
