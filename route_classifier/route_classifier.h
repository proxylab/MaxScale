#ifndef ROUTECLASSIFIER_HG
#define ROUTECLASSIFIER_HG

#include <query_classifier.h>
#include <hint.h>
#include <log_manager.h>

typedef enum {
	TARGET_UNDEFINED    = 0x00,
        TARGET_MASTER       = 0x01,
        TARGET_SLAVE        = 0x02,
        TARGET_NAMED_SERVER = 0x04,
        TARGET_ALL          = 0x08,
        TARGET_RLAG_MAX     = 0x10
} route_target_t;

route_target_t get_route_target (
        skygw_query_type_t qtype,
        bool               trx_active,
        bool           use_sql_variables_in,
        HINT*              hint);

#endif
