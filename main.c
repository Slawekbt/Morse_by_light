/*-------------------------------------------------------------------------
					Technika Mikroprocesorowa 2 - laboratorium
					Projekt czytnika kodow morse'a
					autor: Slawomir Tenerowicz
					wersja: 25.01.2021r.
----------------------------------------------------------------------------*/
					
#include "MKL05Z4.h"
#include "ADC.h"
#include "frdm_bsp.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "pit.h"
#include "uart.h"

#include "morse.h"
#include "settings.h"

/* ***** */
/* Wzory komend */
const char string_pattern_commands[]= {'c','o','m','m','a','n','d','s',0x0};
const char string_pattern_running_config[]= {'r','u','n','n','i','n','g',' ','c','o','n','f','i','g',0x0};

const char string_pattern_vlt_set_l[]= {'v','l','t',' ','s','e','t',0x20,'l','o','w',' ','*','*','*','*',0x0};
const char string_pattern_vlt_set_h[]= {'v','l','t',' ','s','e','t',0x20,'h','i','g','h',' ','*','*','*','*',0x0};
const char string_pattern_vlt_show[]= {'v','l','t',' ','s','h','o','w',0x0};

const char string_pattern_pit_set_period[]= {'p','i','t',' ','s','e','t',' ','p','e','r','i','o','d',' ','*','*','*',0x0};
const char string_pattern_pit_show[]= {'p','i','t',' ','s','h','o','w',0x0};

const char string_pattern_morse_run[]= {'m','o','r','s','e',' ','r','u','n',0x0};
const char string_pattern_morse_debug[]= {'m','o','r','s','e',' ','d','e','b','u','g',0x0};

const char string_pattern_set_dot_marigin[]= {'s','e','t',' ','d','o','t',0x20,'m','a','r','i','g','i','n','*','*',0x0};

/* ***** */
/* Tablica do dekodowania znakow w kodzie morse'a -- kolejnosc nieprzypadkowa */

char baza_znakow[30]={'?','e','t','i','n','a','m','s','d','r','g' ,'u' ,'k' ,'w' ,'o' ,'h' ,'b' ,'l' ,'z' ,'f' ,'c' ,'p' ,'?' ,'v' ,'x' ,'?' ,'q' ,'?' ,'y' ,'j' };
//  numeracja        {'0','1','2','3','4','5','6','7','8','9','10','11','12','13','14','15','16','17','18','19','20','21','22','23','24','25','26','27','28','29'} 


/* ***** */
/* Zmienne globalne do obslugi sygnalow z ADC */
	
float volt_coeff = ((float)(((float)2.91) / 4095) ) * 1000;			//  Korekcja wyniku napieciowego [mV]
uint8_t stan_adc = 0;	// 0 - odczyt rejestru ADC0->R[] w przerwaniu ADC, 1 - usrednianie w przerwaniu PIT, 2 - odczyt w petli glownej
float	wynik = 0;			// Aktualny wynik przetwornika ADC usredniony przy uzyciu PIT, aktualizowany co 100 ms --> patrz pit.c
float	wynik_last = 0; // Do przechowywania ostatniej wartosci odczytanej przez ADC, do wyswietlania przez UART na zyczenie
int global_counter = 0;
	
/* ***** */
/* Flagi opisujace kolejne stany w programie */
uint8_t etap_0 = 1; // ustawienia
uint8_t etap_1 = 0; // czytanie sygnalow morse'a
uint8_t etap_2 = 0; // dekodowanie 1st
uint8_t etap_3 = 0; // dekodowanie 2st


/* ***** */
/* Zmienne globalne do obslugi funkcji kodu morse'a */
	
int tablica_sygnalow[300]; // zliczenia stanow L i H
int tsig_rozmiar = sizeof(tablica_sygnalow);

int tablica_stanow[300];	// kod dla stanow L i H
int stan_zapisu = 0; 			// liczba zapisanych stanow (L,H) do tablicy stanow

char tablica_znakow[30];	// kod wyjsciowy -- ciagi liter
int ilosc_znakow = 0;			// liczba znakow w tablicy znakow	

int morse_run_flag = 0;  // jesli w programie przynajmniej raz zostala wywolana komenda morse run, morse_run_flag = 1

float stan_L = 400;			 // wartosci odczytane z czujnika swiatla mniejsze niz stan_L uznawane sa za niskie
float stan_H = 600;			 // wartosci odczytane z czujnika swiatla mniejsze niz stan_L uznawane sa za niskie

/* ***** */
/* Zmienne globalne do obslugi PIT */
int pit_period = 10;		// poczatkowa wartosc okresu licznika pit [ms]

/* ***** */
/* Zmienne globalne wspierajace funkcje UART */

char rx_buf[32] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};	// bufor do obslugi danych odbieranych przez UART
int rx_buf_len = sizeof(rx_buf);
int znacznik_transmisji = 0;



/* ***** */
/* Inne zmienne globalne */

int i = 0;							// do iteratorow
int display_flag = 1;		// do wyswietlania komunikatow
char sprintf_buf[64];   // do konwersji tekstu przy pomocy sprintf()


/* ***** */
/* Funkcje obslugi przerwan */

void ADC0_IRQHandler()
{	
	static uint16_t temp = 0;
	temp = ADC0->R[0];			// Odczyt danej i skasowanie flagi COCO
	if(stan_adc == 0)				// Sprawdź, czy wynik skonsumowany przez pętlę główną
	{
		wynik += temp;				// Wyślij nową daną do pętli głównej
		global_counter ++;

	}
	NVIC_EnableIRQ(ADC0_IRQn);
}

void PIT_IRQHandler() {
		
	if (PIT->CHANNEL[0].TFLG & PIT_TFLG_TIF_MASK) {

		PIT->CHANNEL[0].TFLG &= PIT_TFLG_TIF_MASK;
		
		// Do PIT work, every pit_period [ms]
		stan_adc = 1;										// przerwij odczyty ADC0->R[0] z adc
		wynik = wynik / global_counter;
		
	}

	//clear pending IRQ
	NVIC_ClearPendingIRQ(PIT_IRQn);
	
}

void UART0_IRQHandler()
{
	znacznik_transmisji = UART_Read(znacznik_transmisji, rx_buf, rx_buf_len);
}

/* ***** */
/*  MAIN */
/* ***** */

int main (void)
{
	uint8_t	kal_error;
	PIT_Init(pit_period);							// PIT_Init(int period) [ms]
	
	kal_error=ADC_Init();				// Inicjalizacja i kalibracja przetwornika A/C
	if(kal_error)
	{	
		while(1);									// Klaibracja się nie powiodła
	}
	
	ADC0->SC1[0] = ADC_SC1_AIEN_MASK | ADC_SC1_ADCH(12);		// Pierwsze wyzwolenie przetwornika ADC0 w kanale 12 i odblokowanie przerwania
	UART_Init();		// Inicjalizacja portu szeregowego UART0 baud rate = 28 800
	menu_powitalne( stan_L, stan_H,  pit_period);
	
	
	while(1)
	{
		if( ( stan_adc == 1) && etap_0)			// Niezalezny ciagly odczyt wartosci z ADC dla etapu_0
		{

			wynik_last = wynik*volt_coeff;		// Dostosowanie wyniku do zakresu napięciowego, zapisz w innej zmiennej niezaleznej od tej
																				// w przerwaniu ADC			
			global_counter = 0;
			wynik = 0;
			stan_adc = 0;											// Aktywuj operacje wewnątrz przerwania od ADC 
		}
		
		
		/* Komunikacja przez UART -- menu kontekstowe */
		
		if( znacznik_transmisji && etap_0)		// Dana przeslana przez UART, nastepuje odczyt w petli glownej
		{																			// znacznik_transmisji == 1 informuje o zakonczeniu odczytu danych wyslanych z terminala przez UART
			
			if(display_flag == 0)
			{
				display_flag = 1;
			}
			
			if(znacznik_transmisji == -1) 
			{
				UART_Println("Wprowadzono zbyt dlugi ciag znakow");
			}
			else
			{
				/* Obsluga komend w terminalu UART */
				
				if(strcmp_wildcard (string_pattern_running_config,rx_buf)==1)						 // running config 
				{
					running_config(stan_L, stan_H, wynik_last, pit_period);
				}
				
				else if(strcmp_wildcard (string_pattern_commands,rx_buf)==1)						  // commands 
				{
					commands();
				}
				
				
				else if(strcmp_wildcard (string_pattern_vlt_set_l,rx_buf)==1)						 // vlt set low **** [mV] 
				{
					stan_L = vlt_set_low(rx_buf, stan_L);
				}
				
				else if(strcmp_wildcard (string_pattern_vlt_set_h,rx_buf)==1)						 // vlt set high **** [mV] 
				{
					stan_H = vlt_set_high(rx_buf, stan_H);
				}

				else if(strcmp_wildcard (string_pattern_vlt_show,rx_buf)==1)						// vlt show [mV] 
				{
					sprintf(rx_buf, "Aktualne napiecie tla %.2f [mV]", wynik_last);
					UART_Println(rx_buf);					
				}
				
				else if(strcmp_wildcard(string_pattern_pit_set_period,rx_buf)==1) 			// pit set period *** [ms]
				{	
					sprintf(sprintf_buf, "Poprzedni okres przerwan licznika PIT %i [ms]", pit_period);
					UART_Println(sprintf_buf);
					
					pit_period = char2int_wildcard(string_pattern_pit_set_period, rx_buf);
					PIT_set_tsv(pit_period);
					
					sprintf(sprintf_buf, "Okres przerwan licznika PIT ustawiony na %i [ms]", pit_period);
					UART_Println(sprintf_buf);
					
				}
				else if(strcmp_wildcard(string_pattern_pit_show,rx_buf)==1) 			// pit show [ms]
				{
					sprintf(sprintf_buf, "Aktualny okres przerwan licznika PIT %i [ms]", pit_period);
					UART_Println(sprintf_buf);					
				}
				
				else if(strcmp_wildcard(string_pattern_morse_debug,rx_buf)==1)		 // morse debug
				{
					if(morse_run_flag == 1)
					{
						int i;
						int i_znak = 0;			// osobny iterator po tablicy zdekodowanych znakow
						int znak_flag = 1;  // czy z sekwencja danych surowych ma byc tez wypisany zdekodowany znak -- na poczatku sekwencji
						sprintf(sprintf_buf,"Dane surowe i czesciowo rozkodowane w ostatniej transmisji, stany = %i", stan_zapisu);
						UART_Println(sprintf_buf);
						UART_Println(" ");
						for( i=0; i<stan_zapisu ; i++)
							{	
								if(znak_flag == 1 && (tablica_stanow[i] != -7))	// wyswietlany kolejny znak
								{
									sprintf(sprintf_buf, "Pozycja [%i] :: ilosc zliczen ** %i ** -- kod ** %i ** -- znak ** %c ** ",i, tablica_sygnalow[i+1], tablica_stanow[i], tablica_znakow[i_znak]);
									UART_Println(sprintf_buf);
									i_znak ++;
									znak_flag = 0;
								}			

								else	// bez znaku
								{
									sprintf(sprintf_buf, "Pozycja [%i] :: ilosc zliczen ** %i ** -- kod ** %i ** ",i, tablica_sygnalow[i+1], tablica_stanow[i]);
									UART_Println(sprintf_buf);
								}
								
								if( tablica_stanow[i] == -3 )	// kolejny znak
								{
									znak_flag = 1;
								}
								
								if( tablica_stanow[i] == -7 )	// kolejny znak
								{

									sprintf(sprintf_buf, "Pozycja [%i] :: ilosc zliczen ** %i ** -- kod ** %i ** -- znak ** %c ** ",i, tablica_sygnalow[i+1], tablica_stanow[i], tablica_znakow[i_znak]);
									UART_Println(sprintf_buf);
									i_znak ++;
									znak_flag = 1;
								
								}
							}
					}
					else
					{
						UART_Println("Brak danych do wyswietlenia");
					}
				}
				
				else if(strcmp_wildcard(string_pattern_morse_run,rx_buf)==1)		 // morse run
				{
					UART_Println("Zaczynam odczyt kodu morse");
					
					etap_0 = 0;
					etap_1 = 1;
					
					if(morse_run_flag == 1)
					{
					UART_Println("Kasuje tablice odczytow");
					memset(tablica_sygnalow, 0, stan_zapisu*sizeof(int));
					memset(tablica_stanow, 0, stan_zapisu*sizeof(int)); 
					memset(tablica_znakow, 0, ilosc_znakow*sizeof(int)); 
					
					stan_zapisu = 0;
					ilosc_znakow = 0;
					}
					
				}
				

				
					else
					{
						UART_Println("Zla komenda");
					}
				}
			

			znacznik_transmisji=0;	// Koniec odczytu
		}
		
		/* Etap 1 */
		
		if( (stan_adc == 1) && etap_1)
		{
			if(display_flag)
			{
				UART_Println("Etap 1 -- czytanie kodu morse'a ");
				display_flag = 0;
			}
			
			if(morse_run_flag == 0)
			{
				morse_run_flag = 1;				// zaznacz, ze morse run zostal wywolany przynajmniej raz
			}
			
			wynik = wynik*volt_coeff;		// Dostosowanie wyniku do zakresu napięciowego
			
			/* Domyslnie na czytniku panuje stan LOW -- pierwszy zapis to LOW */
			
			stan_zapisu = przechwytywanie_sygnalow(stan_L, stan_H, wynik, tablica_sygnalow, tsig_rozmiar);
			
			if(stan_zapisu == 0)
			{

				global_counter = 0;
				wynik = 0;
				stan_adc = 0;

			}
			
			else if (stan_zapisu > 0)
			{

			  sprintf(rx_buf, "Zapis zakonczony. Zapisano %i stanow",stan_zapisu);
				UART_Println(rx_buf);
				
				etap_1 = 0;
				etap_2 = 1;
				

			}
			
			else if (stan_zapisu <0 )	// blad przepelnienia
			{
			  sprintf(rx_buf, "Przekroczono rozmiar bufora zapisu, zapisano %i wartosci %c",stan_zapisu,0xd);
				UART_Println(rx_buf);

				memset(tablica_sygnalow, 0, stan_zapisu*sizeof(int));
				stan_zapisu = 0;
				etap_1 = 0;
				etap_0 = 1;
				
			}


			
		} // if(wynik_ok) petla
		
			/* ************** */
			/* Dekodowanie -- pierwszy stopien */
		
   if(etap_2)
		{
			
		  UART_Print("Dekodowanie wartosci - pierwszy stopien");
			int pd_stan = 0;
			pd_stan = dekoder_1_st(tablica_sygnalow,tablica_stanow,stan_zapisu);
		
			if(pd_stan == 1)	// dekodowanie pierwszego stopnia zakonczone
			{

					etap_2 = 0;
					etap_3 = 1;
					UART_Println(" :: zakonczono pomyslnie");
	
			}
				

			
		} // end dekodowanie_pierwszy_stopien
		
		/* ************** */
		/* ************** */
		/* Dekodowanie -- drugi stopien */
		
		if(etap_3)
		{	
			
			UART_Println("Dekodowanie drugi stopien :: odczytana wartosc");
			ilosc_znakow = dekoder_2_st(tablica_znakow, tablica_stanow, stan_zapisu, baza_znakow);
			 for(i=0; i<ilosc_znakow; i++)
					{
						sprintf(rx_buf, "%c",tablica_znakow[i]);
						UART_Print(rx_buf); 
					} 
					

			etap_3 = 0;					
			etap_0 = 1;
			
		}
		
	} // end while(1) petla glowna
} // end main()

