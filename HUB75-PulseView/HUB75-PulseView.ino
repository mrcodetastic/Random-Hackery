
// MAKE SURE YOU ALWAYS HAVE CDC ON BOOT WHEN WRITING TO THE WF1!!!!

#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA-debug.h>
#include <esp_log.h>

MatrixPanel_I2S_DMA *dma_display = nullptr;

//#define WF1

uint8_t red = 255, green = 0, blue = 0; // Default color: Red
uint8_t brightness = 64; // Default brightness
bool textMode = false; // Toggle for text display

void updateDisplay() {
    dma_display->setBrightness8(brightness);
    dma_display->clearScreen();
    
    if (textMode) {
        dma_display->setCursor(5, 10);
        dma_display->setTextColor(dma_display->color565(red, green, blue));
        dma_display->setTextSize(1);
        dma_display->print("HELLO WORLD");
    } else {
        dma_display->fillScreenRGB888(red, green, blue);
    }

    dma_display->drawPixelRGB888(0, 0, 0, 255, 0);

    dma_display->drawPixelRGB888(dma_display->width()-1, dma_display->height()-1, 0, 0, 255);    
}


void setup() 
{

    Serial.begin(115200);
    Serial.setDebugOutput(true);
    delay(1000);

    // Default everything to WARN except my APP logs
    esp_log_level_set("*", ESP_LOG_WARN);
    esp_log_level_set("MyApp", ESP_LOG_DEBUG);
  
    // show what levels are supported
    ESP_LOGE("MyApp", "Error Reporting On");
    ESP_LOGW("MyApp", "Warning Reporting On");
    ESP_LOGI("MyApp", "Information Reporting On");
    ESP_LOGD("MyApp", "Debug Reporting On");
    ESP_LOGV("MyApp", "Verbose Reporting On");


    // HUB75 Pins
    #define WF1_R1_PIN 2
    #define WF1_R2_PIN 4
    #define WF1_G1_PIN 6
    #define WF1_G2_PIN 8
    #define WF1_B1_PIN 3
    #define WF1_B2_PIN 5

    #define WF1_A_PIN 39
    #define WF1_B_PIN 38
    #define WF1_C_PIN 37
    //#define WF1_D_PIN 36
    //#define WF1_E_PIN 12

    #define WF1_D_PIN -1
    #define WF1_E_PIN 36    

    #define WF1_OE_PIN 35
    #define WF1_CLK_PIN 34
    #define WF1_LAT_PIN 33


#ifdef WF1
        
    HUB75_I2S_CFG::i2s_pins _pins_x1 = {WF1_R1_PIN, WF1_G1_PIN, WF1_B1_PIN, WF1_R2_PIN, WF1_G2_PIN, WF1_B2_PIN, WF1_A_PIN, WF1_B_PIN, WF1_C_PIN, WF1_D_PIN, WF1_E_PIN, WF1_LAT_PIN, WF1_OE_PIN, WF1_CLK_PIN};

    // Standard panel type natively supported by this library (Example 1)
    HUB75_I2S_CFG mxconfig(
      16,   
      8,  
      1   ,
      _pins_x1 
    );

#else

    // Standard panel type natively supported by this library (Example 1)
    HUB75_I2S_CFG mxconfig(
      64,   
      32,  
      1   
    );

#endif

    // Ignore, custom mapping example

  //mxconfig.i2sspeed = HUB75_I2S_CFG::HZ_10M;
  mxconfig.clkphase = false;
  
  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->begin();
  dma_display->setBrightness8(64); //0-255
  dma_display->clearScreen();
  
  ////dma_display->drawPixelRGB888(0, 0, 255,255,0);

  dma_display->fillScreenRGB888(255,0,0);

}



void loop() {
  if (Serial.available()) {
        String input = Serial.readStringUntil('\n');
        input.trim(); // Remove any trailing spaces or newlines

         if (input.length() == 1) {
            if (input.charAt(0) == 't') {
                textMode = !textMode;
                Serial.println("Text mode toggled");
                updateDisplay();
            }
        } else if (input.length() > 1) {
            char cmd = input.charAt(0);
            int value = input.substring(1).toInt();
            
            if (value >= 0 && value <= 255) {
                switch (cmd) {
                    case 'r': 
                        red = value; 
                        Serial.print("Red set to ");
                        break;
                    case 'g': 
                        green = value; 
                        Serial.print("Green set to ");
                        break;
                    case 'b': 
                        blue = value; 
                        Serial.print("Blue set to ");
                        break;
                    case 'l': 
                        brightness = value; 
                        Serial.print("Brightness set to ");
                        break;
                    default: return; // Invalid command, ignore
                }
                Serial.println(value);
                updateDisplay();
            }
        }
    }
}

    