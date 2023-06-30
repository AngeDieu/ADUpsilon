/* In the standard MicroPython build system, this file is autogenerated from the
 * reset of the sources. We manually include it here some modules are not included
 * by the build system, so we need to manually update the MicroPython part
 *
 * How to update this file with a new MicroPython release
 * - Get a clean copy of MicroPython
 * - Copy our mpconfigport.h over the "bare-arm" port of MicroPython
 * - "make" the bare-arm port of MicroPython (don't worry if it doesn't finish)
 * - "cat build/genhdr/moduledefs.h".
 * - Insert the result below in the MicroPython section,
 *   until the definition of MICROPY_REGISTERED_MODULES
 * - copy the MICROPY_REGISTERED_MODULES section at the end of this file,
 *   /!\ this section is present twice in the file, so you need to copy it twice
 *   Keep the Upsilon part when copying the MICROPY_REGISTERED_MODULES section
*/

// MicroPython part

extern const struct _mp_obj_module_t mp_module___main__;
#undef MODULE_DEF_MP_QSTR___MAIN__
#define MODULE_DEF_MP_QSTR___MAIN__ { MP_ROM_QSTR(MP_QSTR___main__), MP_ROM_PTR(&mp_module___main__) },

extern const struct _mp_obj_module_t mp_module_builtins;
#undef MODULE_DEF_MP_QSTR_BUILTINS
#define MODULE_DEF_MP_QSTR_BUILTINS { MP_ROM_QSTR(MP_QSTR_builtins), MP_ROM_PTR(&mp_module_builtins) },

extern const struct _mp_obj_module_t mp_module_cmath;
#undef MODULE_DEF_MP_QSTR_CMATH
#define MODULE_DEF_MP_QSTR_CMATH { MP_ROM_QSTR(MP_QSTR_cmath), MP_ROM_PTR(&mp_module_cmath) },

extern const struct _mp_obj_module_t mp_module_math;
#undef MODULE_DEF_MP_QSTR_MATH
#define MODULE_DEF_MP_QSTR_MATH { MP_ROM_QSTR(MP_QSTR_math), MP_ROM_PTR(&mp_module_math) },

extern const struct _mp_obj_module_t mp_module_micropython;
#undef MODULE_DEF_MP_QSTR_MICROPYTHON
#define MODULE_DEF_MP_QSTR_MICROPYTHON { MP_ROM_QSTR(MP_QSTR_micropython), MP_ROM_PTR(&mp_module_micropython) },

extern const struct _mp_obj_module_t mp_module_urandom;
#undef MODULE_DEF_MP_QSTR_URANDOM
#define MODULE_DEF_MP_QSTR_URANDOM { MP_ROM_QSTR(MP_QSTR_urandom), MP_ROM_PTR(&mp_module_urandom) },

extern const struct _mp_obj_module_t mp_module_arit;
#undef MODULE_DEF_MP_QSTR_ARIT
#define MODULE_DEF_MP_QSTR_ARIT { MP_ROM_QSTR(MP_QSTR_arit), MP_ROM_PTR(&mp_module_arit) },

extern const struct _mp_obj_module_t mp_module_cas;
#undef MODULE_DEF_MP_QSTR_CAS
#define MODULE_DEF_MP_QSTR_CAS { MP_ROM_QSTR(MP_QSTR_cas), MP_ROM_PTR(&mp_module_cas) },

// Upsilon's modules part

extern const struct _mp_obj_module_t modion_module;
#undef MODULE_DEF_MP_QSTR_ION
#define MODULE_DEF_MP_QSTR_ION { MP_ROM_QSTR(MP_QSTR_ion), MP_ROM_PTR(&modion_module) },

extern const struct _mp_obj_module_t modescher_module;
#undef MODULE_DEF_MP_QSTR_ESCHER
#define MODULE_DEF_MP_QSTR_ESCHER { MP_ROM_QSTR(MP_QSTR_escher), MP_ROM_PTR(&modescher_module) },

extern const struct _mp_obj_module_t modkandinsky_module;
#undef MODULE_DEF_MP_QSTR_KANDINSKY
#define MODULE_DEF_MP_QSTR_KANDINSKY { MP_ROM_QSTR(MP_QSTR_kandinsky), MP_ROM_PTR(&modkandinsky_module) },

extern const struct _mp_obj_module_t modmatplotlib_module;
#undef MODULE_DEF_MP_QSTR_MATPLOTLIB
#define MODULE_DEF_MP_QSTR_MATPLOTLIB { MP_ROM_QSTR(MP_QSTR_matplotlib), MP_ROM_PTR(&modmatplotlib_module) },

extern const struct _mp_obj_module_t modpyplot_module;
#undef MODULE_DEF_MP_QSTR_PYPLOT
#define MODULE_DEF_MP_QSTR_PYPLOT { MP_ROM_QSTR(MP_QSTR_matplotlib_dot_pyplot), MP_ROM_PTR(&modpyplot_module) },

extern const struct _mp_obj_module_t modtime_module;
#undef MODULE_DEF_MP_QSTR_TIME
#define MODULE_DEF_MP_QSTR_TIME { MP_ROM_QSTR(MP_QSTR_time), MP_ROM_PTR(&modtime_module) },

extern const struct _mp_obj_module_t modos_module;
#undef MODULE_DEF_MP_QSTR_OS
#define MODULE_DEF_MP_QSTR_OS { MP_ROM_QSTR(MP_QSTR_os), MP_ROM_PTR(&modos_module) },

extern const struct _mp_obj_module_t modturtle_module;
#undef MODULE_DEF_MP_QSTR_TURTLE
#define MODULE_DEF_MP_QSTR_TURTLE { MP_ROM_QSTR(MP_QSTR_turtle), MP_ROM_PTR(&modturtle_module) },

#if !defined(INCLUDE_ULAB)

#define MICROPY_REGISTERED_MODULES \
    MODULE_DEF_MP_QSTR_BUILTINS \
    MODULE_DEF_MP_QSTR_CMATH \
    MODULE_DEF_MP_QSTR_MATH \
    MODULE_DEF_MP_QSTR_MICROPYTHON \
    MODULE_DEF_MP_QSTR_URANDOM \
    MODULE_DEF_MP_QSTR_ARIT \
    MODULE_DEF_MP_QSTR_CAS \
    MODULE_DEF_MP_QSTR___MAIN__ \
/* Upsilon's modules part */ \
    MODULE_DEF_MP_QSTR_ION \
    MODULE_DEF_MP_QSTR_ESCHER \
    MODULE_DEF_MP_QSTR_KANDINSKY \
    MODULE_DEF_MP_QSTR_MATPLOTLIB \
    MODULE_DEF_MP_QSTR_PYPLOT \
    MODULE_DEF_MP_QSTR_TIME \
    MODULE_DEF_MP_QSTR_OS \
    MODULE_DEF_MP_QSTR_TURTLE
#else
extern const struct _mp_obj_module_t ulab_user_cmodule;
#undef MODULE_DEF_MP_QSTR_ULAB
#define MODULE_DEF_MP_QSTR_ULAB { MP_ROM_QSTR(MP_QSTR_ulab), MP_ROM_PTR(&ulab_user_cmodule) },

#define MICROPY_REGISTERED_MODULES \
    MODULE_DEF_MP_QSTR_BUILTINS \
    MODULE_DEF_MP_QSTR_CMATH \
    MODULE_DEF_MP_QSTR_MATH \
    MODULE_DEF_MP_QSTR_MICROPYTHON \
    MODULE_DEF_MP_QSTR_URANDOM \
    MODULE_DEF_MP_QSTR_ARIT \
    MODULE_DEF_MP_QSTR_CAS \
    MODULE_DEF_MP_QSTR___MAIN__ \
/* Upsilon's modules part */ \
    MODULE_DEF_MP_QSTR_ION \
    MODULE_DEF_MP_QSTR_ESCHER \
    MODULE_DEF_MP_QSTR_KANDINSKY \
    MODULE_DEF_MP_QSTR_MATPLOTLIB \
    MODULE_DEF_MP_QSTR_PYPLOT \
    MODULE_DEF_MP_QSTR_TIME \
    MODULE_DEF_MP_QSTR_OS \
    MODULE_DEF_MP_QSTR_TURTLE \
    MODULE_DEF_MP_QSTR_ULAB
#endif
// MICROPY_REGISTERED_MODULES
