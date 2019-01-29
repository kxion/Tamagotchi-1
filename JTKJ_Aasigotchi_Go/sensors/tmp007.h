/*
 * tmp007.h
 *
 *  Created on: 28.9.2016
 *  Author: Teemu Leppanen / UBIComp / University of Oulu
 *
 *  Datakirja: http://www.ti.com/lit/ds/symlink/tmp007.pdf
 *  Aasigotchi Go by Markus Savusalo and Juho Sipola 16.12.2016
 */

#ifndef TMP007_H_
#define TMP007_H_

#include <ti/drivers/I2C.h>

#define TMP007_REG_TEMP	0x03

uint16_t Raw_TMP_Data;

void tmp007_setup(I2C_Handle *i2c);
void tmp007_get_data(I2C_Handle *i2c, double *temperature);

#endif /* TMP007_H_ */
