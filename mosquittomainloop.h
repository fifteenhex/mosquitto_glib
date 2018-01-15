#pragma once

#define MQTTOPTS	{ "mqtthost", 'h', 0, G_OPTION_ARG_STRING, &mqtthost, "", "" },\
					{ "mqttport", 'p', 0, G_OPTION_ARG_INT, &mqttport, "", "" }

struct mosquitto_context {
	struct mosquitto* mosq;
	const gchar* mqtthost;
	gint mqttport;
	GIOChannel* mosqchan;
	guint mosqsource;
	void (*connectcallack)(struct mosquitto* mosq, void* data);
	void* data;
};

void mosquittomainloop(struct mosquitto_context* cntx, const gchar* host,
		gint port, gboolean log,
		void (*connectcallack)(struct mosquitto* mosq, void* data), void* data);
