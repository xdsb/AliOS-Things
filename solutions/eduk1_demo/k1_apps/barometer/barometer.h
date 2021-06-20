#ifndef __BAROMETER_H__
#define __BAROMETER_H__

#include "../menu.h"

extern MENU_TYP barometer;

int  barometer_init(void);
int  barometer_uninit(void);
void barometer_task(void);

static uint8_t icon_data_tempF_16_16[] = {
    0x00, 0x00, 0x00, 0x38, 0x28, 0x38, 0x00, 0xF8, 0xF8, 0x88, 0x88,
    0x88, 0x88, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x1F, 0x1F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static icon_t icon_tempF_16_16 = {icon_data_tempF_16_16, 16, 16};

static uint8_t icon_data_tempC_16_16[] = {
    0x00, 0x00, 0x00, 0x38, 0x28, 0x38, 0x00, 0xE0, 0xF0, 0x18, 0x18,
    0x18, 0x30, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x07, 0x0F, 0x18, 0x18, 0x18, 0x0C, 0x04, 0x00, 0x00};
static icon_t icon_tempC_16_16 = {icon_data_tempC_16_16, 16, 16};

static uint8_t icon_data_atmp_16_16[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0xF8, 0x00, 0x00, 0xFC, 0x00, 0x00,
    0xF8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x21, 0x22, 0x27,
    0x20, 0x24, 0x2F, 0x24, 0x20, 0x27, 0x22, 0x21, 0x00, 0x00};
static icon_t icon_atmp_16_16 = {icon_data_atmp_16_16, 16, 16};

static uint8_t icon_data_asl_16_16[] = {
    0x00, 0x08, 0xDC, 0x08, 0x00, 0x00, 0xC0, 0x70, 0x38, 0xE0, 0x00,
    0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x36, 0x20, 0x2E, 0x27,
    0x21, 0x20, 0x20, 0x2F, 0x26, 0x23, 0x2E, 0x20, 0x00, 0x00};
static icon_t icon_asl_16_16 = {icon_data_asl_16_16, 16, 16};

#endif
