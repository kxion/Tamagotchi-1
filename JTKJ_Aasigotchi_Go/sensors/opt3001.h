/*
 * opt3001.h
 *
 *  Created on: 22.7.2016
 *  Author: Teemu Leppanen / UBIComp / University of Oulu
 *
 *  Datakirja: http://www.ti.com/lit/ds/symlink/opt3001.pdf
 *  Aasigotchi Go by Markus Savusalo and Juho Sipola 16.12.2016
 */

#ifndef OPT3001_H_
#define OPT3001_H_

#include <ti/drivers/I2C.h>

#define OPT3001_REG_RESULT		0x0
#define OPT3001_REG_CONFIG		0x1
#define OPT3001_DATA_READY		0x80

uint16_t Raw_OPT_Data;

void opt3001_setup(I2C_Handle *i2c);
void opt3001_get_data(I2C_Handle *i2c, double *bright);

#endif /* OPT3001_H_ */
