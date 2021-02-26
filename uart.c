/******************************************************************************
 * This file is a part of the Sysytem Microprocessor Tutorial (C).            *
 ******************************************************************************/

/**
 * @file uart.c
 * @author Koryciak, edited by Tenerowicz
 * @date Sep 2020, edit date Jan 2021
 * @brief File containing definitions.
 * @ver 0.1
 */

#include "uart.h"

/******************************************************************************\
* Private prototypes
\******************************************************************************/
void uart_send(uint8_t data);

void UART_Init (void) {
	
	SIM->SCGC4 |= SIM_SCGC4_UART0_MASK;							//UART0 dolaczony do zegara
	SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;							//Port B dolaczony do zegara
	SIM->SOPT2 |= SIM_SOPT2_UART0SRC(1);		        //Zegar MCGFLLCLK=41943040Hz (CLOCK_SETUP=0)
	PORTB->PCR[1] = PORT_PCR_MUX(2);								//PTB1=TX_D
	PORTB->PCR[2] = PORT_PCR_MUX(2);								//PTB2=RX_D
	
	UART0->C2 &= ~(UART0_C2_TE_MASK | UART0_C2_RE_MASK );		//Blokada nadajnika i o dbiornika
	UART0->BDH = 0;
	UART0->BDL =91;		//Dla CLOCK_SETUP=0 BR=28800	BDL=91	:	CLOCK_SETUP=1	BR=230400		BDL=13
	UART0->C4 &= ~UART0_C4_OSR_MASK;
	UART0->C4 |= UART0_C4_OSR(15);	//Dla CLOCK_SETUP=0 BR=28800	OSR=15	:	CLOCK_SETUP=1	BR=230400		OSR=15
	UART0->C5 |= UART0_C5_BOTHEDGE_MASK;	//Próbkowanie odbiornika na obydwu zboczach zegara
	UART0->C2 |= UART0_C2_RIE_MASK;		// Wlacz przerwania od odbiornika
	UART0->C2 |= (UART0_C2_TE_MASK | UART0_C2_RE_MASK);		//Wlacz nadajnik i odbiornik
	NVIC_EnableIRQ(UART0_IRQn);
	NVIC_ClearPendingIRQ(UART0_IRQn);
	
}

void UART_Print(const char *str) {
	
	uint8_t str_len = 0;

  while (str[str_len] != '\0') {              /* until end of string */
    uart_send(str[str_len]);
    ++str_len;
  }
}

void UART_Println(const char *str) {
	
	UART_Print(str);
	UART_Print("\n\r");
}

/**
 * @brief UART send data
 */
void uart_send(uint8_t data) {
	while(!(UART0->S1 & UART0_S1_TDRE_MASK));
	UART0->D = data;
}

void uart_tx_ready(void)
{
	while(!(UART0->S1 & UART0_S1_TDRE_MASK));
}

int UART_Read(int znacznik_transmisji, char* rx_buf, int rx_buf_len)
{	
	char read_val;							 // Tymczasowa zmienna na odebranie ciagu bitow z bufora UART
	uint8_t too_long = 0; // Indykator zbyt dlugiego ciagu znakow
	static int rx_buf_pos = 0;   // Iterator po tablicy bufora
	
	if(UART0->S1 & UART0_S1_RDRF_MASK)
	{
		read_val=UART0->D;				 // Odczyt z bufora odbiornika, skasowanie flagi RDRF
		
		if(!znacznik_transmisji)	 // Jesli rowny 0 znaczy ze w petli glownej nastapil odczyt danych
		{
			
			if(read_val!=0xa) 			// Jesli odczytana wartosc jest rozna od znaku Enter
			{
				if(!too_long)					// Jesli ciag znakow nie jest zbyt dlugi
				{
					rx_buf[rx_buf_pos] = read_val;
					rx_buf_pos ++;
					
					if(rx_buf_pos == rx_buf_len) // Jesli nastapi przepelnienie zadeklarowanego bufora odbiorczego
					{
						too_long = 1;		// Przepelnienie bufora odbiorczego
						rx_buf_pos = 0;					// Zeruj pozycje iteratora po buforze odbiorczym
						return -1;
					}
				}
			}
			else
			{
				if(!too_long)
				{
					rx_buf[rx_buf_pos] = 0; // Zakoncz ciag sybolem 0 EOL
					rx_buf_pos = 0;					// Zeruj pozycje iteratora po buforze odbiorczym
				}
				
				znacznik_transmisji = 1;	// Zakonczono odczyt z bufora, nastepuje odczyt w petli glownej programu
				
			}
				

			
		} // if(!znacznik_trasmisji)
		
		
	} // if(UART0->S1 & UART0_S1_RDRF_MASK)
	
	NVIC_EnableIRQ(UART0_IRQn);
	return znacznik_transmisji; /* 1 -- pomyslnie zakonczono odczyt z bufora, program moze zaczac odczytywac dane w petli glownej
																 0 -- dane w petli glownej while() nie zostaly jeszcze obsluzone
																-1 -- nastapilo przepelnienie
																	*/
	
}
	
