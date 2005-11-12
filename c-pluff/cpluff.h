/*-------------------------------------------------------------------------
 * C-Pluff, a plug-in framework for C
 * Copyright 2005 Johannes Lehtinen
 *-----------------------------------------------------------------------*/

#ifndef _CPLUFF_H_
#define _CPLUFF_H_

#ifdef __cplusplus
extern "C" {
#endif /*__cplusplus*/


/* ------------------------------------------------------------------------
 * Constants
 * ----------------------------------------------------------------------*/

/** Return value for successful operations */
#define CPLUFF_OK 0

/** Return value for failed operations */
#define CPLUFF_ERROR (-1)


/* ------------------------------------------------------------------------
 * Data types
 * ----------------------------------------------------------------------*/

/*
 * IMPORTANT NOTICE!
 * 
 * C-Pluff supports multi-threading (although it does not start new threads
 * on its own). External code accessing the global data structures of the
 * plug-in framework must use the provided locking functions (acquire and
 * release functions declared in this header file).
 */
 

/** Possible plug-in states */
typedef enum plugin_state_t {
	PLUGIN_UNINSTALLED,
	PLUGIN_INSTALLED,
	PLUGIN_RESOLVED,
	PLUGIN_STARTING,
	PLUGIN_STOPPING,
	PLUGIN_ACTIVE
} plugin_state_t;

/** Possible version match rules */
typedef enum version_match_t {
	MATCH_PERFECT,
	MATCH_EQUIVALENT,
	MATCH_COMPATIBLE,
	MATCH_GREATEROREQUAL
} version_match_t;

/* Forward type definitions */
typedef struct plugin_t plugin_t;
typedef struct plugin_import_t plugin_import_t;
typedef struct ext_point_t ext_point_t;
typedef struct extension_t extension_t;


/* Plug-in data structures */

/**
 * Extension point structure captures information about a deployed extension
 * point.
 */
struct ext_point_t {
	
	/**
	 * Human-readable, possibly localized, extension point name or NULL
	 * if not available
	 */
	char *name;
	
	/** 
	 * Simple identifier uniquely identifying the extension point within the
	 * providing plug-in
	 */
	char *simple_id;
	
	/**
	 * Unique identifier constructed by concatenating the plug-in identifier,
	 * period character '.' and the simple identifier
	 */
	char *unique_id;
	
	/** Plug-in providing this extension point */
	plugin_t *plugin;
	
};

/**
 * Extension structure captures information about a deployed extension.
 */
struct extension_t {
	
	/** 
	 * Human-readable, possibly localized, extension name or NULL if not
	 * available
	 **/
	char *name;
	
	/**
	 * Simple identifier uniquely identifying the extension within the
	 * providing plug-in or NULL if not available
	 */
	 char *simple_id;
	 
	/**
	 * Unique identifier constructed by concatenating the plug-in identifier,
	 * period character '.' and the simple identifier or NULL if not available
	 */
	char *unique_id;
	  
	/** Unique identifier of the extension point */
	char *extpt_id;
	 
	/** Plug-in providing this extension */
	plugin_t *plugin;

};

/**
 * Information about plug-in import.
 */
struct plugin_import_t {
	
	/** Identifier of the imported plug-in */
	char *identifier;
	
	/** Version to be matched, or NULL if none */
	char *version;
	
	/** Version match rule */
	version_match_t match;
	
	/** Whether this import is optional */
	int optional;
};

/**
 * Plug-in structure captures information about a deployed plug-in.
 */
struct plugin_t {
	
	/** State of this plug-in */
	plugin_state_t state;
	
	/** Human-readable, possibly localized, plug-in name */
	char *name;
	
	/** Unique identifier */
	char *identifier;
	
	/** Version string */
	char *versionString;
	
	/** Provider name, possibly localized */
	char *providerName;
	
	/** Canonical path of the plugin directory */
	char *path;
	
	/** Number of imports */
	int num_imports;
	
	/** Imports */
	plugin_import_t *imports;

    /** The relative path of plug-in runtime library */
    char *lib_path;
    
    /** The name of the start function */
    char *start_func_name;
    
    /** The name of the stop function */
    char *stop_func_name;

	/** Number of extension points provided by this plug-in */
	int num_ext_points;
	
	/** Extension points provided by this plug-in */
	ext_point_t *ext_points;
	
	/** Number of extensions provided by this plugin */
	int num_extensions;
	
	/** Extensions provided by this plug-in */
	extension_t *extensions;


	/* Plug-in list structure */
	
	/** Previous installed plug-in, or NULL if first */
	plugin_t *previous;
	
	/** Next installed plug-in, or NULL if last */
	plugin_t *next;

};

/**
 * Describes a plug-in status event.
 */
typedef struct plugin_event_t {
	
	/** The associated plug-in */
	plugin_t *plugin;
	
	/** Old state of the plug-in */
	plugin_state_t old_state;
	
	/** New state of the plug-in */
	plugin_state_t new_state;
	
} plugin_event_t;


/* ------------------------------------------------------------------------
 * External variables
 * ----------------------------------------------------------------------*/

/** First installed plug-in, or NULL if none */
plugin_t *plugins;


/* ------------------------------------------------------------------------
 * Function declarations
 * ----------------------------------------------------------------------*/


/* Initialization and destroy functions */

/**
 * Initializes the C-Pluff framework. The framework must be initialized before
 * trying to use its functionality or the associated data structures. This
 * function does nothing if the framework has already been initialized.
 * 
 * @return CPLUFF_OK (0) on success, CPLUFF_ERROR (-1) on failure
 */
int cpluff_init(void);

/**
 * Stops and unloads all plug-ins and releases all resources allocated by
 * the C-Pluff framework. Framework functionality and data structures are not
 * available after calling this function. This function does nothing if the
 * framework has not been initialized. The framework may be reinitialized by
 * calling cpluff_init function.
 */
void cpluff_destroy(void);


/* Functions for error handling */

/**
 * Adds an error handler function that will be called by the plug-in
 * framework when an error occurs. For example, failures to start and register
 * plug-ins are reported to the error handler function, if set. Error messages
 * are localized, if possible. There can be several registered error handlers.
 * This function does nothing and returns CPLUFF_OK if the specified error
 * handler has already been registered.
 * 
 * @param error_handler the error handler to be added
 * @return CPLUFF_OK (0) on success, CPLUFF_ERROR (-1) on failure
 */
int add_cp_error_handler(void (*error_handler)(const char *msg));

/**
 * Removes an error handler. This function does nothing if the specified error
 * handler has not been registered.
 * 
 * @param error_handler the error handler to be removed
 */
void remove_cp_error_handler(void (*error_handler)(const char *msg));


/* Functions for registering plug-in event listeners */

/**
 * Adds an event listener which will be called on plug-in state changes.
 * The event listener is called synchronously immediately after plug-in state
 * has changed, or immediately before uninstalling a plug-in. The data
 * structures lock is being held while calling the listener so the listener
 * should return promptly. There can be several registered listeners. This
 * function does nothing and returns CPLUFF_OK if the specified listener has
 * already been registered.
 * 
 * @param event_listener the event_listener to be added
 * @return CPLUFF_OK (0) on success, CPLUFF_ERROR (-1) on failure
 */
int add_cp_event_listener
	(void (*event_listener)(const plugin_event_t *event));

/**
 * Removes an event listener. This function does nothing if the specified
 * listener has not been registered.
 * 
 * @param event_listener the event listener to be removed
 */
void remove_cp_event_listener
	(void (*event_listener)(const plugin_event_t *event));


/* Functions for finding plug-ins, extension points and extensions */

/**
 * Returns the plug-in having the specified identifier.
 * 
 * @param id the identifier of the plug-in
 * @return plug-in or NULL if no such plug-in exists
 */
plugin_t *find_plugin(const char *id);

/**
 * Returns the extension point having the specified identifier.
 * 
 * @param id the identifier of the extension point
 * @return extension point or NULL if no such extension point exists
 */
ext_point_t *find_ext_point(const char *id);

/**
 * Returns the extension having the specified identifier.
 * 
 * @param id the identifier of the extension
 * @return extension or NULL if no such extension exists
 */
extension_t *find_extension(const char *id);


/* Functions for controlling plug-ins */

/**
 * Scans for plug-ins in the specified directory and loads all plug-ins
 * found.
 * 
 * @param dir the directory containing plug-ins
 * @return the number of successfully deployed new plug-ins
 */
int scan_plugins(const char *dir);

/**
 * Loads a plug-in from a specified path. The plug-in is added to the list of
 * installed plug-ins. If the plug-in at the specified location has already
 * been loaded then a reference to the installed plug-in is
 * returned. If loading fails then NULL is returned.
 * 
 * @param path the installation path of the plug-in
 * @return reference to the plug-in, or NULL if loading fails
 */
plugin_t *load_plugin(const char *path);

/**
 * Starts a plug-in. The plug-in is first resolved, if necessary, and all
 * imported plug-ins are started. If the plug-in is already starting then
 * this function blocks until the plug-in has started or failed to start.
 * If the plug-in is already active then this function returns immediately.
 * If the plug-in is stopping then this function blocks until the plug-in
 * has stopped and then starts the plug-in.
 * 
 * @param plugin the plug-in to be started
 * @return CPLUFF_OK (0) on success, CPLUFF_ERROR (-1) on failure
 */
int start_plugin(plugin_t *plugin);

/**
 * Stops a plug-in. First stops any importing plug-ins that are currently
 * active. Then stops the specified plug-in. If the plug-in is already
 * stopping then this function blocks until the plug-in has stopped. If the
 * plug-in is already stopped then this function returns immediately. If the
 * plug-in is starting then this function blocks until the plug-in has
 * started (or failed to start) and then stops the plug-in (or just returns
 * if the plug-in failed to start).
 * 
 * @param plugin the plug-in to be stopped
 */
void stop_plugin(plugin_t *plugin);

/**
 * Stops all active plug-ins.
 */
void stop_all_plugins(void);

/**
 * Unloads an installed plug-in. The plug-in is first stopped if it is active.
 * Then the importing plug-ins are unloaded
 * and finally the specified plug-in is unloaded.
 * 
 * @param plugin the plug-in to be unloaded
 */
void unload_plugin(plugin_t *plugin);

/**
 * Unloads all plug-ins. This effectively stops all plug-in activity and
 * releases the resources allocated by the plug-ins.
 */
void unload_all_plugins(void);


/* Locking global data structures for exclusive access */

/**
 * Acquires exclusive access to C-Pluff data structures. Access is granted to
 * the calling thread. This function does not block if the calling thread
 * already has exclusive access. If access is acquired multiple times by the
 * same thread then it is only released after corresponding number of calls to
 * release.
 */
void acquire_cp_data(void);

/**
 * Releases exclusive access to C-Pluff data structures.
 */
void release_cp_data(void);


#ifdef __cplusplus
}
#endif /*__cplusplus*/

#endif /*_CPLUFF_H_*/
