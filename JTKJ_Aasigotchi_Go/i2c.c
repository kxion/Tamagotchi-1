    int n = 0;
    uint16_t senderAddr = SERVERADDR;
    char nimi[16], payload[80], tstr[10], bstr[10], pstr[10], hstr[10], mstr[10], str[10];
    float ax, ay, az, gx, gy, gz;
	double temperature,pres,temp,bright,hum;

    // *******************************
    //
    // USE TWO DIFFERENT I2C INTERFACES
    //
    // *******************************
   /* I2C_Handle      i2c; // INTERFACE FOR OTHER SENSORS
    I2C_Params      i2cParams;
    I2C_Handle i2cMPU; // INTERFACE FOR MPU9250 SENSOR
	I2C_Params i2cMPUParams;

    // Create I2C for usage
    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_400kHz;

    I2C_Params_init(&i2cMPUParams);
    i2cMPUParams.bitRate = I2C_400kHz;
    i2cMPUParams.custom = (uintptr_t)&i2cMPUCfg;

    // *******************************
    //
    // MPU OPEN I2C
    //
    // *******************************
    i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
    if (i2cMPU == NULL) {
        System_abort("Error Initializing I2CMPU\n");
    }

    // *******************************
    //
    // MPU POWER ON
    //
    // *******************************
    PIN_setOutputValue(hMpuPin,Board_MPU_POWER, Board_MPU_POWER_ON);

    // WAIT 100MS FOR THE SENSOR TO POWER UP
	Task_sleep(100000 / Clock_tickPeriod);
    System_printf("MPU9250: Power ON\n");
    System_flush();

    // *******************************
    //
    // MPU9250 SETUP AND CALIBRATION
    //
    // *******************************
	System_printf("MPU9250: Setup and calibration...\n");
	System_flush();

	mpu9250_setup(&i2cMPU);

	System_printf("MPU9250: Setup and calibration OK\n");
	System_flush();

    // *******************************
    //
    // MPU CLOSE I2C
    //
    // *******************************
    I2C_close(i2cMPU);

    // *******************************
    //
    // OTHER SENSOR OPEN I2C
    //
    // *******************************

    i2c = I2C_open(Board_I2C0, &i2cParams);
    if (i2c == NULL) {
        System_abort("Error Initializing I2C\n");
    }
    else {
        System_printf("I2C Initialized!\n");
    }

    // *******************************
    //
    // SETUP OTHER SENSORS
    //
    // *******************************

    // TMP007 SETUP
    tmp007_setup(&i2c);

    // OPT3001 SETUP
    opt3001_setup(&i2c);

    // BMP280 SETUP
    bmp280_setup(&i2c);

    // HDC1000 SETUP
    hdc1000_setup(&i2c);

    // *******************************
    //
    // OTHER SENSOR CLOSE I2C
    //
    // *******************************
    I2C_close(i2c);

    // **********************************************
	//
	// LOOP FOR SENSOR OPENING AND DATA RETRIEVAL
	//
	// **********************************************

	    // *******************************
	    //
	    // OTHER SENSORS OPEN I2C
	    //
	    // *******************************
	    i2c = I2C_open(Board_I2C, &i2cParams);
	    if (i2c == NULL) {
	        System_abort("Error Initializing I2C\n");
	    }
	    // *******************************
	    //
	    // TMP007 ASK DATA
	    //
	    // *******************************
	    tmp007_get_data(&i2c, &temperature);

    	sprintf(tstr,"temperature %f\n",temperature);
    	System_printf(str);
    	System_flush();

    	// *******************************
	    //
	    // OPT3001 ASK DATA
	    //
	    // *******************************
	    opt3001_get_data(&i2c, &bright);

    	sprintf(bstr,"brightness %f\n",bright);
    	System_printf(str);
    	System_flush();

	    // *******************************
	    //
	    // BMP280 ASK DATA
	    //
	    // *******************************
	    bmp280_get_data(&i2c, &pres, &temp);

    	sprintf(pstr,"pres %f, temp %f\n",pres,temp);
    	System_printf(str);
    	System_flush();

    	// *******************************
	    //
	    // HDC1000 ASK DATA
	    //
	    // *******************************
	    /*hdc1000_get_data(&i2c, &hum);

    	sprintf(hstr,"%f\n",hum);
    	System_printf(str);
    	System_flush();

	    // *******************************
	    //
	    // OTHER SENSORS CLOSE I2C
	    //
	    // *******************************
	    I2C_close(i2c);

	    // *******************************
	    //
	    // MPU OPEN I2C
	    //
	    // *******************************
	    i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
	    if (i2cMPU == NULL) {
	        System_abort("Error Initializing I2CMPU\n");
	    }

	    // *******************************
	    //
	    // MPU ASK DATA
		//
        //    Accelerometer values: ax,ay,az
	 	//    Gyroscope values: gx,gy,gz
		//
	    // *******************************
		mpu9250_get_data(&i2cMPU, &ax, &ay, &az, &gx, &gy, &gz);

    	sprintf(mstr,"ax %f, ay %f, az %f, gx %f, gy %f, gz %f\n", ax,ay,az,gx,gy,gz);
    	System_printf(str);
    	System_flush();

	    // *******************************
	    //
	    // MPU CLOSE I2C
	    //
	    // *******************************
	    I2C_close(i2cMPU);

	    // WAIT 100MS
    	Task_sleep(100000 / Clock_tickPeriod);


	// MPU9250 POWER OFF
	// Because of loop forever, code never goes here
    PIN_setOutputValue(hMpuPin,Board_MPU_POWER, Board_MPU_POWER_OFF);*/
