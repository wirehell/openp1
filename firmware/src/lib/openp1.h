#ifndef OPENP1_HEADER_H
#define OPENP1_HEADER_H

#include <zephyr/types.h>

enum Format {
    DATE_TIME_STRING,
    DOUBLE_LONG_UNSIGNED_8_3,
    DOUBLE_LONG_UNSIGNED_4_3,
    LONG_UNSIGNED_3_1,
    LONG_SIGNED_3_1,
};

enum Item {
    DATE_TIME,
    METER_ACTIVE_ENERGY_IN,
    METER_ACTIVE_ENERGY_OUT,
    METER_REACTIVE_ENERGY_IN,
    METER_REACTIVE_ENERGY_OUT,
    ACTIVE_ENERGY_IN,
    ACTIVE_ENERGY_OUT,
    REACTIVE_ENERGY_IN,
    REACTIVE_ENERGY_OUT,
    _ITEM_COUNT,
};

enum Unit {
    NONE,
    K_WATT_HOUR,
    K_VOLT_AMPERE_HOUR_REACTIVE,
    K_WATT,
    K_VOLT_AMPERE_REACTIVE,
    VOLT,
    AMPERE,
    _UNIT_COUNT,
};

struct data_definition {
    enum Item item;
    char *obis;
    enum Format format;
    enum Unit unit;
};

extern const struct data_definition data_definition_table[];

#endif /* OPENP1_HEADER_H */