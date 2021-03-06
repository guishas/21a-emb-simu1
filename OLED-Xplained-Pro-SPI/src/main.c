#include <asf.h>

#include "oled/gfx_mono_ug_2832hsweg04.h"
#include "oled/gfx_mono_text.h"
#include "oled/sysfont.h"

#define LED_1_PIO PIOA
#define LED_1_PIO_ID ID_PIOA
#define LED_1_IDX 0
#define LED_1_IDX_MASK (1 << LED_1_IDX)

#define LED_2_PIO PIOC
#define LED_2_PIO_ID ID_PIOC
#define LED_2_IDX 30
#define LED_2_IDX_MASK (1 << LED_2_IDX)

#define LED_3_PIO PIOB
#define LED_3_PIO_ID ID_PIOB
#define LED_3_IDX 2
#define LED_3_IDX_MASK (1 << LED_3_IDX)

#define BUT_1_PIO PIOD
#define BUT_1_PIO_ID ID_PIOD
#define BUT_1_IDX 28
#define BUT_1_IDX_MASK (1u << BUT_1_IDX)

#define BUT_2_PIO PIOC
#define BUT_2_PIO_ID ID_PIOC
#define BUT_2_IDX 31
#define BUT_2_IDX_MASK (1u << BUT_2_IDX)

#define BUT_3_PIO PIOA
#define BUT_3_PIO_ID ID_PIOA
#define BUT_3_IDX 19
#define BUT_3_IDX_MASK (1u << BUT_3_IDX)

volatile char but1_flag = 0;
volatile char but2_flag = 0;
volatile char but3_flag = 0;
volatile char estado = 0;  // 0 - Fechado / 1 - Aberto / 2 - Travado

void but1_callback(void) {
	but1_flag = 1;
}

void but2_callback(void) {
	but2_flag = 1;
}

void but3_callback(void) {
	but3_flag = 1;
}


void io_init(void) {
	
  WDT->WDT_MR = WDT_MR_WDDIS;
  	
  pmc_enable_periph_clk(LED_1_PIO_ID);
  pmc_enable_periph_clk(LED_2_PIO_ID);
  pmc_enable_periph_clk(LED_3_PIO_ID);
  pmc_enable_periph_clk(BUT_1_PIO_ID);
  pmc_enable_periph_clk(BUT_2_PIO_ID);
  pmc_enable_periph_clk(BUT_3_PIO_ID);

  pio_configure(LED_1_PIO, PIO_OUTPUT_0, LED_1_IDX_MASK, PIO_DEFAULT);
  pio_configure(LED_2_PIO, PIO_OUTPUT_0, LED_2_IDX_MASK, PIO_DEFAULT);
  pio_configure(LED_3_PIO, PIO_OUTPUT_0, LED_3_IDX_MASK, PIO_DEFAULT);

  pio_configure(BUT_1_PIO, PIO_INPUT, BUT_1_IDX_MASK, PIO_PULLUP| PIO_DEBOUNCE);
  pio_configure(BUT_2_PIO, PIO_INPUT, BUT_2_IDX_MASK, PIO_PULLUP| PIO_DEBOUNCE);
  pio_configure(BUT_3_PIO, PIO_INPUT, BUT_3_IDX_MASK, PIO_PULLUP| PIO_DEBOUNCE);

  pio_handler_set(BUT_1_PIO, BUT_1_PIO_ID, BUT_1_IDX_MASK, PIO_IT_FALL_EDGE,
  but1_callback);
  pio_handler_set(BUT_2_PIO, BUT_2_PIO_ID, BUT_2_IDX_MASK, PIO_IT_FALL_EDGE,
  but2_callback);
  pio_handler_set(BUT_3_PIO, BUT_3_PIO_ID, BUT_3_IDX_MASK, PIO_IT_FALL_EDGE,
  but3_callback);

  pio_enable_interrupt(BUT_1_PIO, BUT_1_IDX_MASK);
  pio_enable_interrupt(BUT_2_PIO, BUT_2_IDX_MASK);
  pio_enable_interrupt(BUT_3_PIO, BUT_3_IDX_MASK);

  pio_get_interrupt_status(BUT_1_PIO);
  pio_get_interrupt_status(BUT_2_PIO);
  pio_get_interrupt_status(BUT_3_PIO);

  NVIC_EnableIRQ(BUT_1_PIO_ID);
  NVIC_SetPriority(BUT_1_PIO_ID, 4);

  NVIC_EnableIRQ(BUT_2_PIO_ID);
  NVIC_SetPriority(BUT_2_PIO_ID, 4);

  NVIC_EnableIRQ(BUT_3_PIO_ID);
  NVIC_SetPriority(BUT_3_PIO_ID, 4);
}

void RTT_Handler(void) {
	uint32_t ul_status;

	/* Get RTT status - ACK */
	ul_status = rtt_get_status(RTT);

	/* IRQ due to Alarm */
	if ((ul_status & RTT_SR_ALMS) == RTT_SR_ALMS) {
		estado = 2;
	}
	
	/* IRQ due to Time has changed */
	if ((ul_status & RTT_SR_RTTINC) == RTT_SR_RTTINC) {
		   // BLINK Led
	}

}

void RTT_init(float freqPrescale, uint32_t IrqNPulses, uint32_t rttIRQSource) {

	uint16_t pllPreScale = (int) (((float) 32768) / freqPrescale);
	
	rtt_sel_source(RTT, false);
	rtt_init(RTT, pllPreScale);
	
	if (rttIRQSource & RTT_MR_ALMIEN) {
		uint32_t ul_previous_time;
		ul_previous_time = rtt_read_timer_value(RTT);
		while (ul_previous_time == rtt_read_timer_value(RTT));
		rtt_write_alarm_time(RTT, IrqNPulses+ul_previous_time);
	}

	/* config NVIC */
	NVIC_DisableIRQ(RTT_IRQn);
	NVIC_ClearPendingIRQ(RTT_IRQn);
	NVIC_SetPriority(RTT_IRQn, 4);
	NVIC_EnableIRQ(RTT_IRQn);

	/* Enable RTT interrupt */
	if (rttIRQSource & (RTT_MR_RTTINCIEN | RTT_MR_ALMIEN))
		rtt_enable_interrupt(RTT, rttIRQSource);
	else
		rtt_disable_interrupt(RTT, RTT_MR_RTTINCIEN | RTT_MR_ALMIEN);
	
}

static uint32_t get_time_rtt() {
	return rtt_read_timer_value(RTT);
}

void writePasswordToOLED(int tentativa[4], char num1[128], char num2[128], char num3[128], char num4[128]) {
	sprintf(num1, "%d", tentativa[0]);
	sprintf(num2, "%d", tentativa[1]);
	sprintf(num3, "%d", tentativa[2]);
	sprintf(num4, "%d", tentativa[3]);
	
	if (num1[0] == '9') {
		num1[0] = ' ';
	}
	
	if (num2[0] == '9') {
		num2[0] = ' ';
	}
	
	if (num3[0] == '9') {
		num3[0] = ' ';
	}
	
	if (num4[0] == '9') {
		num4[0] = ' ';
	}
	
	gfx_mono_draw_string(num1, 0, 14, &sysfont);
	gfx_mono_draw_string(num2, 20, 14, &sysfont);
	gfx_mono_draw_string(num3, 40, 14, &sysfont);
	gfx_mono_draw_string(num4, 60, 14, &sysfont);
}

void eraseOLEDCima() {
	gfx_mono_draw_string("             ", 0, 0, &sysfont);
}

void eraseOLEDBaixo() {
	gfx_mono_draw_string("             ", 0, 14, &sysfont);
}

void all_leds_off() {
	pio_set(LED_1_PIO, LED_1_IDX_MASK);
	pio_set(LED_2_PIO, LED_2_IDX_MASK);
	pio_set(LED_3_PIO, LED_3_IDX_MASK);
}

void all_leds_on() {
	pio_clear(LED_1_PIO, LED_1_IDX_MASK);
	pio_clear(LED_2_PIO, LED_2_IDX_MASK);
	pio_clear(LED_3_PIO, LED_3_IDX_MASK);
}

int main(void) {
	board_init();
	sysclk_init();
	delay_init();
	io_init();
	gfx_mono_ssd1306_init();
  
	int senha[4] = {1, 1, 2, 3};
	int tentativa[4] = {9, 9, 9, 9};
	int n = 0;
	
	char num1[128], num2[128], num3[128], num4[128];
	
	gfx_mono_draw_string("Cofre Fechado", 0, 0, &sysfont);
  
	while (1) {
		
		if (estado == 2) {
			eraseOLEDBaixo();
			eraseOLEDCima();
			gfx_mono_draw_string("Cofre Fechado", 0, 0, &sysfont);
			estado = 0;
		}
		
		if (estado != 3) {
			if (n == 4) {
				int errou = 0;
				for (int i = 0; i < n; i++) {
					if (tentativa[i] != senha[i]) {
						errou = 1;
					}
				}
				
				if (errou) {
					eraseOLEDCima();
					eraseOLEDBaixo();
					gfx_mono_draw_string("Senha errada", 0, 0, &sysfont);
					gfx_mono_draw_string("Bloqueado", 0, 14, &sysfont);
					estado = 3;
					RTT_init(4, 16, RTT_MR_ALMIEN);
				} else {
					estado = 1;
					eraseOLEDCima();
					eraseOLEDBaixo();
					all_leds_off();
					gfx_mono_draw_string("Cofre Aberto", 0, 0, &sysfont);
				}
				
				for (int i = 0; i < n; i++) {
					tentativa[i] = 9;
				}
				
				n = 0;
			}
			
			if (but1_flag) {
				if (estado == 0) {
					tentativa[n] = 1;
					n++;
					eraseOLEDBaixo();
					writePasswordToOLED(tentativa, num1, num2, num3, num4);
					} else {
					estado = 0;
					all_leds_on();
					eraseOLEDBaixo();
					eraseOLEDCima();
					gfx_mono_draw_string("Cofre Fechado", 0, 0, &sysfont);
				}
				but1_flag = 0;
			}
			
			if (but2_flag) {
				if (estado == 0) {
					tentativa[n] = 2;
					n++;
					eraseOLEDBaixo();
					writePasswordToOLED(tentativa, num1, num2, num3, num4);
				}
				but2_flag = 0;
			}
			
			if (but3_flag) {
				if (estado == 0) {
					tentativa[n] = 3;
					n++;
					eraseOLEDBaixo();
					writePasswordToOLED(tentativa, num1, num2, num3, num4);
				}
				but3_flag = 0;
			}
		}
		
	}
}
