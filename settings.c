#include "uart.h"
#include "settings.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>



void menu_powitalne(float stan_L, float stan_H, int pit_okres)
{
	char menu_buf[64];
	
	UART_Println("Menu kontekstowe programu MORSE");
	UART_Println("Nie obsluguje ciagu znakow ktory zaczyna sie od 't' badz 'e' -- wada dekodowania");
	UART_Println("Wymaga podpiecia czujnika swiatla do portu PTA0");
	UART_Println("Wpisz commands aby uzyskac informacje o dostepnych komendach");
	
	sprintf(menu_buf, "Napiecie stanu niskiego: %.2f", stan_L);
	UART_Println(menu_buf);
	
	sprintf(menu_buf, "Napiecie stanu wysokiego: %.2f", stan_H);
	UART_Println(menu_buf);
	
	sprintf(menu_buf, "Okres pomiedzy kolejnymi odczytami napiecia [ms]: %i", pit_okres);
	UART_Println(menu_buf);

}

void commands()
{
	UART_Println(" *** Dostepne komendy *** ");
	UART_Println(" ");
	UART_Println(" running config :: pokazuje aktualne parametry  dzialajacego programu ");
	UART_Println(" ");
	UART_Println(" morse run :: rozpoczyna zapis stanow L i H do tablicy stanow, dekoduje i wyswietla wynik ");
	UART_Println(" ");
	UART_Println(" morse debug :: wyswietla tablice zapisanych i zdekodowanych danych z ostatniej transmisji ");
	UART_Println(" ");
	UART_Println(" vlt show :: pokazuje aktualne napiecie panujace na czujniku swiatla [mV]  ");
	UART_Println(" ");
	UART_Println(" vlt set low **** :: ustawia wartosc stanu L [mv] ");
	UART_Println(" napiecia odczytane z czujnika swiatla ponizej wartosci L beda traktowane jako stan niski ");
	UART_Println(" w miejsce **** nalezy wpisac liczbe 0 - 2900 ");
	UART_Println(" liczba ta musi byc wieksza niz poziom tla (gdy czujnik nieoswietony) , mniejsza niz stan wysoki (H)");
	UART_Println(" ");
	UART_Println(" vlt set high **** :: ustawia wartosc stanu H [mv] ");
	UART_Println(" napiecia odczytane z czujnika swiatla powyzej wartosci H beda traktowane jako stan wysoki ");
	UART_Println(" w miejsce **** nalezy wpisac liczbe 0 - 2900 ");
	UART_Println(" liczba ta musi byc mniejsza niz wartosc napiecia na czujniku, gdy jest oswietlony");
	UART_Println(" oraz wieksza niz wartosci stanu L ");
	UART_Println(" ");
	UART_Println(" pit period set *** :: ustawia okres przerwan od licznika PIT [ms] ");
	UART_Println(" co ten okres bedzie odczytywane napiecie z czujnika swiatla ");
	UART_Println(" i zostanie okreslony (zliczony) odpowiedni stan (L lub H) ");
	UART_Println(" ");
	UART_Println(" pit show :: pokazuje aktualny okres przerwan licznika PIT [ms] ");
	UART_Println(" ");
	UART_Println(" -------------- ");
	UART_Println(" ");
	
}

void running_config(float stan_L, float stan_H, float stan_tlo, int pit_period)
{
	char temp_buf[64];
	UART_Println(" *** Aktualne parametry configuracji *** ");

	
	sprintf(temp_buf, "Napiecie stanu niskiego (L) :: %.2f", stan_L);
	UART_Println(temp_buf);
	
	sprintf(temp_buf, "Napiecie stanu wysokiego (H) :: %.2f", stan_H);
	UART_Println(temp_buf);
	
	sprintf(temp_buf, "Napiecie tla (current state) :: %.2f", stan_tlo);
	UART_Println(temp_buf);
	
	sprintf(temp_buf, "Okres licznika PIT :: %i", pit_period);
	UART_Println(temp_buf);		
	UART_Println(" -------------- ");
}

float vlt_set_high(char* rx_buf, float stan_H)
{
	char temp_buf[64];
	const char string_pattern_vlt_set_h[]= {'v','l','t',' ','s','e','t',0x20,'h','i','g','h',' ','*','*','*','*',0x0};
	sprintf(temp_buf, "Poprzednia wartosc stanu H: %.2f [mV]",stan_H);
	UART_Println(temp_buf);
	int value_H = char2int_wildcard(string_pattern_vlt_set_h, rx_buf);
	float value_H_float = (float)value_H;
	stan_H = value_H_float;
	sprintf(temp_buf, "Nowa wartosc stanu H: %.2f [mV]",stan_H);
	UART_Println(temp_buf);
	
	return stan_H;
}

float vlt_set_low(char* rx_buf, float stan_L)
{
	char temp_buf[64];
	const char string_pattern_vlt_set_l[]= {'v','l','t',' ','s','e','t',0x20,'l','o','w',' ','*','*','*','*',0x0};
	sprintf(temp_buf, "Poprzednia wartosc stanu L: %.2f [mV]",stan_L);
	UART_Println(temp_buf);
	int value_L = char2int_wildcard(string_pattern_vlt_set_l, rx_buf);
	float value_L_float = (float)value_L;
	stan_L = value_L_float;
	sprintf(temp_buf, "Nowa wartosc stanu L: %.2f [mV]",stan_L);
	UART_Println(temp_buf);
	
	return stan_L;
}


int strcmp_wildcard(const char *string_pattern, const char* string_input)
{
	int i = 0;
	
	for( i=0; string_input[i] != 0; i++)
		{
			
			if( string_pattern[i] == string_input[i])
				continue;
			else if(string_pattern[i] == '*')				// dozwolony wildcard w stringu wzorcowym
				continue;
			else
				return -1;
			
		}
	return 1;
}

int char2int_wildcard(const char *string_pattern, const char* string_input) // dla danych wejsiowych typu "text***"
{
	int i = 0;
	int wildcard_size = 0; 										// liczba znakow wildcard pod ktore mialy zostac podstawione liczby
	int wildcard_seq_start_pos = -1;					// pozycja poczatku ciagu '*' wildcard w stringu wejsciowym, '-1' dziala jak flaga
	
	int decimal_multiplier = 1; 							// mnoznik systemu dziesietnego
	int converted_value = 0;									// wartosc liczbowa uzyskana z ciagu char
	
	for( i = 0; string_input[i] != 0 ; i++)
		{
			if(string_pattern[i] == '*')					// szukaj znakow '*' wildcard w ciagu wzorcowym
			{
				wildcard_size ++;
				
				if(wildcard_seq_start_pos == -1)
				{
					wildcard_seq_start_pos = i;				// zapisz pozycje na ktorej zaczyna sie wildcard
				}
				
			}
			else
			{
				continue;
			}
				
		}
		
		for( i = wildcard_seq_start_pos + wildcard_size - 1; i > wildcard_seq_start_pos -1  ; i--)	// zacznij od ostatnej pozycji wildcard, zakoncz na pierwszej
				{
					
					if(string_input[i] == 0)
						continue;
					else
					{
						converted_value += ( (int)(string_input[i]) - 48) * decimal_multiplier; // (int)(string_input[i]) zamieni cyfre char na jej odpowiednik
																																									 // w kodzie ASCII, "offset" dla cyfr wynosi 48, 0x30
						decimal_multiplier *=10;																							 // kolejna cyfra w zapisie dziesietnym -- jej waga
					}
				}
		
		
	return converted_value;
}


int clear_rx_buf(char* string_input, int string_len)
{
	int i = 0;
	
	for ( i = 0; i < string_len ; i++)
		{
			string_input[i] = 0;
		}
		
	return 0;
}
