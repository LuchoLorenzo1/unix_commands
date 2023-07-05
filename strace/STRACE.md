# Strace
## Compilación

Hay un makefile, se compila todo corriendo `make` solo. Luego podemos correr `./strace <COMANDO>`

## Strace explicación código

El código comienza haciendo un fork, cuyo hijo es en el cual se va a ejecutar el comando a ser analizado, por lo que se llama a la syscall `ptrace(2)` con el argumeto `PTRACE_TRACEME`, indicandole al padre que puede ser analizado.

```c
ptrace(PTRACE_TRACEME, getpid());
```

Como indica `man 2 ptrace`, el comando a ser analizado se va a parar cada vez que se reciba una señal, y podemos verla desde el comando padre llamando a cualquier función de la familia `wait(2)`.

Por lo que lo primero que se hace desde el padre es esperar con un `wait()` a que se ejecute `PTRACE_TRACEME` en el hijo, el cual va a seguir parado hasta que le indiquemos algo con otro `ptrace(2)`.

Como lo que se quiere es analizar las syscalls, llamamos a `ptrace(2)` con el argumento `PTRACE_SYSCALL`, el cual va a hacer continuar al hijo, pero que se pare denuevo cuando este llame a una syscall o reciba una respuesta de una syscall.

```c
ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
waitpid(pid, &status, 0);
```

Cuando se termina el wait, es porque se llamó una syscall en el proceso hijo. para saber que syscall es, se llama a `ptrace(2)` con argumento `PTRACE_GETREGS`, pasandole como cuarto argumento un puntero a un `struct user_regs_struct` (el cual está definido en `sys/user.h`). Dentro del struct se tiene acceso a los registros `rax`, `rbx`, `rdx`, etc. En `x86-64` la llamada a syscall se hace através del registro `rax`, por lo que ahi estará el número.
* En este punto también se podría analizar los argumentos de la syscall, que se encuentran en los registros `rdi`, `rsi`, `rdx`, etc.

Para saber la respuesta de la syscall, se realiza exactamente el mismo proceso, llamamos `ptrace(2)` con `PTRACE_SYSCALL`, luego un `wait(2)` esperando la respuesta. Luego utilizamos `PTRACE_GETREGS` para encontrar la repuesta en los registros. En `x86-64`, el valor de retorno de la syscall también está en `rax`.

Por lo que ahora es cuestión de realizar un bucle hasta que muera el proceso hijo, en el cual se siguen los siguientes pasos:
1. `PTRACE_SYSCALL`, `wait`: esperamos una nueva llamada a syscall
2. `PTRACE_GETREGS`: analizamos el número de la syscall
3. `PTRACE_SYSCALL`, `wait`: esperamos la respuesta de la syscall
4. `PTRACE_GETREGS`: analizamos el número de retorno de la syscall

## Numero syscall a nombre

Para saber el nombre de la syscall desde su número, utilicé la tabla recomendada en la consigna: https://gitlab.com/strace/strace/-/blob/master/src/linux/x86_64/syscallent.h

Pero como no sabía como utilizarla directamente, la formatee para que sea un vector de `chars[23]`. donde el índice indica el número de la syscall.
* nota: 23 es el largo del nombre de la syscall más larga

## Prueba mkdir

Hice un mini programa en c que solo llama a `mkdir(2)`. Se puede corroborar fácilmente que `./strace  ./mkdir carpeta`, imprime las mismas syscalls que `strace ./mkdir carpeta`.
