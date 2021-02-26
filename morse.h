#ifndef MORSE_H
#define MORSE_H


int przechwytywanie_sygnalow(float napiecie_L, float napiecie_H, float sygnal, int *tablica_sygnalow, int rozmiar_tablicy);
int dekoder_1_st(int *tablica_sygnalow,int *tablica_stanow, int ts_rozmiar);
int dekoder_2_st(char *tablica_znakow,int *tablica_stanow, int ts_rozmiar, char *baza_znakow);

#endif
/* Marginesy bledow zalezne od czestotliwosci */
