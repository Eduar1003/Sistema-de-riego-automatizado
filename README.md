Mini proyecto poniendo en práctica mis conocimientos en programación y en el lenguaje C++ con Arduino

El sistema consta de:
1 Arduino
1 Sensor de temperatura
1 Motor que simula el actuador de riego
1 Display LCD 16 X 2
1 Teclado matricial 4 x 4

Pensado para tomar el valor que el usuario ingrese y actuar dependiendo del cultivo según se haya configurado en el código

Se puede modificar el código para agregar o eliminar cultivos

Ejemplo:

    case 1: // Cilantro
    init_range_tmp = 3.0; // temperatura minima a la que se puede regar
    max_range_tmp = 12.0; // temperatura máximaa la que se puede regar
    min_hum = 4.0; // humedad del suelo en porcentaje
    break;
