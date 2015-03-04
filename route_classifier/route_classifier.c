#include <route_classifier.h>
extern int            lm_enabled_logfiles_bitmask;
extern size_t         log_ses_count[];
extern __thread log_info_t tls_log_info;

/**
 * Examine the query type, transaction state and routing hints. Find out the
 * target for query routing.
 * 
 *  @param qtype      Type of query 
 *  @param trx_active Is transacation active or not
 *  @param hint       Pointer to list of hints attached to the query buffer
 * 
 *  @return bitfield including the routing target, or the target server name 
 *          if the query would otherwise be routed to slave.
 */
route_target_t get_route_target (
        skygw_query_type_t qtype,
        bool               trx_active,
	bool           sqlvariables_in_master,
        HINT*              hint)
{
        route_target_t target = TARGET_UNDEFINED;
	/**
	 * These queries are not affected by hints
	 */
	if (QUERY_IS_TYPE(qtype, QUERY_TYPE_SESSION_WRITE) ||
		QUERY_IS_TYPE(qtype, QUERY_TYPE_PREPARE_STMT) ||
		QUERY_IS_TYPE(qtype, QUERY_TYPE_PREPARE_NAMED_STMT) ||
		/** Configured to allow writing variables to all nodes */
		(sqlvariables_in_master == false &&
			QUERY_IS_TYPE(qtype, QUERY_TYPE_GSYSVAR_WRITE)) ||
		/** enable or disable autocommit are always routed to all */
		QUERY_IS_TYPE(qtype, QUERY_TYPE_ENABLE_AUTOCOMMIT) ||
		QUERY_IS_TYPE(qtype, QUERY_TYPE_DISABLE_AUTOCOMMIT))
	{
		/** 
		 * This is problematic query because it would be routed to all
		 * backends but since this is SELECT that is not possible:
		 * 1. response set is not handled correctly in clientReply and
		 * 2. multiple results can degrade performance.
		 *
		 * Prepared statements are an exception to this since they do not
		 * actually do anything but only prepare the statement to be used.
		 * They can be safely routed to all backends since the execution
		 * is done later.
		 *
		 * With prepared statement caching the task of routing
		 * the execution of the prepared statements to the right server would be
		 * an easy one. Currently this is not supported.
		 */
		if (QUERY_IS_TYPE(qtype, QUERY_TYPE_READ) && 
		 !( QUERY_IS_TYPE(qtype, QUERY_TYPE_PREPARE_STMT) ||
		    QUERY_IS_TYPE(qtype, QUERY_TYPE_PREPARE_NAMED_STMT)))
		{
			LOGIF(LE, (skygw_log_write_flush(
				LOGFILE_ERROR,
				"Warning : The query can't be routed to all "
				"backend servers because it includes SELECT and "
				"SQL variable modifications which is not supported. ")));
			
			target = TARGET_MASTER;
		}
		target |= TARGET_ALL;
	}
	/**
	 * Hints may affect on routing of the following queries
	 */
	else if (!trx_active && 
		(QUERY_IS_TYPE(qtype, QUERY_TYPE_READ) ||	/*< any SELECT */
		QUERY_IS_TYPE(qtype, QUERY_TYPE_SHOW_TABLES) || /*< 'SHOW TABLES' */
		QUERY_IS_TYPE(qtype, QUERY_TYPE_USERVAR_READ)||	/*< read user var */
		QUERY_IS_TYPE(qtype, QUERY_TYPE_SYSVAR_READ) ||	/*< read sys var */
		QUERY_IS_TYPE(qtype, QUERY_TYPE_EXEC_STMT) ||   /*< prepared stmt exec */
		QUERY_IS_TYPE(qtype, QUERY_TYPE_GSYSVAR_READ))) /*< read global sys var */
	{
		/** First set expected targets before evaluating hints */
		if (!QUERY_IS_TYPE(qtype, QUERY_TYPE_MASTER_READ) &&
			(QUERY_IS_TYPE(qtype, QUERY_TYPE_READ) ||
			QUERY_IS_TYPE(qtype, QUERY_TYPE_SHOW_TABLES) || /*< 'SHOW TABLES' */
			/** Configured to allow reading variables from slaves */
			(sqlvariables_in_master == false && 
			(QUERY_IS_TYPE(qtype, QUERY_TYPE_USERVAR_READ) ||
			QUERY_IS_TYPE(qtype, QUERY_TYPE_SYSVAR_READ) ||
			QUERY_IS_TYPE(qtype, QUERY_TYPE_GSYSVAR_READ)))))
		{
			target = TARGET_SLAVE;
		}
		else if (QUERY_IS_TYPE(qtype, QUERY_TYPE_MASTER_READ) ||
			QUERY_IS_TYPE(qtype, QUERY_TYPE_EXEC_STMT)	||
			/** Configured not to allow reading variables from slaves */
			(sqlvariables_in_master == true && 
			(QUERY_IS_TYPE(qtype, QUERY_TYPE_USERVAR_READ)	||
			QUERY_IS_TYPE(qtype, QUERY_TYPE_SYSVAR_READ))))
		{
			target = TARGET_MASTER;
		}
		/** process routing hints */
		while (hint != NULL)
		{
			if (hint->type == HINT_ROUTE_TO_MASTER)
			{
				target = TARGET_MASTER; /*< override */
				LOGIF(LD, (skygw_log_write(
					LOGFILE_DEBUG,
					"%lu [get_route_target] Hint: route to master.",
					pthread_self())));
				break;
			}
			else if (hint->type == HINT_ROUTE_TO_NAMED_SERVER)
			{
				/** 
				 * Searching for a named server. If it can't be
				 * found, the oroginal target is chosen.
				 */
				target |= TARGET_NAMED_SERVER;
				LOGIF(LD, (skygw_log_write(
					LOGFILE_DEBUG,
					"%lu [get_route_target] Hint: route to "
					"named server : ",
					pthread_self())));
			}
			else if (hint->type == HINT_ROUTE_TO_UPTODATE_SERVER)
			{
				/** not implemented */
			}
			else if (hint->type == HINT_ROUTE_TO_ALL)
			{
				/** not implemented */
			}
			else if (hint->type == HINT_PARAMETER)
			{
				if (strncasecmp(
					(char *)hint->data, 
						"max_slave_replication_lag", 
						strlen("max_slave_replication_lag")) == 0)
				{
					target |= TARGET_RLAG_MAX;
				}
				else
				{
					LOGIF(LT, (skygw_log_write(
						LOGFILE_TRACE,
						"Error : Unknown hint parameter "
						"'%s' when 'max_slave_replication_lag' "
						"was expected.",
						(char *)hint->data)));
					LOGIF(LE, (skygw_log_write_flush(
						LOGFILE_ERROR,
						"Error : Unknown hint parameter "
						"'%s' when 'max_slave_replication_lag' "
						"was expected.",
						(char *)hint->data)));                                        
				}
			}
			else if (hint->type == HINT_ROUTE_TO_SLAVE)
			{
				target = TARGET_SLAVE;
				LOGIF(LD, (skygw_log_write(
					LOGFILE_DEBUG,
					"%lu [get_route_target] Hint: route to "
					"slave.",
					pthread_self())));                                
			}
			hint = hint->next;
		} /*< while (hint != NULL) */
		/** If nothing matches then choose the master */
		if ((target & (TARGET_ALL|TARGET_SLAVE|TARGET_MASTER)) == 0)
		{
			target = TARGET_MASTER;
		}
	}
	
	return target;
}
