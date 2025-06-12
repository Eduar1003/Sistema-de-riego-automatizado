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
#define DELAY_150_MS 150   // Delay de 150 ms para respuesta del teclado
#define DELAY_1_SEG 1000   // Delay estándar de 1 segundo (en ms)
#define DELAY_2_SEG 2000   // Delay largo de 2 segundos (en ms)

#define TEMP_CALIBRATION_OFFSET -50 // Ajuste de calibración para el sensor TMP36
#define ADC_MAX_VALUE 1023 // Valor máximo del ADC
#define VCC 5.0 // Voltaje de alimentación

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
      temperature = ((analogRead(TMP_SENSOR) * VCC / ADC_MAX_VALUE) * 100.0) + TEMP_CALIBRATION_OFFSET;
      humidity = (analogRead(HUM_SENSOR) * VCC / ADC_MAX_VALUE) * 100.0;
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
// - maxHumidity: Humedad máxima del suelo (%)
// Estos parámetros se utilizan para determinar cuándo activar el riego
struct CropParameters {
    float minTemp;
    float maxTemp;
    float minHumidity;
    float maxHumidity;
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
  // {"Arroz", 2}, // Descomentando estas líneas se puede agregar más cultivos
  // {"Tomate", 3},
  // {"Zanahoria", 4}
};

// Se obtiene el tamaño de la lista de cultivos dividiendo el tamaño total del array cropList entre el tamaño del struct keyValue
byte sizeCropList = sizeof(cropList) / sizeof(cropList[0]);

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

// ======== PROTOTIPOS DE FUNCIONES ========
void initLCD();
void showSelectionMessage(String, String = "", byte = 0, byte = 1);
void showMenu();
bool isValidCropSelection(byte);
void processCropSelection(byte);
void selectCrop();
void addCropParameters(byte);
float readTemperature();
float readHumidity();
void printData();
bool receiveRange(float, float);
void controlIrrigation(bool);

// ======== CONFIGURACIÓN INICIAL ========
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
  addCropParameters(systemState.selectedCrop);

}

// ======== BUCLE PRINCIPAL ========
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

// ======== FUNCIONES DE HARDWARE ========
// --- LCD ---
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

void showSelectionMessage(String message1, String message2, byte row1, byte row2)
{
  lcd.clear();
  lcd.setCursor(0, row1);
  lcd.print(message1);
  lcd.setCursor(0, row2);
  lcd.print(message2);

  delay(DELAY_150_MS);
}

// ======== FUNCIONES DE LÓGICA ========
// --- Menú y selección ---
void showMenu() {

  showSelectionMessage("Seleccione un", "cultivo");
  delay(DELAY_2_SEG);

  for (byte i = 0; i < sizeCropList; i++) {

    showSelectionMessage("Cultivo " + String(cropList[i].structIndex + 1), cropList[i].crop); // Se suma 1 para que el indice se muestre en 1 en lugar de 0
    delay(DELAY_2_SEG);
    lcd.clear();
  }
}

bool isValidCropSelection(byte selection)
{
  return (selection >= 1 && selection <= sizeCropList); 
}

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
    // Si se presiona una tecla y es un dígito, se verifica si es una selección válida
    // Si no es una selección válida, se muestra un mensaje de selección inválida
    // Si se presiona una tecla y no es un dígito, se ignora la tecla

      if (isValidCropSelection(systemState.selectedCrop))
      {
        addCropParameters(systemState.selectedCrop); // Se agregan los parámetros del cultivo seleccionado
        systemState.cropValid = true; // Se marca el cultivo como válido
        if (isValidCropSelection(systemState.selectedCrop))
        {
          processCropSelection(systemState.selectedCrop); // Procesa la selección del cultivo
          return;
        }
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

// Agrega los parámetros del cultivo seleccionado

void addCropParameters(byte option) {
  switch (option) {
    case 1: // Cilantro
    cropParameters.minTemp = 15.0;
    cropParameters.maxTemp = 24.0;
    cropParameters.minHumidity = 40.0;
    cropParameters.maxHumidity = 50.0;
    break;

    case 2: // Fresa
    cropParameters.minTemp = 15.0;
    cropParameters.maxTemp = 20.0;
    cropParameters.minHumidity = 60.0;
    cropParameters.maxHumidity = 80.0;
    break;

    // Descomentando estas líneas se pueden agregar más cultivos con sus respectivos parámetros
    // Debe corresponder con el índice del cropList

    // case 3: // Arroz
    // cropParameters.minTemp = 10.0;
    // cropParameters.maxTemp = 25.0;
    // cropParameters.minHumidity = 10.0;
    // cropParameters.maxHumidity = 100.0;
    // break;

    // case 4: // Tomate
    // cropParameters.minTemp = 10.0;
    // cropParameters.maxTemp = 25.0;
    // cropParameters.minHumidity = 10.0;
    // cropParameters.maxHumidity = 100.0;
    // break;

    // case 5: // Zanahoria
    // cropParameters.minTemp = 15.0;
    // cropParameters.maxTemp = 35.0;
    // cropParameters.minHumidity = 35.0;
    // cropParameters.maxHumidity = 100.0;
    // break;

    default:
    break;
  } 
}

// ======== FUNCIONES DE SENSORES ========
float readTemperature() {
    return ((analogRead(TMP_SENSOR) * 5.0 / 1023.0) * 100.0) - 50; // Restándole 50 al resultado para coincidir en Tinkercad
}

float readHumidity() {
    return (analogRead(HUM_SENSOR) * 5.0 / 1023.0) * 100.0;
}

void printData()
{
  showSelectionMessage("Temp: " + (String(systemState.sensorReadings.temperature) + " C"), "Humedad: " + (String(systemState.sensorReadings.humidity) + " %"));
}

// ======== FUNCIONES DE CONTROL ========
bool receiveRange(float tmp ,float hum)
{
  if (tmp < -20 || tmp > 100)
  {
    showSelectionMessage("Rango de", "temp invalida");
    delay(DELAY_2_SEG);
    return false; // Valores inválidos de los sensores
  }

  if (hum < 0 || hum > 100)
  {
    showSelectionMessage("Rango de", "humedad invalida");
    delay(DELAY_2_SEG);
    return false; // Valores inválidos de los sensores
  }

  if ((tmp >= cropParameters.minTemp && tmp <= cropParameters.maxTemp) && (hum <= cropParameters.minHumidity))
    return true;

  else if ((tmp >= cropParameters.minTemp && tmp <= cropParameters.maxTemp) && (hum <= cropParameters.maxHumidity))
    return true;

  else
    return false;
}

void controlIrrigation(bool shouldActivateMotor)
{
  if (shouldActivateMotor == true)
    digitalWrite(IRRIGATION_MOTOR, HIGH);

  else
    digitalWrite(IRRIGATION_MOTOR, LOW);
}