
// MAKE SURE YOU ALWAYS HAVE CDC ON BOOT WHEN WRITING TO THE WF1!!!!

#include <Arduino.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <esp_log.h>

MatrixPanel_I2S_DMA *dma_display = nullptr;

//#define WF1
#define LIVEGRID

uint8_t red = 255, green = 0, blue = 0; // Default color: Red
uint8_t brightness = 64; // Default brightness
bool textMode = false; // Toggle for text display

void updateDisplay() {
    dma_display->setBrightness8(brightness);
    dma_display->clearScreen();
    
    if (textMode) {
        dma_display->setTextColor(dma_display->color565(red, green, blue));
        dma_display->setTextSize(1);
        dma_display->setCursor(4, 8);        
        dma_display->print("HELLO");
        dma_display->setCursor(14, 16);        
        dma_display->print("WORLD!");        
    } else {
        dma_display->fillScreenRGB888(red, green, blue);
    }

    //dma_display->drawPixelRGB888(0, 0, 0, 255, 0);

    //dma_display->drawPixelRGB888(dma_display->width()-1, dma_display->height()-1, 0, 0, 255);    
}


    // WF1 pin mapping
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


    //LiveGrid New PCB HUB75 Pin Mapping
    #define RL1 18
    #define GL1 17
    #define BL1 16
    #define RL2 15
    #define GL2 7
    #define BL2 6
    #define CH_A 4
    #define CH_B 10
    #define CH_C 14
    #define CH_D 21
    #define CH_E 5 // assign to any available pin if using two panels or 64x64 panels with 1/32 scan
    #define CLK 47
    #define LAT 48
    #define OE  38






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



#ifdef WF1
        
    HUB75_I2S_CFG::i2s_pins _pins_x1 = {WF1_R1_PIN, WF1_G1_PIN, WF1_B1_PIN, WF1_R2_PIN, WF1_G2_PIN, WF1_B2_PIN, WF1_A_PIN, WF1_B_PIN, WF1_C_PIN, WF1_D_PIN, WF1_E_PIN, WF1_LAT_PIN, WF1_OE_PIN, WF1_CLK_PIN};

    // Standard panel type natively supported by this library (Example 1)
    HUB75_I2S_CFG mxconfig(
      64,   
      32,  
      1   ,
      _pins_x1 
    );

#elif defined(LIVEGRID)

    HUB75_I2S_CFG::i2s_pins _pins={RL1, GL1, BL1, RL2, GL2, BL2, CH_A, CH_B, CH_C, CH_D, CH_E, LAT, OE, CLK};

    // Standard panel type natively supported by this library (Example 1)
    HUB75_I2S_CFG mxconfig(
      64,   
      32,  
      6   ,
      _pins 
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

  mxconfig.i2sspeed = HUB75_I2S_CFG::HZ_8M;
  //mxconfig.clkphase = false;
  
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
            else if (input.charAt(0) == 'l') {
                Serial.println("Lets draw a line! Exciting");

                for (int16_t x = 0; x < dma_display->width(); x++) {
                    updateDisplay();
                    dma_display->drawLine(x, 0, x, dma_display->height(), 0xFFFF);
                    delay(16);
                    Serial.print("x: ");                    
                    Serial.println(x);                    
                }                

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

    