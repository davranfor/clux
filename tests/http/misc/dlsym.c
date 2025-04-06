/*

Contenido de plugin.c
=====================
#include <stdio.h>

void print(const char *str)
{
    printf("%s\n", str);
    printf("Hola desde la librería dinámica\n");
}

Compilar libreria dinámica y el programa
========================================
gcc -shared -o libplugin.dylib plugin.c
gcc -o demo demo.c -ldl
./demo libplugin.dylib

*/

#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>

typedef void (*func_t)(const char *);

void call_function(const char *library_path, const char *function)
{
    void *handle;
    func_t func;
    char *error;
    const char *text_to_pass = "Hola desde C";

    handle = dlopen(library_path, RTLD_LAZY);
    if (!handle)
    {
        fprintf(stderr, "%s\n", dlerror());
        exit(1);
    }
    func = (func_t)dlsym(handle, function);
    if ((error = dlerror()) != NULL)
    {
        fprintf(stderr, "%s\n", error);
        exit(1);
    }
    func(text_to_pass);
    dlclose(handle);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s [library-path]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    call_function(argv[1], "print");
    getchar();
    call_function(argv[1], "print");
    return 0;
}

