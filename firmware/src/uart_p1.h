#ifndef UART_P1_HEADER_H
#define UART_P1_HEADER_H

#include <zephyr/kernel.h>

int uart_p1_init(struct k_pipe *output, struct k_pipe *tx); 

#endif /* UART_P1_HEADER_H */