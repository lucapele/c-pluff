/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2005 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

#ifndef _CPLUFF_H_
#define _CPLUFF_H_

/* Define CP_EXPORT for Windows host */
#ifdef __WIN32__
#ifdef CP_BUILD
#define CP_EXPORT __declspec(dllexport)
#else /*CP_BUILD*/
#define CP_EXPORT __declspec(dllimport)
#endif
#else /*__WIN32__*/
#define CP_EXPORT
#endif /*__WIN32__*/

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


/* ------------------------------------------------------------------------
 * Constants
 * ----------------------------------------------------------------------*/

/**
 * Maximum length of a plug-in, extension or extension point identifier in
 * bytes, excluding the trailing '\0'
 */
#define CP_ID_MAX_LENGTH 63


/* Return values for operations that might fail */

/** Operation performed successfully */
#define CP_OK 0

/** An unspecified error occurred */
#define CP_ERR_UNSPECIFIED (-1)

/** Not enough memory or other OS resources available */
#define CP_ERR_RESOURCE (-2)

/** The specified object is unknown to the framework */
#define CP_ERR_UNKNOWN (-3)

/** An I/O error occurred */
#define CP_ERR_IO (-4)

/** Malformed plug-in data when loading a plug-in */
#define CP_ERR_MALFORMED (-5)

/** Plug-in conflicts with an existing plug-in when loading a plug-in */
#define CP_ERR_CONFLICT (-6)


/* Flags for cp_rescan_plugins */

/** Setting this flag prevents uninstallation of plug-ins */
#define CP_RESCAN_NO_UNINSTALL 0x01

/** Setting this flag prevents downgrading of installed plug-ins */
#define CP_RESCAN_NO_DOWNGRADE 0x02

/** Setting this flag prevents installation of new plug-ins */
#define CP_RESCAN_NO_INSTALL 0x04

/** Setting this flag prevents upgrading of installed plug-ins */
#define CP_RESCAN_NO_UPGRAGE 0x08

/** This bitmask corresponds to full rescan */
#define CP_RESCAN_FULL 0x0

/** This bitmask allows for incremental installs and upgrades only */
#define CP_RESCAN_INCREMENTAL (CP_RESCAN_NO_UNINSTALL | CP_RESCAN_NO_DOWNGRADE)


/* ------------------------------------------------------------------------
 * Data types
 * ----------------------------------------------------------------------*/

/** A component identifier in UTF-8 encoding */
typedef char cp_id_t[CP_ID_MAX_LENGTH + 1];

/** Possible plug-in states */
typedef enum cp_plugin_state_t {
	CP_PLUGIN_UNINSTALLED,
	CP_PLUGIN_INSTALLED,
	CP_PLUGIN_RESOLVED,
	CP_PLUGIN_STARTING,
	CP_PLUGIN_STOPPING,
	CP_PLUGIN_ACTIVE
} cp_plugin_state_t;

/**
 * Describes a plug-in status event.
 */
typedef struct cp_plugin_event_t {
	
	/** The affected plug-in */
	cp_id_t plugin_id;
	
	/** Old state of the plug-in */
	cp_plugin_state_t old_state;
	
	/** New state of the plug-in */
	cp_plugin_state_t new_state;
	
} cp_plugin_event_t;

/** 
 * An error handler function called when a recoverable error occurs. An error
 * handler function should return promptly and it must not register or
 * unregister error handlers.
 */
typedef void (*cp_error_handler_t)(const char *msg);

/**
 * An event listener function called synchronously after a plugin state change.
 * An event listener function should return promptly and it must not register
 * or unregister event listeners.
 */
typedef void (*cp_event_listener_t)(const cp_plugin_event_t *event);


/* ------------------------------------------------------------------------
 * Function declarations
 * ----------------------------------------------------------------------*/


/* Initialization and destroy functions */

/**
 * Initializes the C-Pluff framework. The framework must be initialized before
 * trying to use it. This function does nothing if the framework has already
 * been initialized but the framework will be uninitialized only after the
 * corresponding number of calls to cpluff_destroy.
 * 
 * @return CP_OK (0) on success, CP_ERR_RESOURCE if out of resources
 */
CP_EXPORT int cp_init(void);

/**
 * Stops and unloads all plug-ins and releases all resources allocated by
 * the C-Pluff framework. Framework functionality and data structures are not
 * available after calling this function. If cpluff_init has been called
 * multiple times then the actual uninitialization takes place only
 * after corresponding number of calls to cpluff_destroy. The framework may be
 * reinitialized by calling cpluff_init function.
 */
CP_EXPORT void cp_destroy(void);


/* Functions for error handling */

/**
 * Adds an error handler function that will be called by the plug-in
 * framework when an error occurs. For example, failures to start and register
 * plug-ins are reported to the registered error handlers. Error messages are
 * localized, if possible. There can be several registered error handlers.
 * 
 * @param error_handler the error handler to be added
 * @return CP_OK (0) on success, CP_ERR_RESOURCE if out of resources
 */
CP_EXPORT int cp_add_error_handler(cp_error_handler_t error_handler);

/**
 * Removes an error handler. This function does nothing if the specified error
 * handler has not been registered.
 * 
 * @param error_handler the error handler to be removed
 */
CP_EXPORT void cp_remove_error_handler(cp_error_handler_t error_handler);


/* Functions for registering plug-in event listeners */

/**
 * Adds an event listener which will be called on plug-in state changes.
 * The event listener is called synchronously immediately after plug-in state
 * has changed. There can be several registered listeners.
 * 
 * @param event_listener the event_listener to be added
 * @return CP_OK (0) on success, CP_ERR_RESOURCE if out of resources
 */
CP_EXPORT int cp_add_event_listener(cp_event_listener_t event_listener);

/**
 * Removes an event listener. This function does nothing if the specified
 * listener has not been registered.
 * 
 * @param event_listener the event listener to be removed
 */
CP_EXPORT void cp_remove_event_listener(cp_event_listener_t event_listener);


/* Functions for controlling plug-ins */

/**
 * (Re)scans for plug-ins in the specified directory, reloading updated (and
 * downgraded) plug-ins, loading new plug-ins and unloading plug-ins that do
 * not exist anymore. This method can also be used to initially load all the
 * plug-ins. Flags can be specified to inhibit some operations.
 * 
 * @param dir the directory containing plug-ins
 * @param flags a bitmask specifying flags (CPLUFF_RESCAN_...)
 * @return CP_OK (0) if the scanning was successful or an error code
 *   if there were errors while loading some plug-ins
 */
CP_EXPORT int cp_rescan_plugins(const char *dir, int flags);

/**
 * Loads a plug-in from the specified path. The plug-in is added to the list of
 * installed plug-ins. If loading fails then NULL is returned.
 * 
 * @param path the installation path of the plug-in
 * @param id the identifier of the loaded plug-in is copied to the location
 *     pointed to by this pointer if this pointer is non-NULL
 * @return CP_OK (0) on success or an error code on failure
 */
CP_EXPORT int cp_load_plugin(const char *path, cp_id_t *id);

/**
 * Starts a plug-in. The plug-in is first resolved, if necessary, and all
 * imported plug-ins are started. If the plug-in is already starting then
 * this function blocks until the plug-in has started or failed to start.
 * If the plug-in is already active then this function returns immediately.
 * If the plug-in is stopping then this function blocks until the plug-in
 * has stopped and then starts the plug-in.
 * 
 * @param id identifier of the plug-in to be started
 * @return CP_OK (0) on success or an error code on failure
 */
CP_EXPORT int cp_start_plugin(const char *id);

/**
 * Stops a plug-in. First stops any importing plug-ins that are currently
 * active. Then stops the specified plug-in. If the plug-in is already
 * stopping then this function blocks until the plug-in has stopped. If the
 * plug-in is already stopped then this function returns immediately. If the
 * plug-in is starting then this function blocks until the plug-in has
 * started (or failed to start) and then stops the plug-in.
 * 
 * @param id identifier of the plug-in to be stopped
 * @return CP_OK (0) on success or CP_ERR_UNKNOWN if no such plug-in exists
 */
CP_EXPORT int cp_stop_plugin(const char *id);

/**
 * Stops all active plug-ins.
 */
CP_EXPORT void cp_stop_all_plugins(void);

/**
 * Unloads an installed plug-in. The plug-in is first stopped if it is active.
 * 
 * @param id identifier of the plug-in to be unloaded
 * @return CP_OK (0) on success or CP_ERR_UNKNOWN if no such plug-in exists
 */
CP_EXPORT int cp_unload_plugin(const char *id);

/**
 * Unloads all plug-ins. This effectively stops all plug-in activity and
 * releases the resources allocated by the plug-ins.
 */
CP_EXPORT void cp_unload_all_plugins(void);


#ifdef __cplusplus
} /*extern "C"*/
#endif /*__cplusplus*/

#endif /*_CPLUFF_H_*/
