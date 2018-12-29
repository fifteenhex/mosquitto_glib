#include <glib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "mosquitto_client.h"

static guint signal_connected;
static guint signal_disconnected;
static guint signal_subscribe;
static guint signal_unsubscribe;
static guint signal_publish;
static guint signal_message;

struct _MosquittoClient {
	GObject parent_instance;
	struct mosquitto* mosq;
	const gchar* host;
	gint port;
	gboolean connected;
	GIOChannel* mosqchan;
	guint mosqsource;
};

G_DEFINE_TYPE( MosquittoClient, mosquitto_client, G_TYPE_OBJECT);

static void mosquitto_doreadwrite(MosquittoClient* client) {
	//workaround for https://github.com/eclipse/mosquitto/issues/990
	if (LIBMOSQUITTO_VERSION_NUMBER < 1005004)
		mosquitto_loop(client->mosq, 100, 1);

	if (!client->connected)
		return;

	int ret;
	if ((ret = mosquitto_loop_read(client->mosq, 1)) != MOSQ_ERR_SUCCESS) {
		g_message("read err %d", ret);
		goto err;
	}
	if (mosquitto_want_write(client->mosq)) {
		if ((ret = mosquitto_loop_write(client->mosq, 1)) != MOSQ_ERR_SUCCESS) {
			g_message("write err %d", ret);
			goto err;
		}
	}
	return;
	err: client->connected = false;
}

static gboolean mosquitto_client_handlemosq(GIOChannel *source,
		GIOCondition condition, gpointer data) {
	MosquittoClient* client = (MosquittoClient*) data;

	if (condition & G_IO_IN)
		mosquitto_doreadwrite(client);

	return TRUE;
}

static void mosquitto_client_log(struct mosquitto* mosq, void* userdata,
		int level, const char* str) {
	g_message(str);
}

static void mosquitto_client_mosqcb_connect(struct mosquitto* mosq,
		void* userdata, int returncode) {
	MosquittoClient* client = (MosquittoClient*) userdata;
	g_signal_emit(client, signal_connected, 0, NULL);
}

static void mosquitto_client_mosqb_subscribe(struct mosquitto* mosq,
		void* userdata, int mid, int qos_count, const int* granted_qos) {
	MosquittoClient* client = (MosquittoClient*) userdata;
	g_signal_emit(client, signal_subscribe, 0, NULL);
}

static void mosquitto_client_mosqcb_publish(struct mosquitto* mosq,
		void* userdata, int mid) {

}

static void mosquitto_client_mosqb_unsubscribe(struct mosquitto* mosq,
		void* userdata, int mid) {
	MosquittoClient* client = (MosquittoClient*) userdata;
}

static void mosquitto_client_mosqcb_message(struct mosquitto* mosq,
		void* userdata, const struct mosquitto_message* msg) {
	MosquittoClient* client = (MosquittoClient*) userdata;
	g_signal_emit(client, signal_message, 0, msg);
}

static gboolean mosquitto_client_idle(gpointer data) {
	MosquittoClient* client = (MosquittoClient*) data;

	gboolean connected = FALSE;

	// This seems like the only way to work out if
	// we ever connected or got disconnected at
	// some point
	if (mosquitto_loop_misc(client->mosq) == MOSQ_ERR_NO_CONN) {
		if (client->mosqchan != NULL) {
			g_source_remove(client->mosqsource);
			// g_io_channel_shutdown doesn't work :/
			close(mosquitto_socket(client->mosq));
			client->mosqchan = NULL;
		}

		if (mosquitto_connect(client->mosq, client->host, client->port, 60)
				== MOSQ_ERR_SUCCESS) {

			int mosqfd = mosquitto_socket(client->mosq);
			client->mosqchan = g_io_channel_unix_new(mosqfd);
			client->mosqsource = g_io_add_watch(client->mosqchan,
					G_IO_IN /*| G_IO_OUT*/, mosquitto_client_handlemosq,
					client);

			g_io_channel_unref(client->mosqchan);
			connected = TRUE;
		} else
			g_message("connect failed");
	} else
		connected = TRUE;

	client->connected = connected;

	mosquitto_doreadwrite(client);

	return TRUE;
}

static void mosquitto_client_class_init(MosquittoClientClass *klass) {
	mosquitto_lib_init();

	GSignalFlags flags = G_SIGNAL_RUN_LAST | G_SIGNAL_NO_HOOKS
			| G_SIGNAL_NO_RECURSE | G_SIGNAL_DETAILED;
	GType params[] = { G_TYPE_POINTER };
	signal_connected = g_signal_newv(MOSQUITTO_CLIENT_SIGNAL_CONNECTED,
			MOSQUITTO_TYPE_CLIENT, flags, NULL, NULL, NULL, NULL, G_TYPE_NONE,
			G_N_ELEMENTS(params), params);
	signal_disconnected = g_signal_newv(MOSQUITTO_CLIENT_SIGNAL_DISCONNECTED,
			MOSQUITTO_TYPE_CLIENT, flags, NULL, NULL, NULL, NULL, G_TYPE_NONE,
			G_N_ELEMENTS(params), params);
	signal_subscribe = g_signal_newv(MOSQUITTO_CLIENT_SIGNAL_SUBSCRIBE,
			MOSQUITTO_TYPE_CLIENT, flags, NULL, NULL, NULL, NULL, G_TYPE_NONE,
			G_N_ELEMENTS(params), params);
	signal_unsubscribe = g_signal_newv(MOSQUITTO_CLIENT_SIGNAL_UNSUBSCRIBE,
			MOSQUITTO_TYPE_CLIENT, flags, NULL, NULL, NULL, NULL, G_TYPE_NONE,
			G_N_ELEMENTS(params), params);
	signal_publish = g_signal_newv(MOSQUITTO_CLIENT_SIGNAL_PUBLISH,
			MOSQUITTO_TYPE_CLIENT, flags, NULL, NULL, NULL, NULL, G_TYPE_NONE,
			G_N_ELEMENTS(params), params);
	signal_message = g_signal_newv(MOSQUITTO_CLIENT_SIGNAL_MESSAGE,
			MOSQUITTO_TYPE_CLIENT, flags, NULL, NULL, NULL, NULL, G_TYPE_NONE,
			G_N_ELEMENTS(params), params);
}

static void mosquitto_client_init(MosquittoClient *self) {
}

static MosquittoClient* mosquitto_client_new_full(const gchar* id,
		const gchar* host, gint port, gboolean tls, const gchar* rootcert,
		const gchar* clientcert, const gchar* clientkey) {
	MosquittoClient* client = g_object_new(MOSQUITTO_TYPE_CLIENT, NULL);

	client->host = host;
	client->port = port;

	client->mosq = mosquitto_new(id, true, client);
	mosquitto_threaded_set(client->mosq, TRUE);

#if MOSQUITTO_GLIB_LOG
	mosquitto_log_callback_set(client->mosq, mosquitto_client_log);
#endif
	mosquitto_connect_callback_set(client->mosq,
			mosquitto_client_mosqcb_connect);
	mosquitto_subscribe_callback_set(client->mosq,
			mosquitto_client_mosqb_subscribe);
	mosquitto_unsubscribe_callback_set(client->mosq,
			mosquitto_client_mosqb_unsubscribe);
	mosquitto_publish_callback_set(client->mosq,
			mosquitto_client_mosqcb_publish);
	mosquitto_message_callback_set(client->mosq,
			mosquitto_client_mosqcb_message);

	if (tls) {
		int ret = mosquitto_tls_set(client->mosq, rootcert,
		NULL, clientcert, clientkey, NULL);
		g_assert(ret == MOSQ_ERR_SUCCESS);
	}

	g_timeout_add(10, mosquitto_client_idle, client);
	return client;
}

MosquittoClient* mosquitto_client_new_withclientcert(const gchar* id,
		const gchar* host, gint port, const gchar* rootcert,
		const gchar* clientcert, const gchar* clientkey) {

	g_assert(clientcert != NULL && clientkey != NULL);
	g_message("using client cert/key %s %s", clientcert, clientkey);

	return mosquitto_client_new_full(id, host, port, TRUE, rootcert, clientcert,
			clientkey);
}

MosquittoClient* mosquitto_client_new_plaintext(const gchar* id,
		const gchar* host, gint port) {
	return mosquitto_client_new_full(id, host, port, FALSE, NULL, NULL,
	NULL);
}

struct mosquitto* mosquitto_client_getmosquittoinstance(MosquittoClient* client) {
	return client->mosq;
}

gboolean mosquitto_client_isconnected(MosquittoClient* client) {
	return client->connected;
}

gchar* mosquitto_client_createtopic(const gchar* root, ...) {
	GString* topicstr = g_string_new(NULL);
	g_string_append(topicstr, root);

	va_list args;
	va_start(args, root);

	const gchar * part = va_arg(args, const gchar*);
	for (; part != NULL; part = va_arg(args, const gchar*)) {
		g_string_append(topicstr, "/");
		g_string_append(topicstr, part);
	}

	va_end(args);
	gchar* topic = g_string_free(topicstr, FALSE);
	return topic;
}

#ifdef MOSQUITTO_CLIENT_JSON

static gchar* jsonbuilder_freetostring(JsonBuilder* jsonbuilder, gsize* jsonlen,
		gboolean pretty) {
	JsonNode* rootnode = json_builder_get_root(jsonbuilder);
	JsonGenerator* generator = json_generator_new();
	json_generator_set_root(generator, rootnode);
	json_generator_set_pretty(generator, pretty);

	gchar* json = json_generator_to_data(generator, jsonlen);
	g_object_unref(generator);
	g_object_unref(jsonbuilder);
	return json;
}

void mosquitto_client_publish_json_builder(MosquittoClient* client, JsonBuilder* builder,
		const gchar* topic) {
	gsize jsonlen;
	gchar* json = jsonbuilder_freetostring(builder, &jsonlen, FALSE);
	mosquitto_publish(mosquitto_client_getmosquittoinstance(client),
			NULL, topic, jsonlen, json, 0, FALSE);
	g_free(json);
}

void mosquitto_client_publish_json(MosquittoClient* client, JsonNode* root,
		const gchar* topic) {
	JsonGenerator* jsongenerator = json_generator_new();
	json_generator_set_root(jsongenerator, root);
	gsize publishpayloadsz;
	gchar* publishpayload = json_generator_to_data(jsongenerator,
			&publishpayloadsz);
	mosquitto_publish(client->mosq, NULL, topic, publishpayloadsz,
			publishpayload, 0, false);
	g_free(publishpayload);
	g_object_unref(jsongenerator);
}
#endif
