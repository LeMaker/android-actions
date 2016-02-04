#ifndef GAUGE_EQ27441_H_
#define GAUGE_EQ27441_H_

#define EQ27441_I2C_ADDR		(0x55)

int hw_gauge_get_capacity(int gauge_bus_id);
int bq27441_gasgauge_get_capacity(int gauge_bus_id);
int bq27441_gasgauge_get_voltage(int gauge_bus_id);

#endif

