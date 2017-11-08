#include <stdlib.h>
#include <string.h>
#include "libp2p/utils/logger.h"
#include "libp2p/net/protocol.h"

/*
 * Handle the different protocols
 */

/***
 * Compare incoming to see if they are requesting a protocol upgrade
 * @param incoming the incoming string
 * @param incoming_size the size of the incoming string
 * @param test the protocol string to compare it with (i.e. "/secio" or "/nodeio"
 * @returns true(1) if there was a match, false(0) otherwise
 */
const struct Libp2pProtocolHandler* protocol_compare(struct StreamMessage* msg, struct Libp2pVector* protocol_handlers) {
	for(int i = 0; i < protocol_handlers->total; i++) {
		const struct Libp2pProtocolHandler* handler = (const struct Libp2pProtocolHandler*) libp2p_utils_vector_get(protocol_handlers, i);
		if (handler->CanHandle(msg)) {
			return handler;
		}
	}
	return NULL;
}

/**
 * Allocate resources for a new Libp2pProtocolHandler
 * @returns an allocated struct
 */
struct Libp2pProtocolHandler* libp2p_protocol_handler_new() {
	struct Libp2pProtocolHandler* h = (struct Libp2pProtocolHandler*) malloc(sizeof(struct Libp2pProtocolHandler));
	if (h != NULL) {
		h->CanHandle = NULL;
		h->HandleMessage = NULL;
		h->Shutdown = NULL;
		h->context = NULL;
	}
	return h;
}

/***
 * Handle an incoming message
 * @param message the incoming message
 * @param session the SessionContext of the incoming connection
 * @param handlers a Vector of protocol handlers
 * @returns -1 on error, 0 if everything was okay, but the daemon should no longer handle this connection, 1 on success
 */
int libp2p_protocol_marshal(struct StreamMessage* msg, struct Stream* stream, struct Libp2pVector* handlers) {
	const struct Libp2pProtocolHandler* handler = protocol_compare(msg, handlers);

	if (handler == NULL) {
		// turn msg->data to a null terminated string for the error message
		char str[msg->data_size + 1];
		memcpy(str, msg->data, msg->data_size);
		str[msg->data_size] = 0;
		for(int i = 0; i < msg->data_size; i++) {
			if (str[i] == '\n') {
				str[i] = 0;
				break;
			}
		}
		return -1;
	}

	return handler->HandleMessage(msg, stream, handler->context);
}

/***
 * Shut down all protocol handlers and free vector
 * @param handlers vector of Libp2pProtocolHandler
 * @returns true(1)
 */
int libp2p_protocol_handlers_shutdown(struct Libp2pVector* handlers) {
	for(int i = 0; i < handlers->total; i++) {
		struct Libp2pProtocolHandler* handler = (struct Libp2pProtocolHandler*)libp2p_utils_vector_get(handlers, i);
		handler->Shutdown(handler->context);
	}
	libp2p_utils_vector_free(handlers);
	return 1;
}
