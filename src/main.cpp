// Sistema de riego automático con pantalla LCD 16x2, sensor de humedad (YL-69) y sensor de temperatura (TMP 36)
// Controlado mediante teclado matricial 4x4
// Autor: Eduar Gutiérrez
// Versión: 1.0
// Fecha: 2025-06-08

#include <LiquidCrystal.h> // Librería para la pantalla lcd
#include <Keypad.h> // Librería para el teclado matricial

// ========= CONFIGURACIÓN DE HARDWARE =========
// Definición de pines y parámetros del sistema

// Pines analógicos para sensores
#define TMP_SENSOR A0 // Pin del sensor de temperatura (TMP36)
#define HUM_SENSOR A1 // Pin del sensor de humedad del suelo (YL-69)

// Pines digitales para actuadores
#define IRRIGATION_MOTOR A2 // Pin para controlar el motor/rele de riego
#define DELAY_150_MS 150   // Delay de 150 ms
#define DELAY_1_SEG 1000   // Delay estándar de 1 segundo (en ms)
#define DELAY_2_SEG 2000   // Delay largo de 2 segundos (en ms)

// Configuración del teclado matricial 4x4
#define ROWS 4 // Número de filas del teclado
#define COLS 4 // Número de columnas del teclado

// Estructura para almacenar los datos del sensor
// Contiene:
// - temperature: Valor en °C leído del sensor TMP36
// - humidity: Porcentaje de humedad leído del sensor YL-69
// - update(): Método para actualizar los valores de los sensores
struct SensorData {
  float temperature;
  float humidity;
  
  void update() {
      temperature = ((analogRead(TMP_SENSOR) * 5.0 / 1023.0) * 100.0) - 50;
      humidity = (analogRead(HUM_SENSOR) * 5.0 / 1023.0) * 100.0;
  }
};

// Estructura para el estado global del sistema
// Almacena:
// - sensorReadings: Últimos valores de los sensores
// - motorActive: Estado del motor de riego (ON/OFF)
// - cropValid: Indica si se ha seleccionado un cultivo válido
// - selectedCrop: Índice del cultivo seleccionado (1-based)
struct SystemState {
    SensorData sensorReadings;  // Datos del sensor
    bool motorActive;     // turn_on
    bool cropValid;       // isValid
    byte selectedCrop;    // index
};

SystemState systemState; // Variable para almacenar el estado del sistema

// Estructura para parámetros óptimos de cultivo
// Define los rangos ideales para cada tipo de cultivo:
// - minTemp: Temperatura mínima recomendada (°C)
// - maxTemp: Temperatura máxima recomendada (°C)
// - minHumidity: Humedad mínima del suelo (%)
struct CropParameters {
    float minTemp;
    float maxTemp;
    float minHumidity;
};

CropParameters cropParameters; // Variable para almacenar los parámetros del cultivo

// Struct clave valor de los cultivos

struct keyValue {
  String crop;
  byte structIndex;
};

// Se comporta como un diccionario clave valor, donde la clave es el nombre del cultivo y el valor es el indice del cultivo
keyValue cropList [] = {
  {"Cilantro", 0},
  {"Fresa", 1},
  // {"Arroz", 2},
  // {"Tomate", 3},
  // {"Zanahoria", 4}
};

// Se obtiene el tamaño de la lista de cultivos dividiendo el tamaño total del array cropList entre el tamaño del struct keyValue
byte sizeCropList = sizeof(cropList) / sizeof(cropList[1]);

// FIN ASIGNACIÓN DE VARIABLES

// Asignación de pines de la pantalla lcd
              // RS E DB4 DB5 DB6 DB7 
LiquidCrystal lcd(0, 1, 2, 3, 4, 5);

// Configuración del teclado matricial
// Matriz de teclas
const char keys[ROWS][COLS] = {
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

// Función para mostrar mensajes en la pantalla LCD
// Parámetros:
// - message1: Texto para la primera línea (16 caracteres max)
// - message2: Texto para la segunda línea (opcional)
// - row1: Número de fila para message1 (0 o 1)
// - row2: Número de fila para message2 (0 o 1)
// Efectos:
// - Limpia la pantalla
// - Muestra los mensajes en las posiciones especificadas
// - Añade un pequeño delay para legibilidad
void showSelectionMessage(String message1, String message2 = "", byte row1 = 0, byte row2 = 1)
{
  lcd.clear();
  lcd.setCursor(0, row1);
  lcd.print(message1);
  lcd.setCursor(0, row2);
  lcd.print(message2);

  delay(DELAY_150_MS);
}

// Inicialización de la pantalla lcd
void initLCD()
{
  lcd.begin(16, 2);
  showSelectionMessage("Sistema de riego");
  delay(DELAY_2_SEG);
  lcd.clear();
  showSelectionMessage("Iniciando...");
  delay(DELAY_2_SEG);
  lcd.clear();
}

// Función para recorrer la lista de cultivos
void showMenu() {

  showSelectionMessage("Seleccione un", "cultivo valido");
  delay(DELAY_2_SEG);

  for (byte i = 0; i < sizeCropList; i++) {

    showSelectionMessage("Cultivo " + String(cropList[i].structIndex + 1), cropList[i].crop); // Se suma 1 para que el indice se muestre en 1 en lugar de 0
    delay(DELAY_2_SEG);
    lcd.clear();
  }
}

// Función de validación si el cultivo seleccionado se encuentra en el rango de cultivos
// Parámetros:
// - selection: Número de cultivo seleccionado
// Retorna:
// - true: Si la selección está dentro del rango válido
// - false: Si la selección es inválida
bool isValidCropSelection(byte selection)
{
  return (selection >= 1 && selection <= sizeCropList); 
}

// Función para procesar la selección del cultivo
void processCropSelection(byte selection)
{
    byte index = selection - 1;
    showSelectionMessage("Ud selecciono: ", cropList[index].crop);
    delay(DELAY_2_SEG);

    lcd.clear();
    
    showSelectionMessage("Cargando...");
    delay(DELAY_2_SEG);
    
    lcd.clear();
    systemState.cropValid = true;
}

// Función para seleccionar el cultivo
void selectCrop()
{
  // Variable para almacenar la elección del usuario
  char option;

  while (!systemState.cropValid) // Mientras no se presione una tecla seguimos esperando mostrando un mensaje de seleccionar un cultivo
  {
  // Se muestra un mensaje en la pantalla
    showSelectionMessage("Seleccione un", "cultivo valido");
    option = key.getKey();
    systemState.selectedCrop = (option - '0'); // Se resta el valor ASCII de '0' para obtener el valor numérico de la tecla presionada
    if (option != NO_KEY)
    {
      byte selection = option - '0';
      if (isValidCropSelection(selection))
      {
        processCropSelection(selection);
        return;
      }
      else
      {
        showSelectionMessage("Selecc invalida");
        delay(DELAY_2_SEG);
      }
      delay(DELAY_150_MS);
    }
  }
}

// Se crea una función para asignar el cultivo que el usuario seleccionó
// Parámetros:
// - option: Número de cultivo que va desde el 1 hasta el tamaño de la lista de cultivos que puede variar
// Efectos:
// - Actualiza cropParameters con los valores óptimos para el cultivo
// - Los valores incluyen rangos de temperatura y humedad ideal
void selectCrop(byte option) {
  switch (option) {
    case 1: // Cilantro
    cropParameters.minTemp = 3.0;
    cropParameters.maxTemp = 12.0;
    cropParameters.minHumidity = 4.0;
    break;

    case 2: // Fresa
    cropParameters.minTemp = 10.0;
    cropParameters.maxTemp = 25.0;
    cropParameters.minHumidity = 35.0;
    break;

    // case 3: // Arroz
    // cropParameters.minTemp = 10.0;
    // cropParameters.maxTemp = 25.0;
    // cropParameters.minHumidity = 10.0;
    // break;

    // case 4: // Tomate
    // cropParameters.minTemp = 10.0;
    // cropParameters.maxTemp = 25.0;
    // cropParameters.minHumidity = 10.0;
    // break;

    // case 5: // Zanahoria
    // cropParameters.minTemp = 15.0;
    // cropParameters.maxTemp = 35.0;
    // cropParameters.minHumidity = 35.0;
    // break;

    default:
    break;
  } 
}

// Función para leer la temperatura
float readTemperature() {
    return ((analogRead(TMP_SENSOR) * 5.0 / 1023.0) * 100.0) - 50; // Restándole 50 al resultado para coincidir en Tinkercad
}

// Función para leer la humedad
float readHumidity() {
    return (analogRead(HUM_SENSOR) * 5.0 / 1023.0) * 100.0;
}

// Función para imprimir los datos leídos
void printData()
{
  showSelectionMessage("Temp: " + (String(systemState.sensorReadings.temperature) + " C"), "Humedad: " + (String(systemState.sensorReadings.humidity) + " %"));
}

// Función para recibir el rango de la temperatura y la humedad y así controlar el motor de riego
bool receiveRange(float tmp ,float hum)
{
  if ((tmp >= cropParameters.minTemp && tmp <= cropParameters.maxTemp) && hum <= cropParameters.minHumidity)
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

  // Llamada a la función initLCD
  initLCD();

  // Se pregunta al usuario por el cultivo a regar mostrando un menú
  showMenu();

  // Mientras no se presione una tecla seguimos esperando mostrando un mensaje de seleccionar un cultivo
  // Y no se puede avanzar hasta que se selecciona un cultivo válido
  
  do {
    // Se llama a la función selectCrop para seleccionar el cultivo
    selectCrop();

  } while (systemState.cropValid == false);

  // Enviamos el parámetro seleccionado por el usuario
  selectCrop(systemState.selectedCrop);

}

// ========= BUCLE LOOP =========
// Función principal de control de riego
// Lógica:
// 1. Lee valores actuales de sensores
// 2. Verifica si están fuera de rangos óptimos
// 3. Muestra datos en pantalla
// 4. Activa/desactiva el riego según sea necesario
// Se ejecuta en loop continuo con delay de 1 segundo

void loop() {

  systemState.sensorReadings.update(); // Se actualizan los datos del sensor

  // Se recibe el rango de la temperatura y la humedad, si es true se enciende el motor de riego de lo contrario se apaga
  systemState.motorActive = receiveRange(systemState.sensorReadings.temperature, systemState.sensorReadings.humidity);

  // Impresión de los datos leídos en la pantalla lcd
  printData();

  // Se activa el motor de riego si se cumple la condición de la función receiveRange
  controlIrrigation(systemState.motorActive);

  delay(DELAY_1_SEG); // Delay de 1 segundo antes de hacer la siguiente lectura
}