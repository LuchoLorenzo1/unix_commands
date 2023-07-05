# shell

### Búsqueda en $PATH

La familia de wrappers exec(3) utilizan por debajo la syscall execve, y las diferencias entre las distintas funciones de la familia son en como el programa es encontrado, como se especifican los argumentos y de donde proviene el entorno. 
Las funciones con `v` tienen un arreglo como parametro para especificar el `argv[]` del nuevo programa.
Las funciones con `l` toman los argumentos del nuevo programa desde la llamada misma la funcion mediante una lista de largo variable, el final de la lista debe ser un `(char *) NULL`.
Las funciones con `e` toman un argumento para proveer el entorno del nuevo programa, y sino, hereda el entorno del proceso actual.
Las funciones con `p` utilizan la variable de entorno PATH para buscar el programa a ejecutar.

Si, la llamada a cualquier funcion de  `exec()` puede fallar y en ese caso, el valor de retorno es -1. Ademas de eso, `errno`, que es el numero del ultimo error, se setea para indicar el error correspondiente, estos errores provienen de la syscall `execve (2)` y en su respectivo manual se declaran todos los tipos de errores.

---

### Procesos en segundo plano

La idea de la ejecucion de procesos en segundo plano se basa en que el proceso padre pueda esperar que se termine de ejecutar el proceso en segundo plano sin dejarlo huerfano pero sin bloquear la consola. Para lograr eso, lo que hacemos es utilizar un `waitpid` pasandole el PID del proceso hijo y agregandole el flag de `WHOHANG`. Este flag nos permite un comportamiento no bloqueante en la espera de procesos hijos permitiendo continuar con la ejecucion de la shell.

---

### Flujo estándar

El file descriptor 1 es el estandar output
El file descriptor 2 es el estandar error

Entonces se utiliza el `2>&1` para redirigir el estandar error al estandar output. En este caso se debe agregar `&` ya que sin eso, pareciera que el estandar error se redirige a un archivo llamado 1, en cambio agregando `&` se determina que el 1 es un file descriptor.

cat out.txt del ejemplo:
  ls: cannot access '/noexiste': No such file or directory
  /home:
  franco

cat out.txt invirtiendo:  
  ls: cannot access '/noexiste': No such file or directory
  /home:
  frato

En cambio, en bash imprime por pantalla el error y el out.txt contiene:
  /home:
  frato

Este es el comportamiento correcto ya que al invertir las redirecciones, primero se asigna la salida estandar del error a la salida estandar output, la cual en ese momento es la pantalla, por ende el la salida estandar del error es la pantalla y luego se asigna la salida estandar output al archivo out.txt. Pero en nuestra terminal se ejecuta todo en simultaneo.  

---

### Tuberías múltiples

Cuando se ejecuta un pipe, la shell reporta el exit status del ultimo comando dentro del pipe. Por ejemplo en el siguiente caso:
    hola_mundo.sh 5 | grep "Hola Mundo"
El exit status que se reporta es el correspondiente al comando grep.
En el caso que se quiera conocer el exit status de todos los comandos, se utiliza el arreglo PIPESTATUS, que viene incluido en bash.

Este es un ejemplo ejecutado en bash donde el primer comando falla:
$ ls ./noexiste | wc -w 
ls: cannot access './noexiste': No such file or directory
0

Y en nuestra terminal funciona de la misma manera.

El problema de los errores en el pipe es que si nosotros reportaramos el exit status en este caso, nos saltaria el exit status del wc, el cual funciono bien. Eso podria ocasionar en ciertos casos, que se cubra el error del primer comando si no imprimiera nada en pantalla. Para solucionar esto se puede usar el pipefail que detendria el pipe ante el primer error.

---

### Variables de entorno temporarias
La idea de hacerlo luego de la llamada a fork(2) es que al ser variables de entorno temporales queremos que mueran con el proceso hijo. Solo queremos que existan durante la ejecucion del comando acutal que estaremos ejecutando. Si nosotros ejecutamos algo como lo siguiente:
```
$ TEMP=nada pwd
$ echo $TEMP
```
No tendremos nada ya que la variable temporal `$TEMP` solo existe en el proceso de `pwd` y no deberia seguir existiendo luego.


Con respecto a como sera el comportamiento si utulizamos la familia de funciones de exec(3) que terminan con la letra e en vez de usar setenv(3) podemos decir que no sera el mismo. Esto se debe a que si nosotros utilizamos setenv(3) para definir las variables al momento de ejecutar exec tendremos a disposicion todas las variables de entorno mientras que si le pasamos explicitamente las variables mediante un array a exec solo contaremos con esas que le pasamos y no las demas.

Podriamos tener el mismo comportamiento si obtenemos todas las variables de entorno (temporales y de la shell) y se la pasamos con las nuevas que queremos a exec.

---

### Pseudo-variables
Las tres variables magicas estandar que vamos a explicar son: `$$`, `$!` y `$_`.

 - La variable magica `$$` tiene el proceso actual en bash, es edcir el PID.
```
$ echo $$
4045
```
 - La variable magica `$0` contiene el nombre del programa actual.  
 ```
$ echo $0
-bash
 ```
 - La variable magica `$_` contiene el ulitmo argumento del comando anterior que se ejecuto. 
  ```
$ sleep 5
$ echo $_
5
  ```

---

### Comandos built-in

Entre `cd` y `pwd`, el que podría ser implementado sin necesidad de ser built-in es `pwd`. Ya que este puede ser un ejecutable común dentro del path, que escriba en stdout el directorio actual utilizand la función de c `getcwd()`. Y sabemos que será el mismo directorio que el de la shell, ya que el fork no cambia el directorio de trabajo del nuevo proceso.

En cambio `cd` debe estar junto al funcionamiento de la shell, ya que tiene que cambiar el directorio del proceso y de futuros forks.

Una razón para que `pwd` sea built-in, es para no tener que hacer un fork y un exec solo para ejecutar una función simple que tiene c.

---

### Historial

Cuando la terminal entra en modo no canonico deja de leer cuando se ingresa un fin de linea para pasar a leer constantemente. MIN y TIME son los parametros que se tienen para regular como se lee. 

MIN establece la cantidad minima de caracteres que se tiene que ingresar en la terminal para que la función de lectura retorne. Si MIN es 0, entonces la función retorna constantemente, haya algo escrito o no. Si MIN es mayor a 0, por poner un ejemplo 1, entonces la función de lectura se vuelve bloqueante y retorna unicamente cuando se registre la escritura de 1 byte.

La razón de TIME es hacer que la función que es bloqueante cuando se establece MIN retorne eventualmente cada un determinado tiempo independientemente si se cumple la condición de MIN para que siga la ejecución del programa. Cuando TIME es 0, el funcionamiento de lectura es como se expresó arriba. Cuando TIME es mayor a 0 y no se establece un minimo de lectura el objetivo es que no retorne constantemente la función de lectura sino que solo cuando se cumple el timeout. En caso de poder leer algo se retorna lo leido, caso contrario retorna 0. Y cuando TIME es mayor a 0 y MIN es mayor a 0 la función va a retornar cuando se cumpla el minimo de lectura o se cumpla el timeout establecido, haciendo que la función de lectura no sea bloqueante.

En el caso concreto de MIN = 1 y TIME = 0 en el trabajo se logra que la función sea bloqueante y que solo se continue la ejecución cuando hay un caracter nuevo ingresado, y no es necesario considerar el caso donde la lectura no retorna ningun caracter.

### Implementacion extra: La primera opcion
