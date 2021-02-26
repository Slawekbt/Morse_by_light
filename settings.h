#ifndef SETTINGS_H
#define SETTINGS_H

void menu_powitalne(float stan_L, float stan_H, int pit_okres);
void commands(void);
void running_config(float stan_L, float stan_H, float stan_tlo, int pit_period);
float vlt_set_high(char* rx_buf, float stan_H);
float vlt_set_low(char* rx_buf, float stan_L);

int strcmp_wildcard(const char *string_pattern, const char* string_input);
int char2int_wildcard(const char *string_pattern, const char* string_input); // dla danych wejsiowych typu "text***"

#endif
/* Marginesy bledow zalezne od czestotliwosci */
