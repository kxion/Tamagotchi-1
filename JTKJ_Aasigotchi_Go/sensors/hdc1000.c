/*
 * hdc1000.c
 *
 *  Created on: 22.7.2016
 *  Author: Teemu Leppanen / UBIComp / University of Oulu
 *
 * 	Datasheet http://www.ti.com/lit/ds/symlink/hdc1000.pdf
 */

#include <xdc/runtime/System.h>
#include <ti/sysbios/knl/Task.h>

#include "Board.h"
#include "hdc1000.h"

I2C_Transaction i2cTransaction;
char txBuffer[4];
char rxBuffer[4];

void hdc1000_setup(I2C_Handle *i2c) {

    i2cTransaction.slaveAddress = Board_HDC1000_ADDR;
    txBuffer[0] = HDC1000_REG_CONFIG;
    txBuffer[1] = 0x10; // sequential mode s.16
    txBuffer[2] = 0x00;
    i2cTransaction.writeBuf = txBuffer;
    i2cTransaction.writeCount = 3;
    i2cTransaction.readBuf = NULL;
    i2cTransaction.readCount = 0;

    if (I2C_transfer(*i2c, &i2cTransaction)) {

        System_printf("HDC1000: Config write ok\n");
    } else {
        System_printf("HDC1000: Config write failed!\n");
    }
    System_flush();

}

void hdc1000_get_data(I2C_Handle *i2c, double *hum) {

	/* Start temperature measurement */
	i2cTransaction.slaveAddress = Board_HDC1000_ADDR;
	txBuffer[0] = HDC1000_REG_TEMP;
	i2cTransaction.writeBuf = txBuffer;
	i2cTransaction.writeCount = 1;
	i2cTransaction.readBuf = NULL;
	i2cTransaction.readCount = 0;

	if (!I2C_transfer(*i2c, &i2cTransaction)) {

		System_printf("HDC1000: Trigger temp meas failed!\n");
		System_flush();
	}

	Task_sleep(10000);

	/* Start humidity measurement */
	i2cTransaction.slaveAddress = Board_HDC1000_ADDR;
	txBuffer[0] = HDC1000_REG_HUM;
	i2cTransaction.writeBuf = txBuffer;
	i2cTransaction.writeCount = 1;
	i2cTransaction.readBuf = NULL;
	i2cTransaction.readCount = 0;

	if (!I2C_transfer(*i2c, &i2cTransaction)) {

		System_printf("HDC1000: Trigger hum meas failed!\n");
		System_flush();
	}

	Task_sleep(10000);

	uint16_t raw_H = 0;

	/*DATA STRUCTURE TO ASK DATA*/
    i2cTransaction.slaveAddress = Board_HDC1000_ADDR;
    txBuffer[0] = HDC1000_REG_HUM;
    i2cTransaction.writeBuf = txBuffer;
    i2cTransaction.writeCount = 1;
    i2cTransaction.readBuf = rxBuffer;
    i2cTransaction.readCount = 2;

	if (I2C_transfer(*i2c, &i2cTransaction)) {

		// SENSOR VALUES FROM RXBUFFER
        raw_H = (rxBuffer[0] << 2) | (rxBuffer[1] >> 2);        //?

        // ACCORDING TO DATASHEET: GET HUMIDITY
	   *hum = ((raw_H / pow(2, 16))) * 100;                     //?

	} else {

		System_printf("HDC1000: Hum data read failed!\n");
		System_flush();
	}
}

