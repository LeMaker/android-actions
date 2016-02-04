/*
 * gauge_bq27441.c - EQ27441 driver
 *
 * Copyright (C) 2012, Actions Semiconductor Co. LTD.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <common.h>
#include <asm/io.h>
#include <asm/arch/gmp.h>
#include <i2c.h>
#include <power/gauge_bq27441.h>

int hw_gauge_get_capacity(int gauge_bus_id)
{
	return bq27441_gasgauge_get_capacity(gauge_bus_id);	
}

int bq27441_gasgauge_get_capacity(int gauge_bus_id)
{
	u8 data[2] = {0, 0};
	int ret = 0;
	int capacity = 0;
	printf("OWL_GAUGE_BUS_ID : %d\n", gauge_bus_id);
	I2C_SET_BUS(gauge_bus_id);

	ret = i2c_read(EQ27441_I2C_ADDR, 0x1c, 1, data, 2);
	if (ret) {
		printf("[bq27441] bq27441_gasgauge_get_capacity failed\n");
		return -1;
	}

	capacity = data[0] | (data[1]<<8);
	printf("[bq27441] capacity:%d\n", capacity);

	return capacity;
}

int bq27441_gasgauge_get_voltage(int gauge_bus_id)
{
	u8 data[2] = {0, 0};
	int ret = 0;
	int bat_v = 0;

	I2C_SET_BUS(gauge_bus_id);

	ret = i2c_read(EQ27441_I2C_ADDR, 0x4, 1, data, 2);
	if (ret) {
		printf("[bq27441] bq27441_gasgauge_get_voltage failed\n");
		return -1;
	}

	bat_v = data[0] | (data[1]<<8);
	printf("[bq27441] bat_v:%d\n", bat_v);

	return bat_v;

}

int bq27441_gasgauge_get_curr(int gauge_bus_id)
{
	u8 data[2] = {0, 0};
	int ret = 0;
	int bat_curr = 0;

	I2C_SET_BUS(gauge_bus_id);

	ret = i2c_read(EQ27441_I2C_ADDR, 0x10, 1, data, 2);
	if (ret) {
		printf("[bq27441] bq27441_gasgauge_get_curr failed\n");
		return -1;
	}

	bat_curr = data[0] | (data[1]<<8);
	if (data[1] & 0x80)
		bat_curr |= 0xFFFF0000;
	printf("[bq27441] bat_curr:%d\n", bat_curr);

	return bat_curr;

}
