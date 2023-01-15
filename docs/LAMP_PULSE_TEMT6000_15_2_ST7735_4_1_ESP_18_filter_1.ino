/*


https://mysku.club/blog/diy/90736.html

*/


//#include <UTFTGLUE.h>              //use GLUE class and constructor
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <GyverFilters.h>

#define ANALOG_IN_PIN A0 //A5  //Light Sensor pin

#define ENV_MEASURE_PIN D4 //9 //button

#define TOP_HEIGHT_DISPLAY 40
#define BOTTOM_HEIGHT_DISPLAY 60
#define LCD_X_WIDTH 127
#define LCD_Y_HEIGHT 159

#define LIMIT_MAX_ENV_ILLUMINANCE 80 //1023
#define LIMIT_MIN_ILLUMINANCE 150 //1023
#define LIMIT_MAX_ILLUMINANCE 950 //1023

#define ENV_NUMBER_CALIBRATION_MEASURES 500 //one measure per ENV_MEASURE_INTERVAL
#define ENV_MEASURE_INTERVAL 1 //ms

#define PRINT_INFO_INTERVAL 1000 //ms

#define X_POS_PRINT 3

//UTFTGLUE myGLCD(0,A2,A1,A3,A4,A0); //all dummy args

// TFT display and SD card will share the hardware SPI interface.
// Hardware SPI pins are specific to the Arduino board type and
// cannot be remapped to alternate pins.  For Arduino Uno,
// Duemilanove, etc., pin 11 = MOSI, pin 12 = MISO, pin 13 = SCK.
//#define SD_CS    4  // Chip select line for SD card
#define TFT_CS   D2 //10  // Chip select line for TFT display
#define TFT_DC   D1 //8  A0  // Data/command line for TFT 
#define TFT_RST  D6 //-1  // Reset line for TFT (or connect to +5V)

#define ST7735_DARKGRAY 0x7BEF

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

long timer_1 = 0;
long timer_2 = 0;
long timer_3 = 0;
long timer_4 = 0;
long timer_5 = 0;

float a_val = 0.0;
int n = 0;
int m = 0;

int x = 0;
int y = 0;

bool arr_filled = false;

//int point_arr[400] = {}; //2.4"
int point_arr[160] = {}; //1.8"
int mid_point_arr[160] = {};
int point_arr_filt[160] = {};

byte n_rise = 0;
byte n_fall = 0;

byte x1_min = 0;
byte x1_max = 0;

int y_1 = 0;
int y_2 = 0;
int y_3 = 0;

float kp1 = 0;
float kp2 = 0;

long sum_E = 0;
int env_E = 0; //5
long env_E_sum = 0;
int Emax = 0;
int Emin = 0;

int delta_y = 40;
int y_min = 0;

int Emax_sum = 0;
int Emin_sum = 0;
int sum_E_sum = 0;
int n_m = 0;

long timer_print = 0;

int y_1_1 = 0;
int y_1_2 = 0;

int y_2_1 = 0;
int y_2_2 = 0;

//GFilterRA analog0;
GKalman kalman(90, 90, 0.5);
GFilterRA average(0.5, 80);

void setup()
{

    WiFi.mode(WIFI_OFF);
    WiFi.forceSleepBegin();

    Serial.begin(115200);
    pinMode(ANALOG_IN_PIN,  INPUT);
    pinMode(ENV_MEASURE_PIN, INPUT_PULLUP);

    randomSeed(analogRead(0));
    /*
    // Setup the LCD
    myGLCD.InitLCD();
    myGLCD.setFont(BigFont); //SmallFont
    myGLCD.setTextSize(5);

    //myGLCD.setColor(127,127,127);
    myGLCD.setColor(0, 0, 0);
    myGLCD.fillRect(0, 0, 319, 239);*/

    // Initialize 1.8" TFT 128x160
    tft.initR(INITR_BLACKTAB);   // initialize a ST7735S chip, black tab
    tft.setRotation(0);

    Serial.println("1.8 TFT OK!");
    tft.fillScreen(ST7735_BLACK);

    tft.setTextSize(1); //1 2 3
    tft.setTextColor(ST7735_WHITE, ST7735_BLACK);


    tft.setCursor(30, 0);
    tft.print("LAMPTEST.RU");

    tft.setCursor(0, 150);
    tft.print("Flicker test v0.18_f");

    tft.setCursor(15, 75);
    tft.print("Auto calibration");


    delay(1000);

    Serial.println("Start measure Eenv. It will take about " + String(ceil(ENV_NUMBER_CALIBRATION_MEASURES / 1000.0)) + " seconds");
    //Eenv measure
    while (m < ENV_NUMBER_CALIBRATION_MEASURES)
    {
        if (millis() > (timer_5 + ENV_MEASURE_INTERVAL))
        {
            timer_5 = millis();
            m++;
            //env_E_sum += analogRead(ANALOG_IN_PIN);
            int a_val = analogRead(ANALOG_IN_PIN);
            env_E_sum += a_val;
            Serial.println(a_val);
        }
    }
    env_E = int((float) env_E_sum / (float) ENV_NUMBER_CALIBRATION_MEASURES);
    Serial.println("env_E_sum: " + String(env_E_sum) + " env_E: " + String(env_E));

    tft.fillScreen(ST7735_BLACK);

    //  analog0.setCoef(1.0); // установка коэффициента фильтрации (0.0... 1.0). Чем меньше, тем плавнее фильтр
    //  analog0.setStep(312.5); // установка шага фильтрации (мс). Чем меньше, тем резче фильтр


}

long start_time = 0;
//long finish_time = 0;

void loop()
{

    //1.8" 128x160 128/4=32*5=160  4 periods x 32 points = 128 pts   100 Hz x 32 values   1000000/3200 = 312.5 mcs
    //2.4" 320x240 320/4=80*5=400  4 periods x 80 points = 320 pts   100 Hz x 80 values   1000000/8000 = 125 mcs


    //if (micros() > (timer_2 + 125))
    if (micros() > (timer_2 + 312.5))
    {
        timer_2 = micros();
        //if (x < 400)
        if (x < 160)
        {
            point_arr[x] = analogRead(ANALOG_IN_PIN);
            //point_arr_filt[x] = analog0.filteredTime(point_arr[x]);
            //point_arr_filt[x] = (int)kalman.filtered(point_arr[x]);
            point_arr_filt[x] = (int)average.filtered(point_arr[x]);
            if (x > 0)
            {
                mid_point_arr[x] = (point_arr[x - 1] + point_arr[x]) / 2;
            }
            x++;
        }
        else
        {
            arr_filled = true;
            x = 0;
            start_time = millis();
        }
    }

    if (arr_filled == true)
    {
        /*if (digitalRead(ENV_MEASURE_PIN) == LOW)
        {
          if (millis() > (timer_4 + 1000))
          {
            timer_4 = millis();
            env_E = analogRead(ANALOG_IN_PIN);
          }
        }*/

        //find max min
        sum_E = point_arr[0];
        x1_max = 0;
        x1_min = 0;
        //for (int x1 = 1; x1 <80; x1++)
        for (int x1 = 1; x1 < 32; x1++)
        {
            sum_E += point_arr[x1] - env_E;

            if (point_arr[x1] > point_arr[x1_max])
            {
                x1_max = x1;
            }

            if (point_arr[x1] < point_arr[x1_min])
            {
                x1_min = x1;
            }
        }

        delta_y = 40 * ceil((point_arr[x1_max] - point_arr[x1_min]) / 40.0);
        y_min = point_arr[x1_max] - delta_y;
        //Serial.println(y_min);
        if (y_min < 0)
        {
            y_min = 0;

        }

        /*
        _mid = point_arr[x1_max]
        point_arr[x1_min]
        sum_E_mid
        */
        Emax_sum += point_arr[x1_max];
        Emin_sum += point_arr[x1_min];
        sum_E_sum += sum_E;
        n_m++;



        if (millis() > (timer_print + PRINT_INFO_INTERVAL))
        {
            timer_print = millis();
            //tft.fillRect(0, 49, 128, 50, ST7735_BLACK);
            //1.8" TFT 128x160    point_arr[x1_max]   point_arr[x1_min]
            tft.setCursor(X_POS_PRINT, 50);
            //if (kp1 < 0)
            //tft.print("Emax: " + String((point_arr[x1_max] - env_E)) + "   " + String(point_arr[x1_max]) + " " + String(point_arr[x1_min]) + "   ");
            //if < 0  =0 and red
            //Emax = point_arr[x1_max] - env_E;
            //Emax = point_arr[x1_max];
            Emax = Emax_sum / n_m;
            //if (Emax < 0)
            if (Emax < env_E)
            {
                //Emax = 0;
                tft.setTextColor(ST7735_RED, ST7735_BLACK);
            }
            else
            {
                tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
            }
            //tft.print("Emax: " + String((point_arr[x1_max] - env_E)) + "   " + String(point_arr[x1_max]) + " " + String(y_min) + "   ");
            //tft.print("Emax: " + String(Emax) + " " + String(point_arr[x1_max]) + " " + String(y_min)); // + "         "
            tft.print("Emax: " + String(Emax) + "    " + String(y_min) + "       "); // + "         "
            //tft.print("Emax: " + String(Emax) + "   "); // + "         "

            //Emin = point_arr[x1_min] - env_E;
            //Emin = point_arr[x1_min];
            Emin = Emin_sum / n_m;
            //if (Emin < 0)
            if (Emin < env_E)
            {
                //Emin = 0;
                tft.setTextColor(ST7735_RED, ST7735_BLACK);
            }
            else
            {
                tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
            }
            tft.setCursor(X_POS_PRINT, 60);
            //tft.print("Emin: " + String((point_arr[x1_min] - env_E)) + "    ");
            ///tft.print("Emin: " + String(Emin) + "   ");
            if ((Emax < 150) || (Emax > 950))
            {
                tft.print("Emin: " + String(Emin)); // + " "
            }
            else
            {
                tft.print("Emin: " + String(Emin) + "   ");
            }

            //      if (Emax < 150)
            //      {
            //        tft.setCursor(68, 60);
            //        tft.print("Low Light ");
            //      } else if ((Emax >= 150) && (Emax <= 950)) {
            //        tft.setCursor(68, 60);
            //        tft.print("          ");
            //      } else if (Emax > 950) {
            //        tft.setCursor(68, 60);
            //        tft.print("Over Light");
            //      }

            tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
            tft.setCursor(X_POS_PRINT, 70);
            tft.print("Eenv: " + String(env_E) + "   ");

            //if < 0  =0 and red
            //kp1 = 100.0*(point_arr[x1_max] - point_arr[x1_min])/(point_arr[x1_min] + point_arr[x1_max] - 2*env_E);
            kp1 = 100.0 * (Emax - Emin) / (Emax + Emin - 2 * env_E);
            if ((kp1 < 0) || (kp1 > 100))
            {
                kp1 = 0.0;
                //red color
                tft.setTextColor(ST7735_RED, ST7735_BLACK);
            }
            else
            {
                tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
            }
            tft.setCursor(X_POS_PRINT, 80);
            tft.print("kp1: " + String(kp1, 1) + " %   ");

            tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
            //kp2 = 100.0*(point_arr[x1_max] - point_arr[x1_min])/(2*sum_E/32);
            kp2 = 100.0 * (Emax - Emin) / (2 * (sum_E_sum / n_m) / 32); //
            if (kp2 < 0.0)
            {
                kp2 = 0.0;
                //red color
                tft.setTextColor(ST7735_RED, ST7735_BLACK);
            }
            else
            {
                tft.setTextColor(ST7735_WHITE, ST7735_BLACK);
            }
            tft.setCursor(X_POS_PRINT, 90);
            tft.print("kp2: " + String(kp2, 1) + " %   "); // + String(point_arr[x1_max]) + " 0   "

            tft.setTextColor(ST7735_WHITE, ST7735_BLACK);

            Emax_sum = 0;
            Emin_sum = 0;
            sum_E_sum = 0;
            n_m = 0;
        }

        if (Emax < 150)
        {
            tft.setCursor(68, 60);
            tft.print("Low Light ");
        }
        else
            if ((Emax >= 150) && (Emax <= 950))
            {
                tft.setCursor(68, 60);
                tft.print("          ");
            }
            else
                if (Emax > 950)
                {
                    tft.setCursor(68, 60);
                    tft.print("Over Light");
                }

        /*
        //2.4" LCD TFT 8-bit UNO
        //1023/1000 = 0.9775171
        myGLCD.print("Emax: " + String((point_arr[x1_max] - env_E)*0.9775171, 1) + " lx   ", 5, 15);
        myGLCD.print("Emin: " + String((point_arr[x1_min] - env_E)*0.9775171, 1) + " lx   ", 5, 35);
        myGLCD.print("Eenv: " + String(env_E*0.9775171, 1) + " lx   ", 5, 55);

        kp1 = 100.0*(point_arr[x1_max] - point_arr[x1_min])/(point_arr[x1_min] + point_arr[x1_max] - 2*env_E);
        myGLCD.print("kp1: " + String(kp1, 1) + " %  ", 5, 75);
        kp2 = 100.0*(point_arr[x1_max] - point_arr[x1_min])/(2*sum_E/80);
        myGLCD.print("kp2: " + String(kp2, 1) + " %  ", 5, 95);
        */

        //Y 0 - 40    100 - 159
        //for (int x2 = 0; x2 <320; x2++)
        for (int x2 = 0; x2 < 128; x2++)
        {
            //1.8"
            //tft.height()-1
            tft.drawLine(x2, 40, x2, 0, ST7735_BLACK); //ST77XX_YELLOW
            //tft.drawPixel(x2, 40 - (point_arr[x2 + x1_min] - env_E)/((1023 - env_E)/40), ST7735_WHITE); // 1000 - 0 / 40 = 25

            ///y_1 = 40 - int( (point_arr[x2 + x1_min] - point_arr[x1_min]) / ( (point_arr[x1_max] - point_arr[x1_min])/40.0) );
            ///tft.drawPixel(x2, y_1, ST7735_DARKGRAY);
            //y_1 = 40 - int((point_arr[x2 + x1_min] - 40*int(point_arr[x1_min]/40.0))/( (point_arr[x1_max] - 40*int(point_arr[x1_min]/40.0) )/40.0));

            //delta_y = 40*ceil((point_arr[x1_max] - point_arr[x1_min])/40.0);

            //y_min = point_arr[x1_max] - delta_y;

            //y_1 = 40 - int( ((point_arr[x2 + x1_min] - y_min)/delta_y)*20.0 );
            y_1 = 40 - int(((point_arr[x2 + x1_min] - y_min) / (delta_y / 40.0)));
            //Serial.println("A: " + String(point_arr[x2 + x1_min] - y_min) + " delta " + String(delta_y) + " " + String(((point_arr[x2 + x1_min] - y_min)/delta_y)*40.0));
            //y_1 = 40 - int( ((point_arr[x2 + x1_min] - y_min)/delta_y)*80.0 );
            //y_1 = int( ((point_arr[x2 + x1_min] - y_min)/delta_y)*40.0 );

            if (x2 > 1)
            {
                y_1_1 = 40 - int(((mid_point_arr[x2 + x1_min - 1] - y_min) / (delta_y / 40.0)));    //int(mid_point_arr[x2 + x1_min - 1]/17.07);
            }
            y_1_2 = 40 - int(((mid_point_arr[x2 + x1_min] - y_min) / (delta_y / 40.0)));

            if (y_1_1 < 0)
            {
                y_1_1 = 0;
            }
            if (y_1_1 > 40)
            {
                y_1_1 = 40;
            }

            if (y_1_2 < 0)
            {
                y_1_2 = 0;
            }
            if (y_1_2 > 40)
            {
                y_1_2 = 40;
            }

            if (y_1 < 0)
            {
                y_1 = 0;
            }
            if (y_1 > 40)
            {
                y_1 = 40;
            }
            if (x2 > 1)
            {
                tft.drawLine(x2 - 1, y_1_1, x2 - 1, y_1_2, ST7735_WHITE);
            }
            tft.drawPixel(x2, y_1, ST7735_BLUE); // 1000 - 0 / 40 = 25
            ///tft.drawPixel(x2, y_1, ST7735_WHITE); // 1000 - 0 / 40 = 25
            //tft.drawLine(x2, y_mid1, x2, y_mid2, ST7735_WHITE);

            // 1024/60 = 17.07
            y_2 = 159 - int(point_arr[x2 + x1_min] / 17.07);
            y_2_1 = 159 - int(mid_point_arr[x2 + x1_min - 1] / 17.07);
            y_2_2 = 159 - int(mid_point_arr[x2 + x1_min] / 17.07);

            if (y_2_1 < 100)
            {
                y_2_1 = 100;
            }
            if (y_2_1 > 159)
            {
                y_2_1 = 159;
            }

            if (y_2_2 < 100)
            {
                y_2_2 = 100;
            }
            if (y_2_2 > 159)
            {
                y_2_2 = 159;
            }

            if (y_2 < 100)
            {
                y_2 = 100;
            }
            if (y_2 > 159)
            {
                y_2 = 159;
            }
            tft.drawLine(x2, 159, x2, 100, ST7735_BLACK); //ST77XX_YELLOW
            if (x2 > 1)
            {
                tft.drawLine(x2 - 1, y_2_1, x2 - 1, y_2_2, ST7735_WHITE);
            }
            tft.drawPixel(x2, y_2, ST7735_BLUE); // 1000/60 = ~17   20  1024/60 = 17.07

            tft.drawPixel(x2, 159 - env_E / 17.07, ST7735_BLUE);

            //filtered values
            y_3 = 159 - int(point_arr_filt[x2 + x1_min] / 17.07);
            tft.drawPixel(x2, y_3, ST7735_RED); // 1000/60 = ~17   20  1024/60 = 17.07
            /*
            //2.4"
            myGLCD.setColor(0,0,0);
            myGLCD.drawLine(x2, 239, x2, 120);
            //myGLCD.setColor(127, 127, 127);
            myGLCD.setColor(255, 255, 255);
            myGLCD.drawPixel(x2, 239 - point_arr[x2 + x1_min]/10);
            //myGLCD.drawPixel(240 - point_arr[y], y+1);
            */

            //Serial.println(" " + String(point_arr[x2 + x1_min]));
        }

        //color rectangle
        if (kp1 < 5.0)
        {
            //tft.setColor(0, 0, 0); //green
            tft.fillRect(0, 42, 128, 5, ST7735_GREEN);
        }
        else
            if ((kp1 >= 5.0) && (kp1 < 20.0))
            {
                //tft.setColor(0, 0, 0); //yellow
                tft.fillRect(0, 42, 128, 5, ST7735_YELLOW);
            }
            else
                if ((kp1 >= 20.0) && (kp1 < 40.0))
                {
                    //tft.setColor(0, 0, 0); //orange
                    tft.fillRect(0, 42, 128, 5, ST7735_ORANGE);
                }
                else
                    if (kp1 >= 40.0)
                    {
                        //tft.setColor(0, 0, 0); //red
                        tft.fillRect(0, 42, 128, 5, ST7735_RED);
                    }
        //tft.fillRect(0, 40, 128, 5, ST7735_RED);

        arr_filled = false;
        long cycle_time = millis() - start_time;
        Serial.println(cycle_time);
    }

}
