/* Proof of concept for a module which allows to call JS functions from
   Python.

   You can test it from a web browser, or directly from node:

   $ node ./build/micropython.js

    MicroPython v1.19.1-532-g8e912a501-dirty on 2023-02-01; JS with Emscripten
    Type "help()" for more information.
    >>> import jsffi
    >>> jsffi.console_log("hello")
    hello
    0
*/

#include <emscripten.h>
#include "py/runtime.h"

/* EM_JS is Emscripten magic.
   This defines a C function with the following signature:
      void console_log(const char *str);

   However, the *body* of the function is Javascript code. Emscripten takes
   care of doing all the glueing to connect the C and JS sides.
*/
EM_JS(void, console_log, (const char* str), {
  console.log(UTF8ToString(str));
});

/* This is the micropython-side of things. When we call
   `jsffi.console_log(...)` from Python, we end up calling this function.

   Basically, the control flow is the following:

   1. the user calls jsffi.console_log("hello") from Python

   2. we enter jsffi_console_log; `obj` is the Python-level string object

   3. we use mp_get_buffer_raise to get a char* pointer to the C buffer
      containing the string

   4. we call console_log()

   5. console_log is implemented in JS, thanks to Emscripten magic. It
      receives a "pointer" to the buffer, which is just an index into the WASM
      memory

   6. Inside console_log, we call UTF8ToString(), which is an Emscripten
      helper to convert a C pointer to char into a "proper" JS string

   7. We call the JS console.log() using the string obtained in (6)
*/
STATIC mp_obj_t jsffi_console_log(mp_obj_t obj) {
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(obj, &bufinfo, MP_BUFFER_READ);
    console_log(bufinfo.buf);
    return mp_obj_new_int(0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(jsffi_console_log_obj, jsffi_console_log);


STATIC const mp_rom_map_elem_t mp_jsffi_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_jsffi) },
    { MP_ROM_QSTR(MP_QSTR_console_log), MP_ROM_PTR(&jsffi_console_log_obj) },
};

STATIC MP_DEFINE_CONST_DICT(mp_jsffi_globals, mp_jsffi_globals_table);

const mp_obj_module_t mp_module_jsffi = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&mp_jsffi_globals,
};

// Register the module to make it available in Python.
MP_REGISTER_MODULE(MP_QSTR_jsffi, mp_module_jsffi);
