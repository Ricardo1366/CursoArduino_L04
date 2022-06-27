#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_I2CDevice.h>
#include <RTClib.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#define DEBUG
#define ANCHO_PANTALLA 128 // OLED display width, in pixels
#define ALTO_PANTALLA 32   // OLED display height, in pixels
#define PINPULSADOR 4
#define PIN_TERMOMETRO 5
#define PIN_INTERRUPCION 2
// Declaracion para SSD1306 display conectador por I2C (SDA, SCL pins)
// Los pines I2C están definidos en la biblioteca Wire.
// Para arduino UNO, Nano:       A4(SDA), A5(SCL)
#define RESET_PANTALLA -1       // Reset pin # (or -1 if sharing Arduino reset pin)
#define DIRECCION_PANTALLA 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

// Declaración funciones ///////////////////////////////////////////////////////
void mostrarPantalla();
void siguienteLectura();

/////////////////////// Declaración variables //////////////////////////////////
Adafruit_SSD1306 pantalla(ANCHO_PANTALLA, ALTO_PANTALLA, &Wire, RESET_PANTALLA);
RTC_DS1307 reloj;
DateTime tiempo;
OneWire termometro(PIN_TERMOMETRO);
DallasTemperature lecturaTemperatura(&termometro);

bool pulsado = false;
bool estadoPulsador;
bool leerHora = false;
volatile bool lectura;
char cadenaFormateada[9];

byte datos[9];
byte termometro_addr[8];
byte termometro_cfg;
int16_t temperatura;
float celsius;

void setup()
{
#if defined(DEBUG)
  Serial.begin(115200);
#endif

  // Inicializamos el reloj.
  if (!reloj.begin())
  {
    // Error inicializando el relor. Entramos en un bucle infinito.
    pantalla.println(F("Error en DS1307"));
    pantalla.display();
    for (;;)
      ;
  }
  else
  {
    // Compilamos la fecha y hora del momento de la compilación
    reloj.adjust(DateTime(F(__DATE__), F(__TIME__)));
#if defined(DEBUG)
    Serial.println(F("Reloj funcionando."));
#endif

    // Configuramos la señal del reloj (SQW) a 1 Hz.
    reloj.writeSqwPinMode(DS1307_SquareWave1HZ);
    
  }

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if (!pantalla.begin(SSD1306_SWITCHCAPVCC, DIRECCION_PANTALLA))
  {
#if defined(DEBUG)
    Serial.println(F("Error en la asignación de SSD1306"));
#endif
    for (;;)
      ; // Entramos en un bucle infinito.
  }
  else
  {
    // Pantalla iniciada correctamente. Continuamos.
#if defined(DEBUG)
    Serial.println(F("SSD1306 iniciado."));
#endif
  }

  // Mostrar pantalla de inicio de Adafruit.
  pantalla.display();
  delay(2000);
#if defined(DEBUG)
  Serial.println(F("Borramos pantalla."));
#endif
  pantalla.clearDisplay();
  pantalla.display();
  // Configuramos la pantalla.
  pantalla.setTextSize(2);              // Grande 2:1 pixel escala
  pantalla.setTextColor(SSD1306_WHITE); // Definir color blanco
  pantalla.setCursor(0, 0);             // Situamos el cursor en la parte superior izquierda de la pantalla.
  pantalla.cp437(true);                 // Habilitamos la página de códigos 437
#if defined(DEBUG)
  Serial.println(F("Pantalla configurada correctamente."));
#endif
  tiempo = reloj.now();
  // Pantalla bienvenida.
  pantalla.println(F("   Curso"));
  pantalla.println(F("  Arduino"));
  pantalla.display();
  lecturaTemperatura.begin();
  // Buscamos el termómetro y guardamos la dirección.
  if (!lecturaTemperatura.getAddress(termometro_addr, 0))
  {
    pantalla.println(F("Termómetro Err."));
    pantalla.display();
    for (;;)
      ;
  }
  else
  {
#if defined(DEBUG)
    Serial.println(F("Termómetro OK"));
#endif
  }
  // Configuramos la interrupción
  attachInterrupt(digitalPinToInterrupt(PIN_INTERRUPCION), siguienteLectura, RISING);
}

void loop()
{
  // Leemos el estado del pulsador
  estadoPulsador = !digitalRead(PINPULSADOR);
  if (pulsado)
  {
    // En la anterior lectura estaba pulsado. Comprobamos si se ha soltado.
    if (!estadoPulsador)
    {
      pulsado = false;
    }
  }
  else
  {
    // En la anterior lectura estaba sin pulsar. Comprobamos si se ha pulsado.
    if (estadoPulsador)
    {
      leerHora = !leerHora;
      pulsado = true;
    }
  }
  if (lectura)
  {
    mostrarPantalla();
  }
}

// Muestra en pantalla la temperatura y la hora de forma alternativa.
void mostrarPantalla()
{
  lectura = false;
  if (leerHora)
  {
    // Mostramos la hora en pantalla
    tiempo = reloj.now();

    // Guardamos en un char array la hora formateada como HH:MM:SS
    sprintf(cadenaFormateada, "%02u:%02u:%02u", tiempo.hour(), tiempo.minute(), tiempo.second());
    pantalla.clearDisplay();
    pantalla.setCursor(16, 10);
    pantalla.println(cadenaFormateada);
    pantalla.display();
  }
  else
  {
    lecturaTemperatura.requestTemperatures(); // Send the command to get temperatures

    celsius = lecturaTemperatura.getTempCByIndex(0);
    pantalla.clearDisplay();
    pantalla.setCursor(16, 10);
    pantalla.print(celsius);

    // Símbolo º
    pantalla.setCursor(76, 6);
    pantalla.write(9);

    pantalla.setCursor(93, 10);
    pantalla.print("C");
    pantalla.display();
  }
}

// Activamos la orden para la siguiente lectura.
void siguienteLectura()
{
  lectura = true;
}