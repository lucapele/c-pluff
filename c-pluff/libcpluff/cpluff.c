/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2006 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

// Core C-Pluff functions 

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include "cpluff.h"
#include "defines.h"
#include "util.h"
#ifdef CP_THREADS
#include "thread.h"
#endif
#include "internal.h"


/* ------------------------------------------------------------------------
 * Constants
 * ----------------------------------------------------------------------*/

/// Logging limit for no logging
#define CP_LOG_NONE 1000


/* ------------------------------------------------------------------------
 * Data types
 * ----------------------------------------------------------------------*/

typedef struct logger_t logger_t;

/// Contains information about installed loggers
struct logger_t {
	
	/// Pointer to logger
	cp_logger_t logger;
	
	/// User data pointer
	void *user_data;
	
	/// Minimum severity
	cp_log_severity_t min_severity;
	
	/// Context criteria
	cp_context_t *ctx_rule;
};


/* ------------------------------------------------------------------------
 * Variables
 * ----------------------------------------------------------------------*/

/// Implementation information 
static cp_implementation_info_t implementation_info = {
	CP_RELEASE_VERSION,
	CP_API_VERSION,
	CP_API_REVISION,
	CP_API_AGE,
	CP_HOST,
	CP_THREADS
};

/// Number of initializations 
static int initialized = 0;

#ifdef CP_THREADS

/// Framework mutex
static cpi_mutex_t *framework_mutex = NULL;

#elif !defined(NDEBUG)

/// Framework locking count
static int framework_locked = 0;

#endif

/// Loggers
static list_t *loggers = NULL;

/// Global minimum severity for logging
static int log_min_severity = CP_LOG_NONE;

/// Fatal error handler, or NULL for default 
static cp_fatal_error_handler_t fatal_error_handler = NULL;


/* ------------------------------------------------------------------------
 * Function definitions
 * ----------------------------------------------------------------------*/

cp_implementation_info_t * CP_API cp_get_implementation_info(void) {
	return &implementation_info;
}

void CP_LOCAL cpi_lock_framework(void) {
	#if defined(CP_THREADS)
	cpi_lock_mutex(framework_mutex);
#elif !defined(NDEBUG)
	framework_locked++;
#endif
}

void CP_LOCAL cpi_unlock_framework(void) {
#if defined(CP_THREADS)
	cpi_unlock_mutex(framework_mutex);
#elif !defined(NDEBUG)
	assert(framework_locked > 0);
	framework_locked--;
#endif
}

static void reset(void) {
	if (loggers != NULL) {
		lnode_t *node;
		
		while ((node = list_first(loggers)) != NULL) {
			logger_t *l = lnode_get(node);
			list_delete(loggers, node);
			lnode_destroy(node);
			free(l);
		}
		list_destroy(loggers);
		loggers = NULL;
	}
#ifdef CP_THREADS
	if (framework_mutex != NULL) {
		cpi_destroy_mutex(framework_mutex);
		framework_mutex = NULL;
	}
#endif
}

int CP_API cp_init(void) {
	int status = CP_OK;
	
	// Initialize if necessary
	do {
		if (!initialized) {
			if (bindtextdomain(PACKAGE, CP_DATADIR CP_FNAMESEP_STR "locale") == NULL) {
				status = CP_ERR_RESOURCE;
				break;
			}
#ifdef CP_THREADS
			if ((framework_mutex = cpi_create_mutex()) == NULL) {
				status = CP_ERR_RESOURCE;
				break;
			}
#endif
			if ((loggers = list_create(LISTCOUNT_T_MAX)) == NULL) {
				status = CP_ERR_RESOURCE;
				break;
			}
		}
		initialized++;
	} while (0);
	
	// Rollback on failure
	if (status != CP_OK) {
		reset();
	}
	
	return status;
}

void CP_API cp_destroy(void) {
	assert(initialized > 0);
	initialized--;
	if (!initialized) {
#ifdef CP_THREADS
		assert(framework_mutex == NULL || !cpi_is_mutex_locked(framework_mutex));
#else
		assert(!framework_locked);
#endif
		cpi_info(NULL, _("The plug-in framework is being shut down."));
		cpi_destroy_all_contexts();
		cpi_destroy_all_infos();
		reset();
	}
}

/**
 * Updates the global logging limits.
 */
static void update_logging_limits(void) {
	lnode_t *node;
	int nms = CP_LOG_NONE;
	
	node = list_first(loggers);
	while (node != NULL) {
		logger_t *lh = lnode_get(node);
		if (lh->min_severity < nms) {
			nms = lh->min_severity;
		}
		node = list_next(loggers, node);
	}
	log_min_severity = nms;
}

static int comp_logger(const void *p1, const void *p2) {
	const logger_t *l1 = p1;
	const logger_t *l2 = p2;
	return l1->logger != l2->logger;
}

int CP_API cp_add_logger(cp_logger_t logger, void *user_data, cp_log_severity_t min_severity, cp_context_t *ctx_rule) {
	logger_t l;
	logger_t *lh;
	lnode_t *node;

	assert(logger != NULL);
	
	// Check if logger already exists and allocate new holder if necessary
	l.logger = logger;
	cpi_lock_framework();
	if ((node = list_find(loggers, &l, comp_logger)) == NULL) {
		lh = malloc(sizeof(logger_t));
		node = lnode_create(lh);
		if (lh == NULL || node == NULL) {
			cpi_unlock_framework();
			if (lh != NULL) {
				free(lh);
			}
			if (node != NULL) {
				lnode_destroy(node);
			}
			cpi_error(NULL, _("Logger could not be registered due to insufficient memory."));
			return CP_ERR_RESOURCE;
		}
		lh->logger = logger;
		lh->user_data = user_data;
		list_append(loggers, node);
	} else {
		lh = lnode_get(node);
	}
		
	// Initialize or update the logger holder
	lh->min_severity = min_severity;
	lh->ctx_rule = ctx_rule;
		
	// Update global limits
	update_logging_limits();
	cpi_unlock_framework();

	cpi_debugf(NULL, "Logger %p was added or updated with minimum severity %d.", (void *) logger, min_severity);
	return CP_OK;
}

void CP_API cp_remove_logger(cp_logger_t logger) {
	logger_t l;
	lnode_t *node;
	
	l.logger = logger;
	cpi_lock_framework();
	if ((node = list_find(loggers, &l, comp_logger)) != NULL) {
		logger_t *lh = lnode_get(node);
		list_delete(loggers, node);
		lnode_destroy(node);
		free(lh);
		update_logging_limits();
	}
	cpi_unlock_framework();
	cpi_debugf(NULL, "Logger %p was removed.", (void *) logger);
}

static void log(cp_context_t *ctx, cp_log_severity_t severity, const char *msg) {
	lnode_t *node;
	
	cpi_lock_framework();
	node = list_first(loggers);
	while (node != NULL) {
		logger_t *lh = lnode_get(node);
		if (severity >= lh->min_severity
			&& (lh->ctx_rule == NULL || ctx == lh->ctx_rule)) {
			lh->logger(severity, msg, ctx, lh->user_data);
		}
		node = list_next(loggers, node);
	}
	cpi_unlock_framework();
}

void CP_LOCAL cpi_log(cp_context_t *ctx, cp_log_severity_t severity, const char *msg) {
	if (severity >= log_min_severity) {
		log(ctx, severity, msg);
	}
}

void CP_LOCAL cpi_logf(cp_context_t *ctx, cp_log_severity_t severity, const char *msg, ...) {
	if (severity >= log_min_severity) {
		char buffer[256];
		va_list va;
		
		va_start(va, msg);
		vsnprintf(buffer, sizeof(buffer), msg, va);
		va_end(va);
		buffer[sizeof(buffer)/sizeof(char) - 1] = '\0';
		log(ctx, severity, buffer);
	}
}

int CP_LOCAL cpi_is_logged(cp_log_severity_t severity) {
	return severity >= log_min_severity;
}

void CP_API cp_set_fatal_error_handler(cp_fatal_error_handler_t error_handler) {
	fatal_error_handler = error_handler;
}

void CP_LOCAL cpi_fatalf(const char *msg, ...) {
	va_list params;
	char fmsg[256];
		
	// Format message 
	assert(msg != NULL);
	va_start(params, msg);
	vsnprintf(fmsg, sizeof(fmsg), msg, params);
	va_end(params);
	fmsg[sizeof(fmsg)/sizeof(char) - 1] = '\0';

	// Call error handler or print the error message
	if (fatal_error_handler != NULL) {
		fatal_error_handler(fmsg);
	} else {
		fprintf(stderr, _(PACKAGE_NAME ": FATAL ERROR: %s\n"), fmsg);
	}
	
	// Abort if still alive 
	abort();
}
