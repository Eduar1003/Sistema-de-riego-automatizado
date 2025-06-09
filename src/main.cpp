// Sistema de riego automático con pantalla lcd, sensor de humedad (YL-69) y sensor de temperatura (TMP 36)
// Incluyendo un teclado matricial para controlar el sistema
// Desarrollador: EduarÖ Gutiérrez
// Fecha: 2025-06-08

#include <LiquidCrystal.h> // Librería para la pantalla lcd
#include <Keypad.h> // Librería para el teclado matricial

// Asignación de pines de la pantalla lcd
              // RS E DB4 DB5 DB6 DB7 
LiquidCrystal lcd(0, 1, 2, 3, 4, 5);

// Configuración del teclado matricial
const byte ROWS = 4; // Número de filas
const byte COLS = 4; // Número de columnas

// Matriz de teclas
char keys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

// Pines de las filas y columnas
byte rowPins[ROWS] = {13, 12, 11, 10}; 
byte colPins[COLS] = {9, 8, 7, 6}; 

// Creación del teclado matricial
Keypad key = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Constantes para las teclas
const char A = 'A';
const char B = 'B';
const char C = 'C';
const char D = 'D';

// Variables para el rango de la temperatura y la humedad
float init_range_tmp;
float max_range_tmp;
float min_hum;

// Variable para verificar si se seleccionó un cultivo válido
bool isValid = false;

// Struct clave valor de los cultivos

struct keyValue {
  String crop;
  int structIndex;
};

//String cropList [] = {"Cilantro" : 1, "Fresa" : 2, "Arroz" : 3, "Tomate" : 4, "Zanahoria" : 5};
// Se comporta como un diccionario clave valor, donde la clave es el nombre del cultivo y el valor es el indice del cultivo
keyValue cropList [] = {
  {"Cilantro", 0},
  {"Fresa", 1},
  {"Arroz", 2},
  {"Tomate", 3},
  // {"Zanahoria", 4}
};

// Variable para almacenar el indice del cultivo seleccionado
int index;

// Se obtiene el tamaño de la lista de cultivos dividiendo el tamaño total del array cropList entre el tamaño del struct keyValue
int sizeCropList = sizeof(cropList) / sizeof(cropList[1]);


// Función para recorrer la lista de cultivos
void showMenu() {

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Selec cultivo: ");
  delay(2000);  
  lcd.clear();

  for (int i = 0; i < sizeCropList; i++) {

    lcd.setCursor(0, 0);
    lcd.print(cropList[i].structIndex + 1); // Se suma 1 para que el indice se muestre en 1 en lugar de 0
    lcd.setCursor(0, 1);
    lcd.print(cropList[i].crop);
    delay(2000);
    lcd.clear();
  }
}

// Se crea una función para asignar el cultivo que el usuario seleccionó
void selectCrop(int option) {
  switch (option) {
    case 1: // Cilantro
    init_range_tmp = 3.0;
    max_range_tmp = 12.0;
    min_hum = 4.0;
    break;

    case 2: // Fresa
    init_range_tmp = 10.0;
    max_range_tmp = 25.0;
    min_hum = 35.0;
    break;

    case 3: // Arroz
    init_range_tmp = 10.0;
    max_range_tmp = 25.0;
    min_hum = 10.0;
    break;

    case 4: // Tomate
    init_range_tmp = 10.0;
    max_range_tmp = 25.0;
    min_hum = 10.0;
    break;

    // case 5: // Zanahoria
    // init_range_tmp = 15.0;
    // max_range_tmp = 35.0;
    // min_hum = 35.0;
    // break;

    default:
    break;
  } 
}


//Definición de pines analógicos
#define TMP_SENSOR A0 // Sensor de temperatura
#define HUM_SENSOR A1 // Sensor de humedad

//Definición de pines digitales
#define IRRIGATION_MOTOR A2 // Actuador de riego

float tmp; // Variable para almacenar la temperatura
float hum; // Variable para almacenar la humedad
bool turn_on; // Variable para almacenar un valor boolean

// Inicialización de la pantalla lcd
void initLCD()
{
  lcd.begin(16, 2);
  lcd.print("Sistema de riego");
  delay(2000);
  lcd.clear();
  lcd.print("Iniciando...");
  delay(3000); // Delay de 3 segundos antes de continuar
  lcd.clear();
}

// Función para imprimir los datos leídos
void printData()
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(tmp);
  lcd.print(" C");

  lcd.setCursor(0, 1);
  lcd.print("Humedad: ");
  lcd.print(hum);
  lcd.print(" %");
}

// Función para recibir el rango de la temperatura y la humedad y así controlar el motor de riego
bool receiveRange(float tmp ,float hum)
{
  if ((tmp >= init_range_tmp && tmp <= max_range_tmp) && hum <= min_hum)
    return true;

  else
    return false;
}

// Función para activar el motor de riego si se cumple la condición
void controlIrrigation(bool turn_on)
{
  if (turn_on == true)
    digitalWrite(IRRIGATION_MOTOR, HIGH);

  else
    digitalWrite(IRRIGATION_MOTOR, LOW);
}

void setup() {

  // Configuración de pines
  pinMode(TMP_SENSOR, INPUT); // Configuración del pin del sensor de temperatura como entrada
  pinMode(HUM_SENSOR, INPUT); // Configuración del pin del sensor de humedad como entrada
  pinMode(IRRIGATION_MOTOR, OUTPUT); // Configuración del pin del motor de riego como salida

  // Variable para almacenar la elección del usuario
  char option;

  // Llamada a la función initLCD
  initLCD();

  // Se pregunta al usuario por el cultivo a regar mostrando un menú
  showMenu();

  // Mientras no se presione una tecla seguimos esperando mostrando un mensaje de seleccionar un cultivo
  // Y no se puede avanzar hasta que se selecciona un cultivo válido
  do {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Seleccione un");
    lcd.setCursor(0, 1);
    lcd.print("cultivo valido");
    option = key.getKey();
    index = (option - '0'); // Se resta el valor ASCII de '0' para obtener el valor numérico de la tecla presionada

    // Se verifica que la tecla presionada sea válida y que el índice esté dentro del rango de cultivos
    // Los índices válidos son de 1 a sizeCropList
    if (option != NO_KEY && index >= 1 && index <= sizeCropList)
    {
      // Restamos 1 porque los cultivos están indexados desde 0
      int selectedCrop = index - 1;

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Ud selecciono: ");

      lcd.setCursor(0, 1);
      lcd.print(cropList[selectedCrop].crop); // Mostrar nombre del cultivo
      delay(2000);
      lcd.clear();

      lcd.setCursor(0, 0);
      lcd.print("Cargando...");
      delay(1000);
      lcd.clear();

      isValid = true;
    }

    delay(150);

  } while (isValid == false);

  // Enviamos el parámetro seleccionado por el usuario
  selectCrop(index);

}

void loop() {

  tmp = ((analogRead(TMP_SENSOR) * 5.0 / 1023.0) * 100.0) - 50; // Lectura del sensor de temperatura, restándole 50 al resultado para coincidir en Tinkercad
  hum = (analogRead(HUM_SENSOR) * 5.0 / 1023.0) * 100.0; // Lectura del sensor de humedad

  // Se recibe el rango de la temperatura y la humedad, si es true se enciende el motor de riego de lo contrario se apaga
  turn_on = receiveRange(tmp, hum);

  // Impresión de los datos leídos en la pantalla lcd
  printData();

  // Se activa el motor de riego si se cumple la condición de la función receiveRange
  controlIrrigation(turn_on);

  delay(1000); // Delay de 1 segundo antes de hacer la siguiente lectura

}