#pragma once

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
		gint port, void (*connectcallack)(struct mosquitto* mosq, void* data),
		void* data);
