/*
 *
 *
 *  Created on: 29.10.2016
 *  Author: Teemu Leppanen / UBIComp / University of Oulu
 *
 *  Aasigotchi Go by Markus Savusalo and Juho Sipola 16.12.2016
 */
#include <stdio.h>
#include <string.h>
#include <wireless/comm_lib.h>
#include <stdint.h>
#include <time.h>
#include <math.h>

/* XDCtools files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/drivers/PIN.h>
#include <ti/drivers/pin/PINCC26XX.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/i2c/I2CCC26XX.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>
#include <ti/mw/display/Display.h>
#include <ti/mw/display/DisplayExt.h>

/* Board Header files */
#include "Board.h"
#include "wireless/comm_lib.h"
#include "sensors/tmp007.h"
#include "sensors/opt3001.h"
#include "sensors/bmp280.h"
#include "sensors/hdc1000.h"
#include "sensors/mpu9250.h"

// *******************************
//
// GLOBAL VARIABLES AND CONSTANTS
//
// *******************************

int n = 0, k = 0, i = 0, j = 0, num = 0;
char payload[80], *token;
const char s[2] = ",";
struct elukka {
    char nimi[16];
    uint8_t kuva[8];
    uint32_t liikunta;
    uint32_t aurinko;
    uint32_t ulkoilu;
    uint32_t sosiaalisuus;
} aasi1;

/* Task */
#define STACKSIZE 4096

char taskStack[STACKSIZE];
char taskCommStack[STACKSIZE];

/* Display */
Display_Handle hDisplay;

/* Pin Button1 configured as power/selection button */
static PIN_Handle hPowerButton;
static PIN_State sPowerButton;
PIN_Config cPowerButton[] = {
    Board_BUTTON1 | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE,
    PIN_TERMINATE
};
PIN_Config cPowerWake[] = {
    Board_BUTTON1 | PIN_INPUT_EN | PIN_PULLUP | PINCC26XX_WAKEUP_NEGEDGE,
    PIN_TERMINATE
};

/* Pin Button0 configured as menu/scrolling button */
static PIN_Handle hButton0;
static PIN_State sButton0;
PIN_Config cButton0[] = {
    Board_BUTTON0 | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE,
    PIN_TERMINATE
};

/* Leds */
static PIN_Handle hLed;
static PIN_State sLed;
PIN_Config cLed[] = {
    Board_LED0 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    Board_LED1 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    PIN_TERMINATE
};

/* MPU global variables */
static PIN_Handle hMpuPin;
static PIN_State MpuPinState;
static PIN_Config MpuPinConfig[] = {
    Board_MPU_POWER  | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    PIN_TERMINATE
};

// MPU9250 uses its own I2C interface
static const I2CCC26XX_I2CPinCfg i2cMPUCfg = {
    .pinSDA = Board_I2C0_SDA1,
    .pinSCL = Board_I2C0_SCL1
};


// *******************************
//
// PROTOTYPES
//
// *******************************

void powerButtonFxn(PIN_Handle handle, PIN_Id pinId);
void buttonFxn(PIN_Handle handle, PIN_Id pinId);
void draw(image);
void commFxn(UArg arg0, UArg arg1);
void taskFxn(UArg arg0, UArg arg1);
char *strtok(char *str, const char *delim);

// *******************************
//
// BUTTON HANDLES AND HANDLERS
//
// *******************************

/* Handle power button */
void powerButtonFxn(PIN_Handle handle, PIN_Id pinId) {
    k = 1;
    if (k == 1) {
        n = i+1;
        if (n>4) {
        n = 0;
        }
    }
    if (j==1){
        k=0;
    }
}

/* Nappien k�sittelij�funktio */
void buttonFxn(PIN_Handle handle, PIN_Id pinId) {
    if(pinId == Board_BUTTON0) {
        i++;
    }
}

// *******************************
//
// PICTURE DRAWING FUNCTION
//
// *******************************

void draw(image) {

    tContext *pContext = DisplayExt_getGrlibContext(hDisplay);
    if (pContext) {
        GrImageDraw(pContext, &image, 7, 7);
        GrFlush(pContext);
     }
}
// *******************************
//
// COMMUNICATION TASK
//
// *******************************

void commFxn(UArg arg0, UArg arg1){
    int l=0,o;
    char data[80];
    uint16_t senderAddr;
    char nimi[16], payload[80], tstr[10], bstr[10], pstr[10], mstr[10], sstr[10], fstr[10], estr[10];
    float ax, ay, az, gx, gy, gz;
	double temperature,pres,temp,bright;
	time_t start1, end1, start2, end2, start3, end3, aurinko1, liikunta, ulkoilu;

    // Aasigotchi
    strcpy(aasi1.nimi, "Benis:D");
    aasi1.kuva[8] = 0x8,0x14,0x14,0x14,0x14,0x83,0xFF;
    aasi1.aurinko = 0;
    aasi1.ulkoilu = 0;
    aasi1.sosiaalisuus = 0;

    // *******************************
    //
    // USE TWO DIFFERENT I2C INTERFACES
    //
    // *******************************
    I2C_Handle      i2c; // INTERFACE FOR OTHER SENSORS
    I2C_Params      i2cParams;
    I2C_Handle i2cMPU; // INTERFACE FOR MPU9250 SENSOR
	I2C_Params i2cMPUParams;

    // Create I2C for usage
    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_400kHz;

    I2C_Params_init(&i2cMPUParams);
    i2cMPUParams.bitRate = I2C_400kHz;
    i2cMPUParams.custom = (uintptr_t)&i2cMPUCfg;

    int32_t result = StartReceive6LoWPAN();
    if (result != true) {
        System_abort("Wireless receive start failed");
    }
    System_printf("Vierailu Commissa\n");
    System_flush();
    while(1){
        // Suoritetaan vaihtoehto 1: Gotchin lataus
        if (n == 1){
            n = 0;
            System_printf("Gotchin lataus valittu\n");
            System_flush();
            Display_print0(hDisplay,1,1,"Paina virta");
            Task_sleep(2000000/Clock_tickPeriod);
            Display_clear(hDisplay);

                Display_print0(hDisplay,1,1, "Lataus?");
                Task_sleep(5000000/Clock_tickPeriod);
                Display_clear(hDisplay);

                // Luodaan uusi gotchi
                    if (GetTXFlag() == false){
                        sprintf(nimi, "%s\n", aasi1.nimi);
                        System_printf(nimi);
                        System_flush();
                        sprintf(data, "Uusi:%u,%u,%u,%u,%u,%u,%u,%u,%s\n",aasi1.kuva[0],aasi1.kuva[1],aasi1.kuva[2],aasi1.kuva[3],aasi1.kuva[4],aasi1.kuva[5],aasi1.kuva[6],aasi1.kuva[7],nimi);
                        System_printf(data);
                        System_flush();
                        Send6LoWPAN(IEEE80154_SINK_ADDR, data, strlen(data));
                        System_printf("Nakki");
                        System_flush();
                        StartReceive6LoWPAN();
                        System_printf("vene\n");
                        System_flush();
                        Display_clear(hDisplay);
                    }
                    else if (GetRXFlag() == false){
                            for(o=0; o<6000; o++){
                                if (GetRXFlag() == true){
                                    System_printf("Keke\n");
                                    System_flush();
                                    if (Receive6LoWPAN(IEEE80154_SINK_ADDR, payload, 80) != -1) {
                                        Display_print0(hDisplay,1,1,payload);
                                        Task_sleep(2000000/Clock_tickPeriod);
                                        Display_clear(hDisplay);
                                        l=1;
                                    }
                                }
                                else{
                                    Display_print0(hDisplay,1,1, "Ei ollut");
                                    Task_sleep(5000000/Clock_tickPeriod);
                                    Display_clear(hDisplay);
                                    break;
                                }
                            }
                        }

                    }
            // Haetaan vanha gotchi
            if (l == 1){
                Display_print0(hDisplay,1,1,"Haetaan vanha");
                Task_sleep(2000000/Clock_tickPeriod);
                Display_clear(hDisplay);
                if (GetTXFlag() == false){
                    sprintf(nimi,"Leiki:%s\n",aasi1.nimi);
                    Send6LoWPAN(0xBEEF, nimi, 25);
                    Display_print0(hDisplay,1,1,"Odottaa");
                    StartReceive6LoWPAN();
                }
                else if (GetRXFlag() == true){
                    if (Receive6LoWPAN(IEEE80154_SINK_ADDR, payload, 80) != -1) {
                        Display_clear(hDisplay);
                        Display_print0(hDisplay,1,1,payload);
                        if(strstr(payload,"OK\n")){
                            scanf(payload,aasi1.kuva[0],aasi1.kuva[1],aasi1.kuva[2],aasi1.kuva[3],aasi1.kuva[4],aasi1.kuva[5],aasi1.kuva[6],aasi1.kuva[7],aasi1.liikunta,aasi1.aurinko,aasi1.ulkoilu,aasi1.sosiaalisuus,aasi1.nimi);

                        }
                        else{
                            Display_print0(hDisplay,1,1,"Tapahtui virhe!");
                            Task_sleep(2000000/Clock_tickPeriod);
                            Display_clear(hDisplay);
                        }
                    }
                }
            }

        // Suoritetaan vaihtoehto 2: Leikkiminen
        else if (n == 2){
            n = 0;
            System_printf("Leikkiminen valittu\n");
            System_flush();
            while(1){
                // MEASURE TIME FOR SUNLIGHT
                start1 = time (0);
                System_printf("Starting sunlight clock, start_t = %ld\n", start1);

                Display_print0(hDisplay,1,1,"Leikitaan");
                Task_sleep(2000000/Clock_tickPeriod);
                Display_clear(hDisplay);
                // OTHER SENSORS OPEN I2C
                i2c = I2C_open(Board_I2C0, &i2cParams);
                if (i2c == NULL) {
                    System_abort("Error Initializing I2C\n");
                }
                else {
                    System_printf("I2C Initialized!\n");
                }
                // SETUP OTHER SENSORS

                // TMP007 SETUP
                tmp007_setup(&i2c);
                // OPT3001 SETUP
                opt3001_setup(&i2c);
                // BMP280 SETUP
                bmp280_setup(&i2c);
                // HDC1000 SETUP
                hdc1000_setup(&i2c);

                // TMP007 ASK AND SHOW DATA
                tmp007_get_data(&i2c, &temperature);
                sprintf(tstr,"Temperature: %f C\n",temperature);
                System_printf(tstr);
                System_flush();
                Display_print0(hDisplay,1,1, "Temperature:");
                Task_sleep(2000000/Clock_tickPeriod);
                Display_clear(hDisplay);
                Display_print0(hDisplay,1,1, &temperature);
                Task_sleep(2000000/Clock_tickPeriod);
                Display_clear(hDisplay);

                // OPT3001 ASK AND SHOW DATA
                opt3001_get_data(&i2c, &bright);
                sprintf(bstr,"Brightness: %f Lux\n",bright);
                System_printf(bstr);
                System_flush();
                Display_print0(hDisplay,1,1, "Brightness:");
                Task_sleep(2000000/Clock_tickPeriod);
                Display_clear(hDisplay);
                Display_print0(hDisplay,1,1, &bright);
                Task_sleep(2000000/Clock_tickPeriod);
                Display_clear(hDisplay);

                // SUNLIGHT TIME
                end1 = time (0);
                aasi1.aurinko = end1 - start1;
                sprintf(sstr,"Aasigotchin auringonottotulos: %d sekuntia\n", aasi1.aurinko);
                System_printf(sstr);
                System_flush();
                Display_print0(hDisplay,1,1, "Sunlight time:");
                Task_sleep(2000000/Clock_tickPeriod);
                Display_clear(hDisplay);
                Display_print0(hDisplay,1,1, aasi1.aurinko);
                Task_sleep(2000000/Clock_tickPeriod);
                Display_clear(hDisplay);

                // MEASURE TIME FOR FRESH AIR
                start2 = time (0);

                // BMP280 ASK AND SHOW DATA
                bmp280_get_data(&i2c, &pres, &temp);
                sprintf(pstr,"Pressure: %f hPa\n",pres);
                System_printf(pstr);
                System_flush();
                Display_print0(hDisplay,1,1, "Pressure:");
                Task_sleep(2000000/Clock_tickPeriod);
                Display_clear(hDisplay);
                Display_print0(hDisplay,1,1, &pres);
                Task_sleep(2000000/Clock_tickPeriod);
                Display_clear(hDisplay);

                // FRESH AIR TIME
                end2 = time (0);
                aasi1.ulkoilu = end2 - start2;
                sprintf(fstr,"Aasigotchin ulkoilutulos: %d sekuntia\n", aasi1.ulkoilu);
                System_printf(fstr);
                System_flush();
                Display_print0(hDisplay,1,1, "Fresh air time:");
                Task_sleep(2000000/Clock_tickPeriod);
                Display_clear(hDisplay);
                Display_print0(hDisplay,1,1, aasi1.ulkoilu);
                Task_sleep(2000000/Clock_tickPeriod);
                Display_clear(hDisplay);

                // OTHER SENSORS CLOSE I2C
                I2C_close(i2c);

                // MPU OPEN I2C
                i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
                if (i2cMPU == NULL) {
                    System_abort("Error Initializing I2CMPU\n");
                }

                // MPU POWER ON
                PIN_setOutputValue(hMpuPin,Board_MPU_POWER, Board_MPU_POWER_ON);

                //MEASURE TIME FOR EXERCISE
                start3 = time (0);

                // WAIT 100MS FOR THE SENSOR TO POWER UP
                Display_print0(hDisplay,1,1, "Odota hetki:");
                Task_sleep(100000 / Clock_tickPeriod);
                System_printf("MPU9250: Power ON\n");
                System_flush();

                // MPU9250 SETUP AND CALIBRATION
                System_printf("MPU9250: Setup and calibration...\n");
                System_flush();
                mpu9250_setup(&i2cMPU);
                System_printf("MPU9250: Setup and calibration OK\n");
                System_flush();

                // MPU ASK AND SHOW DATA
                // Accelerometer values: ax,ay,az
                // Gyroscope values: gx,gy,gz
                mpu9250_get_data(&i2cMPU, &ax, &ay, &az, &gx, &gy, &gz);
                sprintf(mstr,"Acceleration: ax:%f, ay:%f, az:%f\n", ax,ay,az);
                System_printf(mstr);
                System_flush();
                sprintf(mstr,"Gyroscope: gx:%f, gy:%f, gz:%f\n",gx,gy,gz);
                System_printf(mstr);
                System_flush();
                Display_clear(hDisplay);
                Task_sleep(1000000/Clock_tickPeriod);
                Display_print0(hDisplay,1,1, "Acceleration:");
                Task_sleep(2000000/Clock_tickPeriod);
                Display_clear(hDisplay);
                Display_print0(hDisplay,1,1, &ax);
                Task_sleep(1000000/Clock_tickPeriod);
                Display_clear(hDisplay);
                Display_print0(hDisplay,1,1, &ay);
                Task_sleep(1000000/Clock_tickPeriod);
                Display_clear(hDisplay);
                Display_print0(hDisplay,1,1, &az);
                Task_sleep(1000000/Clock_tickPeriod);
                Display_clear(hDisplay);
                Display_print0(hDisplay,1,1, "Gyroscope:");
                Task_sleep(2000000/Clock_tickPeriod);
                Display_clear(hDisplay);
                Display_print0(hDisplay,1,1, &gx);
                Task_sleep(1000000/Clock_tickPeriod);
                Display_clear(hDisplay);
                Display_print0(hDisplay,1,1, &gy);
                Task_sleep(1000000/Clock_tickPeriod);
                Display_clear(hDisplay);
                Display_print0(hDisplay,1,1, &gz);
                Task_sleep(1000000/Clock_tickPeriod);
                Display_clear(hDisplay);

                // EXERCISE TIME
                end3 = time (0);
                aasi1.liikunta = end3 - start3;
                sprintf(estr,"Aasigotchin liikuntatulos: %d sekuntia\n", aasi1.liikunta);
                System_printf(estr);
                System_flush();
                Display_print0(hDisplay,1,1, "Exercise time:");
                Task_sleep(2000000/Clock_tickPeriod);
                Display_clear(hDisplay);
                Display_print0(hDisplay,1,1, aasi1.liikunta);
                Task_sleep(2000000/Clock_tickPeriod);
                Display_clear(hDisplay);
                System_printf("Leikkiminen leikitty\n");
                System_flush();
                Display_print0(hDisplay,1,1, "Leikki ohi\n");
                Task_sleep(2000000/Clock_tickPeriod);
                Display_clear(hDisplay);

                // MPU CLOSE I2C
                I2C_close(i2cMPU);

                // WAIT 100MS
                Task_sleep(100000 / Clock_tickPeriod);

                // MPU9250 POWER OFF
                PIN_setOutputValue(hMpuPin,Board_MPU_POWER, Board_MPU_POWER_OFF);
                break;
            }
        }
        // Suoritetaan vaihtoehto 3: Gotchi tervehtii muita gotcheja
        else if(n == 3){
            n = 0;
            System_printf("Kaverin morotus valittu\n");
            System_flush();
            if (GetTXFlag() == false){
                Send6LoWPAN(0xBEEA, "Terve",25);
                StartReceive6LoWPAN();
                Display_print0(hDisplay,1,1,"Etsitaan kamua");
                Task_sleep(10000000/Clock_tickPeriod);
            }
            for(o=0;o<6000;o++){
                if (GetRXFlag() == true){
                    System_printf("Testi");
                    System_flush();
                    if(Receive6LoWPAN(&senderAddr, payload, 80 != -1)){
                        Display_clear(hDisplay);
                        Display_print0(hDisplay,1,1,"Nahtiin kaveri");
                        Display_print0(hDisplay,2,1,payload);
                        System_printf("N�htiin kaveri\n");
                        aasi1.sosiaalisuus += 1;
                        System_flush();
                        Task_sleep(5000000/Clock_tickPeriod);
                        Display_clear(hDisplay);
                        break;
                    }
                }
            else {
                Display_print0(hDisplay,1,1, "Ei Kaveria");
                Task_sleep(2000000/Clock_tickPeriod);
                Display_clear(hDisplay);
                break;
            }
            }
        }
        // Suoritetaan vaihtoehto 4: Gotchi menee nukkumaan
        else if (n == 4) {
            n = 0;
            Display_clear(hDisplay);
            System_printf("Nukkuminen valittu\n");
            System_flush();

            if (GetTXFlag() == false){
                sprintf(data,"Nuku:%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%s\n",aasi1.kuva[0],aasi1.kuva[1],aasi1.kuva[2],aasi1.kuva[3],aasi1.kuva[4],aasi1.kuva[5],aasi1.kuva[6],aasi1.kuva[7],aasi1.liikunta,aasi1.aurinko,aasi1.ulkoilu,aasi1.nimi);
                System_printf(data);
                System_flush();
                Send6LoWPAN(IEEE80154_SINK_ADDR, data, strlen(data));
                StartReceive6LoWPAN();
                System_printf("Kiki\n");
                System_flush();
                Display_print0(hDisplay,1,1,"Yhteyden muodostus");
            }
            Display_print0(hDisplay,1,1,"Jonodus:DD");
            for(o=0;o<6000; o++){
                if (GetRXFlag() == true){
                    Display_print0(hDisplay,7,1,"Testi");
                    System_printf("Ei ole Gotchia\n");
                    System_flush();
                    if (Receive6LoWPAN(&senderAddr, payload, strlen(payload)) != -1){
                        System_printf("Gotchi meni nukkumaan\n");
                        System_flush();
                        Display_print0(hDisplay,1,1,"Gotchi nukkuu");
                        Task_sleep(1000000/Clock_tickPeriod);
                        Display_clear(hDisplay);
                        strcpy(aasi1.nimi, NULL);
                        aasi1.ulkoilu = 0;
                        aasi1.sosiaalisuus = 0;
                        aasi1.aurinko = 0;
                        aasi1.liikunta = 0;
                        break;
                    }
                }
            }
            if(GetRXFlag() == false){
                Display_print0(hDisplay,1,1,"Yhteytta ei saatu");
                Task_sleep(1000000/Clock_tickPeriod);
                Display_clear(hDisplay);
                break;
            }
        }
    }
}
    // *******************************
    //
    // MAIN TASK
    //
    // *******************************

void taskFxn(UArg arg0, UArg arg1) {


    // *******************************
	//
	// DISPLAY
	//
	// *******************************

    Display_Params displayParams;
	displayParams.lineClearMode = DISPLAY_CLEAR_BOTH;
    Display_Params_init(&displayParams);

    hDisplay = Display_open(Display_Type_LCD, &displayParams);
    if (hDisplay == NULL) {
        System_abort("Error initializing Display\n");
    }

    // *******************************
	//
	// PICTURE
	//
	// *******************************

	Display_clear(hDisplay);
    const uint8_t imgData[8] = {
        0x8, 0x14, 0x14, 0x14, 0x14, 0x63, 0x81, 0xFF
    };

    // Mustavalkoinen kuva: v�rit musta ja valkoinen
    uint32_t imgPalette[] = {0, 0xFFFFFF};

    // Kuvan m��rittelyt
    const tImage image = {
        .BPP = IMAGE_FMT_1BPP_UNCOMP,
        .NumColors = 2,
        .XSize = 1,
        .YSize = 8,
        .pPalette = imgPalette,
        .pPixel = imgData
    };

    tContext *pContext = DisplayExt_getGrlibContext(hDisplay);
    if (pContext) {
        GrImageDraw(pContext, &image, 50, 50);
        GrFlush(pContext);
     }
     Task_sleep(5000000/Clock_tickPeriod);

    // *******************************
	//
	// SCROLLING MENU
	//
	// *******************************

	System_printf("Ohjelma k�ynniss�\n");
	System_flush();
    Display_print0(hDisplay, 1,1, "Gotchi");
    Display_print0(hDisplay, 2,1, "kaynnissa");
    Task_sleep(1000000/Clock_tickPeriod);
    Display_clear(hDisplay);

    // Vaihtoehtojen esittely
    if (hDisplay){
    Display_print0(hDisplay,1,1, "Lataa gotchi?");
    Display_print0(hDisplay,2,1, "Leiki?");
    Display_print0(hDisplay,3,1, "Terve muille?");
    Display_print0(hDisplay,4,1, "Nukkumaan?");
    Display_print0(hDisplay,5,1, "Sammuta laite?");
    Task_sleep(3000000/Clock_tickPeriod);
    Display_clear(hDisplay);
    while(1){
        // Selausikkuna odottaa kun vaihtoehto on valittu
        if (j == 1){
        Task_sleep(45*1000000/Clock_tickPeriod);
        j =0;
        i=0;
        k=0;
        }
        //Piirr� kuva
        while(1){
        tContext *pContext = DisplayExt_getGrlibContext(hDisplay);
        if (pContext) {
            GrImageDraw(pContext, &image, 60, 50);
            GrImageDraw(pContext, &image, 60, 50);
            GrFlush(pContext);
        }
        Task_sleep(1000000/Clock_tickPeriod);
        Display_print0(hDisplay,6,1, aasi1.nimi);
            // Vaihtoehto 1: Gotchin lataus
            if (i == 0){
                Display_print0(hDisplay,1,1, "Lataa gotchi?");
                if (k==1){
                    Task_sleep(1000000/Clock_tickPeriod);
                    Display_clear(hDisplay);
                    System_printf("Ladataan gotchi\n");
                    System_flush();
                    Task_sleep(2000000/Clock_tickPeriod);
                    Display_clear(hDisplay);
                    j=1;
                    break;
                }
                else if (hButton0 == true){
                    Task_sleep(1000000/Clock_tickPeriod);
                    Display_clear(hDisplay);
                    i++;
    			}
            }
            // Vaihtoehto 2: Leikkiminen
            else if (i == 1){
                Display_print0(hDisplay, 1,1, "Leiki?");
                if (k == 1){
                    Display_clear(hDisplay);
                    j=1;
                    break;
                }
                else if (hButton0 == true){
                    Task_sleep(1000000/Clock_tickPeriod);
                    i++;
                }
            }
            // Vaihtoehto 3: Gotchi tervehtii muita
            else if (i == 2){
                Display_print0(hDisplay,1,1, "Terve muille?");
                if (k == 1){
                    //Task_sleep(1000000/Clock_tickPeriod);
                    Display_clear(hDisplay);
                    j=1;
                    break;
                }
                else if (hButton0 == true){
                    Task_sleep(1000000/Clock_tickPeriod);
                    i++;
                }
            }
            // Vaihtoehto 4: Gotchi menee nukkumaan
            else if (i == 3){
                Display_print0(hDisplay,1,1, "Nukkumaan?");
                if (k == 1){
                    Task_sleep(1000000/Clock_tickPeriod);
                    Display_clear(hDisplay);
                    Display_print0(hDisplay,1,1, "Gotchi nukkuu");
                    Task_sleep(1000000/Clock_tickPeriod);
                    Display_clear(hDisplay);
                    j=1;
                    break;
                }
                else if (hButton0 == true){
                    Task_sleep(1000000/Clock_tickPeriod);
                    i++;
                }
            }
            // Vaihtoehto 5: Laitteen sammuttaminen
            else if (i == 4){
                Display_print0(hDisplay,1,1, "Sammuta laite?");
                if (k == 1){
                    n = 0;
                    Display_clear(hDisplay);
                    Display_print0(hDisplay,1,1, "Laite sammuu");
                    System_printf("Sammutetaan laite\n");
    		        System_flush();
                    Task_sleep(1000000 / Clock_tickPeriod);
                    Display_clear(hDisplay);
                    Display_close(hDisplay);
                    Task_sleep(1000000 / Clock_tickPeriod);
                    PIN_close(hPowerButton);
                    PINCC26XX_setWakeup(cPowerWake);
                    Power_shutdown(NULL,0);
                }
                else if(hButton0 == true){
                    Task_sleep(1000000/Clock_tickPeriod);
                    Display_clear(hDisplay);
                    i++;
                }
            }
            else {
                i=0;
            }

    }
    }
}
}

// *******************************
//
// MAIN PROGRAM
//
// *******************************

int main(void) {

    // Task variables
	Task_Handle task;
	Task_Params taskParams;
	Task_Handle taskComm;
	Task_Params taskCommParams;

    // Initialize Board
    Board_initGeneral();
    Board_initI2C();

	/* Buttons */
	hPowerButton = PIN_open(&sPowerButton, cPowerButton);
	if(!hPowerButton) {
		System_abort("Error initializing button shut pins\n");
	}
	if (PIN_registerIntCb(hPowerButton, &powerButtonFxn) != 0) {
		System_abort("Error registering button callback function");
	}

	/* Open button pin */
   hButton0 = PIN_open(&sButton0, cButton0);
   if(!hButton0) {
      System_abort("Error initializing button pins\n");
   }
	if (PIN_registerIntCb(hButton0, &buttonFxn) != 0) {
		System_abort("Error registering button callback function");
	}

    /* Leds */
    hLed = PIN_open(&sLed, cLed);
    if(!hLed) {
        System_abort("Error initializing LED pin\n");
    }

    /* OPEN MPU POWER PIN */
    hMpuPin = PIN_open(&MpuPinState, MpuPinConfig);
    if (hMpuPin == NULL) {
    	System_abort("Pin open failed!");
    }

    /* Main Task */
    Task_Params_init(&taskParams);
    taskParams.stackSize = STACKSIZE;
    taskParams.stack = &taskStack;
    taskParams.priority=2;
    task = Task_create((Task_FuncPtr)taskFxn, &taskParams, NULL);
    if (task == NULL) {
    	System_abort("Task create failed!");
    }

    /* Communication Task */
    Init6LoWPAN();
    Task_Params_init(&taskCommParams);
    taskCommParams.stackSize = STACKSIZE;
    taskCommParams.stack = &taskCommStack;
    taskCommParams.priority=1;
    taskComm = Task_create(commFxn, &taskCommParams, NULL);
    if (taskComm == NULL) {
    	System_abort("Task create failed!");
    }
    System_printf("Hello world!\n");
    System_flush();

    /* Start BIOS */
    BIOS_start();

    return (0);
}
