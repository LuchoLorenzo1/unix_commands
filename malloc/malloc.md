# malloc

Lugar para respuestas en prosa y documentación del TP.


### Almacenamiento de bloques

Para almacenar los bloques se decidió contar con 3 arreglos de tamaño fijo predeterminado. A pesar de tener que utilizar mucha memoria estatica en punteros, la ventaja principal es que los bloques estan organizados por tamaño. Por lo que se puede facilmente ir a alocar memoria al array de bloques adecuado.


### Metadata de la región
Se le decidió agregar a la metadata de cada región los siguentes datos:
* __prev__: Asi como cada región tiene una referencia a la siguiente región se decidió que también
tenga una referencia a la región anterior, dado que por ejemplo al liberar memoria se debe saber si
la región previa también se encuentra liberada para agruparlas. Se decidió incluir la referencia a la
metadata con objetivo de simplificar la resolución.
* __index__: Dado que se tiene un arreglo de bloques, al liberar una región de memoria dejando el bloque completamente libre y querer desmapear el bloque se realiza un swap de posición entre el bloque actual y el último para que el arreglo quede contiguo.

  Por este motivo es necesario tener una referencia al lugar que ocupa el bloque en el arreglo.Y como en free() solo recibimos el puntero existen 2 opciones, o se itera en el arreglo de bloques hasta encontrar la igualdad y de este modo obtener el indice o se
le agrega a la metadata el indice al cual pertenece cada región.

  De elegir agregar el indice se debió considerar que al realizar el swap se debe iterar por todas las regiones pertenecientes al ultimo bloque
y modificarlo. Tomando por ejemplo los bloques de 16Kib se está hablando de 2048 bytes, y con un tamaño
    minimo de bloque de 256 bytes estamos hablando de una iteración de 8 elementos a contraposición de
    iterar un arreglo de 5000 elementos, por lo que resulta más conveniente esta implementación.
