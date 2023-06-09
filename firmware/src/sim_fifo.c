#include <sys/types.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/crc.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

#if CONFIG_OPENP1_SIM_DATA_ENABLED

#define STACKSIZE 1024
#define PRIORITY 7

extern struct k_pipe rx_pipe; 
extern struct k_pipe tx_pipe; 

#if CONFIG_OPENP1_UART_DATA_SOURCE
#define SIM_OUTPUT &tx_pipe
#else
#define SIM_OUTPUT &rx_pipe
#endif

LOG_MODULE_REGISTER(sim_fifo, LOG_LEVEL_DBG);

#define PI 3.14159

uint8_t buf[4096];

struct tm tm;

void sim_update(int t) {
    int pos = 0;
    time_t time = t;
    if (localtime_r(&time, &tm) == NULL) {
        LOG_ERR("Failed to generate time");
        return;
    }

    pos += sprintf(&buf[pos], "/ASD5id123\r\n\r\n");
    pos += sprintf(&buf[pos], "0-0:1.0.0(%02d%02d%02d%02d%02d%02dW)\r\n", 
        tm.tm_year % 100, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec); 
    pos += sprintf(&buf[pos], "1-0:1.8.0(%012.3f*kWh)\r\n", 0.394 * t);
    pos += sprintf(&buf[pos], "1-0:2.8.0(%012.3f*kWh)\r\n", 0.128 * t);
    pos += sprintf(&buf[pos], "1-0:3.8.0(%012.3f*kvarh)\r\n", 0.074 * t);
    pos += sprintf(&buf[pos], "1-0:4.8.0(%012.3f*kvarh)\r\n", 0.018 * t);
    pos += sprintf(&buf[pos], "1-0:1.7.0(%08.3f*kW)\r\n",  5 + 5 * sin(PI * t / 10.0));
    pos += sprintf(&buf[pos], "1-0:2.7.0(%08.3f*kW)\r\n",  3 + 3 * cos(PI * t / 10.0));
    pos += sprintf(&buf[pos], "1-0:3.7.0(%08.3f*kvar)\r\n", 0.5 + 0.5 * sin(PI * t / 7.0));
    pos += sprintf(&buf[pos], "1-0:4.7.0(%08.3f*kvar)\r\n", 0.2 + 0.2 * cos(PI * t / 7.0));
    pos += sprintf(&buf[pos], "1-0:32.7.0(%05.1f*V)\r\n", 240 + sin(PI * t / 10) * 5);
    pos += sprintf(&buf[pos], "1-0:52.7.0(%05.1f*V)\r\n", 240 + sin(PI * t / 9) * 5);
    pos += sprintf(&buf[pos], "1-0:72.7.0(%05.1f*V)\r\n", 240 + sin(PI * t / 8) * 5);
    pos += sprintf(&buf[pos], "1-0:31.7.0(%05.1f*A)\r\n", 5 + sin(PI * t / 10) * 5);
    pos += sprintf(&buf[pos], "1-0:51.7.0(%05.1f*A)\r\n", 5 + sin(PI * t / 9) * 5);
    pos += sprintf(&buf[pos], "1-0:71.7.0(%05.1f*A)\r\n", 5 + sin(PI * t / 8) * 5);
    pos += sprintf(&buf[pos], "!");
    uint16_t checksum = crc16_reflect(0xa001, 0, buf, pos);
    pos += sprintf(&buf[pos], "%4X\r\n", checksum);
}

void sim_fifo(void *, void *, void *) {
    size_t bytes_written;
    int t = 0;
    while (true) {
        k_sleep(K_MSEC(3000));
        sim_update(t++);
        LOG_INF("Transmitting");
        int len = strlen(buf);
        int ret = k_pipe_put(SIM_OUTPUT, buf, len, &bytes_written, len, K_NO_WAIT);
        if (ret < 0) {
            LOG_ERR("Failed to send simulated data to fifo: %d", ret);
        }
    }
}

K_THREAD_DEFINE(sim_fifo_thread, STACKSIZE,
                sim_fifo, NULL, NULL, NULL,
                PRIORITY, 0, 0);
    
#endif
