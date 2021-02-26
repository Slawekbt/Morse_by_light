#ifndef pit_h
#define pit_h
#define BUS_CLOCK SystemCoreClock/2

void PIT_Init(int period);
void PIT_set_tsv(int period); // period in [ms]

#endif
