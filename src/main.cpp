#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_I2CDevice.h>
#include <RTClib.h>
#include <OneWire.h>

#define DEBUG
#define ANCHO_PANTALLA 128 // OLED display width, in pixels
#define ALTO_PANTALLA 32   // OLED display height, in pixels
#define PINPULSADOR 4
#define PIN_TERMOMETRO 2
// Declaracion para SSD1306 display conectador por I2C (SDA, SCL pins)
// Los pines I2C están definidos en la biblioteca Wire.
// Para arduino UNO, Nano:       A4(SDA), A5(SCL)
#define RESET_PANTALLA -1       // Reset pin # (or -1 if sharing Arduino reset pin)
#define DIRECCION_PANTALLA 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

// Declaración funciones ///////////////////////////////////////////////////////
void mostrarPantalla();

/////////////////////// Declaración variables //////////////////////////////////
Adafruit_SSD1306 pantalla(ANCHO_PANTALLA, ALTO_PANTALLA, &Wire, RESET_PANTALLA);

RTC_DS1307 reloj;
DateTime tiempo;
OneWire termometro(PIN_TERMOMETRO);

bool pulsado = false;
bool estadoPulsador;
bool leerHora = false;

char cadenaFormateada[9];

byte datos[9];
byte termometro_addr[8];
byte termometro_cfg;
int16_t temperatura;
float celsius, fahrenheit;

void setup()
{
#if defined(DEBUG)
  Serial.begin(115200);
#endif

  pinMode(PIN_TERMOMETRO, INPUT);

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
  Serial.println(F("Pantalla configurada en modo texto."));
#endif
  // Inicializamos el reloj.
  if (!reloj.begin())
  {
    // Error inicializando el relor. Entramos en un bucle infinito.
    pantalla.println(F("Error en DS1307"));
    pantalla.display();
    while (1)
      delay(10);
  }
  else
  {
    // Compilamos la fecha y hora del momento de la compilación
    reloj.adjust(DateTime(F(__DATE__), F(__TIME__)));
#if defined(DEBUG)
    Serial.println(F("Reloj funcionando."));
#endif
  }

  tiempo = reloj.now();
// Pantalla bienvenida.
  pantalla.println(F("   Curso"));
  pantalla.println(F("  Arduino"));
  pantalla.display();

  // Buscamos el termómetro y guardamos la dirección.
  if (!termometro.search(termometro_addr))
  {
    Serial.println(F("Termómetro no encontrado."));
    Serial.println();
    termometro.reset_search();
    delay(250);
  }
  else
  {
#if defined(DEBUG)
    Serial.println(F("Termómetro OK"));
#endif
  }
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
      mostrarPantalla();
    }
  }
}

// Muestra en pantalla la temperatura y la hora de forma alternativa.
void mostrarPantalla()
{
  if (leerHora)
  {
    // Mostramos la hora en pantalla
    tiempo = reloj.now();
    // Guardamos en un char array la hora formateada como HH:MM:SS
    sprintf(cadenaFormateada, "%02u:%02u:%02u", tiempo.hour(), tiempo.minute(), tiempo.second());
    pantalla.clearDisplay();
    pantalla.setCursor(16, 10);
    // pantalla.setTextSize(2);
    pantalla.println(cadenaFormateada);
    pantalla.display();
  }
  else
  {
    // Hay que resetear antes de enviar una orden al termómetro.
    termometro.reset();
    // Seleccionamos el termómetro indicando su dirección.
    termometro.select(termometro_addr);
    // Enviamos la orden
    termometro.write(0x44, 1); // start conversion, with parasite power on at the end

    // Lanzamos la lectura de la temperatura.
    termometro.reset();
    termometro.select(termometro_addr);
    termometro.write(0xBE);
    // Hacemos una pausa antes de leer los datos recibidos.
    delay(1000);
    for (byte i = 0; i < 9; i++)
    { // Leemos 9 bytes
      datos[i] = termometro.read();
    }

    // Guardamos en una variable de 16 bits la tempreatura (sin formatear)
    temperatura = (datos[1] << 8) | datos[0];
    // Este byte nos indica los bit de resolución.
    termometro_cfg = (datos[4] & 0x60);

    if (termometro_cfg == 0x00)
      temperatura = temperatura & ~7; // 9 bit resolution, 93.75 ms
    else if (termometro_cfg == 0x20)
      temperatura = temperatura & ~3; // 10 bit res, 187.5 ms
    else if (termometro_cfg == 0x40)
      temperatura = temperatura & ~1; // 11 bit res, 375 ms

    // Cálculamos la temperatura en grados celsius.
    celsius = (float)temperatura / 16.0;
    pantalla.clearDisplay();
    pantalla.setCursor(16, 10);
    pantalla.print(celsius);
    
    // Símbolo º
    pantalla.setCursor(76,6);
    pantalla.write(9);

    pantalla.setCursor(93,10);
    pantalla.print("C");
    pantalla.display();
  }
}
