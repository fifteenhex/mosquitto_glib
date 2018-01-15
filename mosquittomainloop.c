#include <mosquitto.h>
#include <glib.h>
#include <string.h>
#include <unistd.h>

#include "mosquittomainloop.h"

static void mosq_log(struct mosquitto* mosq, void* userdata, int level,
		const char* str) {
	g_message(str);
}

static gboolean handlemosq(GIOChannel *source, GIOCondition condition,
		gpointer data) {
	struct mosquitto* mosq = (struct mosquitto*) data;
	mosquitto_loop_read(mosq, 1);
	return TRUE;
}

static gboolean mosq_idle(gpointer data) {
	struct mosquitto_context* cntx = (struct mosquitto_context*) data;

	gboolean connected = FALSE;

	// This seems like the only way to work out if
	// we ever connected or got disconnected at
	// some point
	if (mosquitto_loop_misc(cntx->mosq) == MOSQ_ERR_NO_CONN) {
		if (cntx->mosqchan != NULL) {
			g_source_remove(cntx->mosqsource);
			// g_io_channel_shutdown doesn't work :/
			close(mosquitto_socket(cntx->mosq));
			cntx->mosqchan = NULL;
		}

		if (mosquitto_connect(cntx->mosq, cntx->mqtthost, cntx->mqttport, 60)
				== MOSQ_ERR_SUCCESS) {
			int mosqfd = mosquitto_socket(cntx->mosq);
			cntx->mosqchan = g_io_channel_unix_new(mosqfd);
			cntx->mosqsource = g_io_add_watch(cntx->mosqchan, G_IO_IN,
					handlemosq, cntx->mosq);
			g_io_channel_unref(cntx->mosqchan);
			connected = TRUE;

			if (cntx->connectcallack != NULL) {
				cntx->connectcallack(cntx->mosq, cntx->data);
			}
		}
	} else
		connected = TRUE;

	if (connected) {
		mosquitto_loop_read(cntx->mosq, 1);
		mosquitto_loop_write(cntx->mosq, 1);
	}
	return TRUE;
}

void mosquittomainloop(struct mosquitto_context* cntx, const gchar* host,
		gint port, gboolean log,
		void (*connectcallack)(struct mosquitto* mosq, void* data), void* data) {

	memset(cntx, 0, sizeof(*cntx));
	cntx->mqtthost = host;
	cntx->mqttport = port;
	cntx->connectcallack = connectcallack;
	cntx->data = data;

	mosquitto_lib_init();
	cntx->mosq = mosquitto_new(NULL, true, data);

	if (log)
		mosquitto_log_callback_set(cntx->mosq, mosq_log);

	g_timeout_add(500, mosq_idle, cntx);
}
