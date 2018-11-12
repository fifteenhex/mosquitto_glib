#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define MQTTOPTS	{ "mqttid", 0, 0, G_OPTION_ARG_STRING, &mqttid, "", "" },\
					{ "mqtthost", 'h', 0, G_OPTION_ARG_STRING, &mqtthost, "", "" },\
					{ "mqttport", 'p', 0, G_OPTION_ARG_INT, &mqttport, "", "" },\
					{ "mqttrootcert", 0, 0, G_OPTION_ARG_STRING, &mqttrootcert, "", "" },\
					{ "mqttdevicecert", 0, 0, G_OPTION_ARG_STRING, &mqttdevicecert, "", "" },\
					{ "mqttdevicekey", 0, 0, G_OPTION_ARG_STRING, &mqttdevicekey, "", "" }

#define MOSQUITTO_TYPE_CLIENT mosquitto_client_get_type ()
G_DECLARE_FINAL_TYPE(MosquittoClient, mosquitto_client, MOSQUITTO, CLIENT,
		GObject);

#define MOSQUITTO_CLIENT_SIGNAL_CONNECTED		"connected"
#define MOSQUITTO_CLIENT_SIGNAL_DISCONNECTED	"disconnected"
#define MOSQUITTO_CLIENT_SIGNAL_SUBSCRIBE		"subscribe"
#define MOSQUITTO_CLIENT_SIGNAL_UNSUBSCRIBE		"unsubscribe"
#define MOSQUITTO_CLIENT_SIGNAL_PUBLISH			"publish"
#define MOSQUITTO_CLIENT_SIGNAL_MESSAGE			"message"

typedef gboolean (*mosquitto_client_connectioncallback)(MosquittoClient* client,
		void* something, gpointer user_data);

MosquittoClient* mosquitto_client_new_withclientcert(const gchar* id,
		const gchar* host, gint port, const gchar* rootcert,
		const gchar* clientcert, const gchar* clientkey);
MosquittoClient* mosquitto_client_new_plaintext(const gchar* id,
		const gchar* host, gint port);
struct mosquitto* mosquitto_client_getmosquittoinstance(MosquittoClient* client);
gboolean mosquitto_client_isconnected(MosquittoClient* client);

G_END_DECLS
