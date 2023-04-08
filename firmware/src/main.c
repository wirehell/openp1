#include "lib/openp1.h"
#include "lib/parser.h"
#include "lib/telegram.h"
#include "lib/value_store.h"

#include "uart_p1.h"
#include "framer_task.h"
#include "parser_task.h"
#include "handler_task.h"
#include "modbus.h"
#include "tcp_log.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/net/openthread.h>
#include <openthread/srp_client.h>
#include <openthread/instance.h>

K_PIPE_DEFINE(rx_pipe, 4096, 4);
K_PIPE_DEFINE(tx_pipe, 4096, 4);
K_FIFO_DEFINE(telegram_frame_fifo);
K_MSGQ_DEFINE(telegram_queue, sizeof(telegram_message), 1, 4); 

LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

const char HOSTNAME[] = CONFIG_OPENP1_HOSTNAME; 

struct value_store value_store;

void update_data_store(struct data_item *data_item) {
	value_store_update(&value_store, data_item);
}

#if CONFIG_OPENTHREAD
//typedef void (*otSrpClientAutoStartCallback)(const otSockAddr *aServerSockAddr, void *aContext);
void on_ot_srp_client_autostart_cb(const otSockAddr *aServerSockAddr, void *aContext) {
	uint8_t buf[INET6_ADDRSTRLEN];
	otIp6AddressToString(&aServerSockAddr->mAddress, buf, INET6_ADDRSTRLEN);
	LOG_INF("SRP autostart callback. Server: %s", buf);
}

const char INSTANCE_NAME[] = CONFIG_OPENP1_SRP_INSTANCE_NAME;
#if CONFIG_OPENP1_UDP
const char SERVICE_LABEL[] = "_blep1-modbus._udp";
#elif CONFIG_OPENP1_TCP
const char SERVICE_LABEL[] = "_blep1-modbus._tcp";
#else
const char SERVICE_LABEL[] = "_blep1-unknown._unknown";
#endif
struct otSrpClientService srp_client_service = {0};

static int service_registration() {
	struct otInstance *otInstance = openthread_get_default_instance();
	if (otInstance == NULL) {
		LOG_ERR("Openthread instance not available");
		return -1;
	}
	int ret = otSrpClientEnableAutoHostAddress(otInstance);
	if (ret != OT_ERROR_NONE) {
		LOG_ERR("Could not set auto host name address, error: %d", ret);
		return -1;
	}
	ret = otSrpClientSetHostName(otInstance, HOSTNAME);
	if (ret != OT_ERROR_NONE) {
		LOG_ERR("Could not set host name address, error: %d", ret);
		return -1;
	}
	srp_client_service.mInstanceName = INSTANCE_NAME;
	srp_client_service.mName = SERVICE_LABEL;
	srp_client_service.mPort = 502;

	ret = otSrpClientAddService(otInstance, &srp_client_service);
	if (ret != OT_ERROR_NONE) {
		LOG_ERR("Could not set service, error: %d", ret);
		return -1;
	}
	otSrpClientEnableAutoStartMode(otInstance, &on_ot_srp_client_autostart_cb, NULL);
	LOG_INF("Service discovery setup done");

	return 0;
}
#else
static int service_registration() { 
	LOG_INF("No service registration configured");
	return 0;
}

#endif


void main(void) {
	int err;
	LOG_INF("Starting..");

#if CONFIG_OPENP1_SERIAL
	err = uart_p1_init(&rx_pipe, &tx_pipe);
	if (err < 0) {
		LOG_ERR("Could not init uart (err %d)", err);
		return;
	}
#endif

	err = framer_task_init(&rx_pipe, &telegram_frame_fifo);
	if (err < 0) {
		LOG_ERR("Could not init parser task (err %d)", err);
		return;
	}

	err = parser_task_init(&telegram_frame_fifo, &telegram_queue);
	if (err < 0) {
		LOG_ERR("Could not init parser task (err %d)", err);
		return;
	}

	err = handler_task_init(&telegram_queue, &update_data_store);
	if (err < 0) {
		LOG_ERR("Could not init handler task (err %d)", err);
		return;
	}

	err = modbus_init(&value_store);
	if (err < 0) {
		LOG_ERR("Could not initalize Modbus server");
		return;
	}

#if CONFIG_OPENP1_LOG_TCP
	err = tcp_log_server_start();
	if (err < 0) {
		LOG_ERR("Could not initalize tcp log server");
		return;
	}
#endif

	err = service_registration();
	if (err < 0) {
		LOG_ERR("Could not start service registration");
		return;
	}

	LOG_INF("Up and running..");

	while (true) {
		k_sleep(K_SECONDS(1));
	}
}
