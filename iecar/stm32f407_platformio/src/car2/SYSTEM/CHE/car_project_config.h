#ifndef CAR_PROJECT_CONFIG_H__
#define CAR_PROJECT_CONFIG_H__

/*
 * Project-wide runtime configuration for the 2021-F drug car project.
 * Shared defaults live in ../../COMMON/CHE/car_project_config_shared.h.
 * Only board-specific overrides should remain in this wrapper.
 */

#define CAR_ROLE_ID                  2u
#define CAR_ALLOWED_TASK_MASK        (CAR_TASK_MASK_TASK2 | CAR_TASK_MASK_TASK3)
#define CAR_LEGACY_TASK_MASK         0u
#define CAR_OPENMV_BOARD_HOLD_MS     300u
#define CAR_LINK_USART1_DEBUG_RX_ENABLED 1u

#include "../../../COMMON/CHE/car_project_config_shared.h"

#endif /* CAR_PROJECT_CONFIG_H__ */
