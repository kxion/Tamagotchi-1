/*
 * tmp007.c
 *
 *  Created on: 28.9.2016
 *  Author: Teemu Leppanen / UBIComp / University of Oulu
 *
 *  Datakirja: http://www.ti.com/lit/ds/symlink/tmp007.pdf
 *  Aasigotchi Go by Markus Savusalo and Juho Sipola 16.12.2016
 */

#include <xdc/runtime/System.h>
#include <string.h>
#include "Board.h"
#include "tmp007.h"

I2C_Transaction i2cTransaction;
char txBuffer[4]; //Transmitting buffer
char rxBuffer[2]; //Receiving buffer

void tmp007_setup(I2C_Handle *i2c) {

	System_printf("TMP007: Config not needed!\n");
    System_flush();
}

void tmp007_get_data(I2C_Handle *i2c, double *temperature) {
    int i, Negative;

	/*DATA STRUCTURE TO ASK DATA*/
    i2cTransaction.slaveAddress = Board_TMP007_ADDR;
    txBuffer[0] = TMP007_REG_TEMP;
    i2cTransaction.writeBuf = txBuffer;
    i2cTransaction.writeCount = 1;
    i2cTransaction.readBuf = rxBuffer;
    i2cTransaction.readCount = 2;

	if (I2C_transfer(*i2c, &i2cTransaction)) {

        // SENSOR VALUES FROM RXBUFFER
		Raw_TMP_Data = (rxBuffer[0] << 8) | (rxBuffer[1]);

		// ACCORDING TO DATASHEET: GET TEMPERATURE
        *temperature = 0.0;
        Negative = Raw_TMP_Data & 0x8000;
        if (Negative) {
            Raw_TMP_Data = ~(Raw_TMP_Data)+1;
        }
        for (i=2; i<15; ++i) {
            int BitSet = (Raw_TMP_Data >> i) & 1;
            int exp = i-7;
            if (BitSet) {
                *temperature += pow(exp, 2);
            }
            if (Negative) {
                *temperature *= -1;
            }
        }
	    // Fahrenheit -> Celsius
	    *temperature = (*temperature - 32.0) * 5/9;
	    } else {
		System_printf("TMP007: Data read failed!\n");
		System_flush();
	}
}

