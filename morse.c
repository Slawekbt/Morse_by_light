#include "morse.h"

// zdefiniowac biblioteke z LED ktora bedzie pokazywac ze wykryto transmismisje, i ktora bedzie transmisje przerywac
// wyswietlanie momentu dekodowania itp

// funkcje z tablicami musza przechowywac rozmiary tablic


int przechwytywanie_sygnalow(float napiecie_L, float napiecie_H, float sygnal, int* tablica_sygnalow, int tsig_rozmiar)
{
	static int ile_stanow_H = 0;
	static int ile_stanow_L = 0;
	static int tsig_iterator = 0;
	int liczba_obsadzonych_miejsc = 0;
	
	if(sygnal > napiecie_H)
		
			{
				ile_stanow_H +=1;
				
				if(ile_stanow_H && ile_stanow_L) // zapis zliczen LOW tylko przy zmianie stanu na HIGH -- quazi zbocze narastajace
				{
					tablica_sygnalow[tsig_iterator] = ile_stanow_L;
					tsig_iterator +=1;

				}
				
				ile_stanow_L = 0;
				
			}
			
			else if(sygnal < napiecie_L)
			{
				ile_stanow_L +=1;
				
			  if(ile_stanow_H && ile_stanow_L) // zapis zliczen HIGH tylko przy zmianie stanu na LOW -- quazi zbocze opadajace
				{
					tablica_sygnalow[tsig_iterator] = ile_stanow_H;
					tsig_iterator +=1;

				}
				
				ile_stanow_H = 0;
			}
			/* Wykrycie przepelnienia*/
		  if( tsig_iterator > tsig_rozmiar)
			{
				return -1; // przepelnienie tablicy (!) trzeba powiekszac tablice przy pomocy remalloc !!!
			}
			/* Wykrycie konca transmisji */
			if( (ile_stanow_L > 10 * tablica_sygnalow[tsig_iterator - 2]) && (tsig_iterator > 1) ) // tsig_iterator - 2 to bedzie zawsze kropka
			{
				/* zerowanie static int'ow */
				liczba_obsadzonych_miejsc = tsig_iterator -1;
				
				tsig_iterator = 0;
				ile_stanow_L = 0;
				ile_stanow_H = 0;
				
				return liczba_obsadzonych_miejsc  ; // ilosc obsadzonych miejsc
			}
			
			else
			{
				return 0; // trasmisja (jak i zapis) trwaja nadal
			}
			
	
}

int dekoder_1_st(int* tablica_sygnalow, int *tablica_stanow, int ts_rozmiar) //
{
	/*
	  stany niskie (L)
		
	1 kropka : odstep pomiedzy elementami znaku (stanami wysokimi)	{-1}
	3 kropki : odstep pomiedzy znakami (literami)										{-3}
	7 kropek : odstep pomiedzy slowami															{-7}
	
	 stany wysokie (H)
	
	kropka :																												{1}
	kreska :																												{3}
	
	stany nieokreslone (?)
	
	???    :																												{0}
	
	
	tablica 'pierwszy_dekod' bedzie o jeden element krotsza w porownaniu do tablicy 'tablica_sygnalow'
	ze wzgledu na smieciowe informacje w 'tablica_sygnalow[0]'
	
	*/
	
	volatile int i;
	int kreska;
	int kropka;
	int spacja;
	
	kropka = tablica_sygnalow[2]; // zawsze kropka {-1} -- (!!!) chyba ze zdanie zaczyna sie od "e ***", "t ***", wtedy bedzie to kreska !
	kreska = 3 * kropka;
	spacja = 7 * kropka;
	
	/* Dopuszczalne zakresy zmiennosci */
	
	int kreska_L = (int)(0.9*kreska);
	int kreska_H = (int)(1.1*kreska);
	
	int kropka_L = (int)(0.9*kropka);
	int kropka_H = (int)(1.1*kropka);
	
	int spacja_L = (int)(0.9*spacja);
	int spacja_H = (int)(1.1*spacja);
	
	for( i = 0 ; i < ts_rozmiar ; i++ ) // tablica_sygnalow[0] przechowuje smieci, stad tablica_sygnalow[i+1]
	{
		if( (tablica_sygnalow[i+1] > kropka_L) && (tablica_sygnalow[i+1] < kropka_H) ) 
		{
			tablica_stanow[i] = 1;
		}
		
		else if( (tablica_sygnalow[i+1] > kreska_L) && (tablica_sygnalow[i+1] < kreska_H) )
		{
			tablica_stanow[i] = 3;
		}

		else if( (tablica_sygnalow[i+1] > spacja_L) && (tablica_sygnalow[i+1] < spacja_H) )
		{
			tablica_stanow[i] = 7;
		}
		
		else
		{
			tablica_stanow[i] = 0;			
		}
		
	}
	
	/* co drugi element tablicy to sygnal o stanie niskim (L) -- obrocenie tych elementow */
	
	for( i = 1; i< ts_rozmiar ; i+=2)
	{
		tablica_stanow[i] = - tablica_stanow[i];
	}
	
	return 1; // informacja o zakonczeniu dekodowania
	
}


int dekoder_2_st(char *tablica_znakow,int *tablica_stanow, int ts_rozmiar, char* baza_znakow)
{
	volatile int i;
	int symbol_stanu = 0;             // wartosc reprezentujaca kreske {3} , kropke {1}  ... opisane na poczatku funkcji int dekoder_1_st() 
	int identyfikator_znaku = 0;
	int mnoznik_poziomu = 1;   // mnozy wartosc kreski {3} lub kropki {1} przez odpowiednia wartosc -- drzewko stanow -- dokumentacj
	int tz_iterator = 0;       // iterator po tablicy znakow
	
	for( i=0 ; i<ts_rozmiar ; i++)
		{

			symbol_stanu = tablica_stanow[i];
			switch(symbol_stanu)
			{
				case 1:
					identyfikator_znaku += (1 * mnoznik_poziomu);
				
					if( i == (ts_rozmiar-1) ) // zapis dla ostatniego elementu tablicy, niezakonczonego przy pomocy {-3} lub {-7}
					{
						tablica_znakow[tz_iterator] = baza_znakow[identyfikator_znaku];
					}
					break;
				
				case 3:
					identyfikator_znaku += (2 * mnoznik_poziomu);
				
					if( i == (ts_rozmiar-1) ) // zapis dla ostatniego elementu tablicy, niezakonczonego przy pomocy {-3} lub {-7}
					{
						tablica_znakow[tz_iterator] = baza_znakow[identyfikator_znaku];
					}
					
					break;
				
				case -1:
					 mnoznik_poziomu *= 2;
				   break;
				
				case -3:
					 tablica_znakow[tz_iterator] = baza_znakow[identyfikator_znaku];
					 mnoznik_poziomu = 1;
					 identyfikator_znaku = 0;
					 tz_iterator ++;
					 break;
				
				case -7:
					 tablica_znakow[tz_iterator] = baza_znakow[identyfikator_znaku]; // zapis poprzedniego znaku
					 tz_iterator ++;
					 tablica_znakow[tz_iterator] = 0x20; // zapis spacji
					 tz_iterator ++;
					 mnoznik_poziomu = 1;
					 identyfikator_znaku = 0;
						break;
				
			} // switch()
					
				
					
		} // for()
		
		return tz_iterator + 1; // ilosc zapisanych znakow
		
} //dekoder_2_st()

