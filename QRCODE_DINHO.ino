
#include <Arduino.h>
#include "esp_camera.h"
#include <ESP32QRCodeReader.h>
#include <ESP32Servo.h>

#define SERVO_PIN 15  // Pino de sinal do servo motor (GPIO 14)

// Opcional: defina um texto esperado, ou deixe vazio "" para aceitar qualquer QR code valido
const String QR_ESPERADO = ""; 
const unsigned long TEMPO_BLOQUEIO_MS = 3000; // Evita repetir o mesmo QR code em seguida

ESP32QRCodeReader reader(CAMERA_MODEL_AI_THINKER);
Servo meuServo;

volatile bool acionando = false;
unsigned long ultimaLeituraMs = 0;
String ultimoConteudo = "";

void moverServo() {
  Serial.println("Acionando servo -> 90 graus...");
  meuServo.write(90);
  vTaskDelay(2000 / portTICK_PERIOD_MS);

  Serial.println("Retornando servo -> 0 graus...");
  meuServo.write(0);
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}

void onQrCodeTask(void *pvParameters) {
  struct QRCodeData qrCodeData;

  while (true) {
    if (!acionando && reader.receiveQrCode(&qrCodeData, 100)) {
      if (qrCodeData.valid) {
        String conteudo = String((char *)qrCodeData.payload);
        conteudo.trim();

        unsigned long agora = millis();
        bool mesmoQrRecente = (conteudo == ultimoConteudo) && (agora - ultimaLeituraMs < TEMPO_BLOQUEIO_MS);

        if (!mesmoQrRecente) {
          bool corresponde = (QR_ESPERADO.length() == 0) || (conteudo == QR_ESPERADO);

          if (corresponde) {
            acionando = true;
            ultimoConteudo = conteudo;
            ultimaLeituraMs = agora;

            Serial.println("==================================");
            Serial.print("QR Code Reconhecido: ");
            Serial.println(conteudo);
            Serial.println("==================================");

            moverServo();

            acionando = false;
          } else {
            Serial.print("QR Code ignorado (conteudo diferente): ");
            Serial.println(conteudo);
          }
        }
      }
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);

  // Evita conflito de timer PWM com a camera (que usa Timer 0)
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  meuServo.setPeriodHertz(50);
  meuServo.attach(SERVO_PIN, 500, 2400);
  meuServo.write(0);

  reader.setup();
  reader.beginOnCore(1);

  xTaskCreate(onQrCodeTask, "onQrCode", 4096, NULL, 4, NULL);

  Serial.println("Sistema pronto! Aguardando QR Code...");
}

void loop() {
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}