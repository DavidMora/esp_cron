set(esp_cron_SRCS
 	${CMAKE_CURRENT_LIST_DIR}/cron.c
    ${CMAKE_CURRENT_LIST_DIR}/library/ccronexpr/ccronexpr.c
    ${CMAKE_CURRENT_LIST_DIR}/library/jobs/jobs.c
)

set(esp_cron_INCLUDE_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/library/ccronexpr
    ${CMAKE_CURRENT_LIST_DIR}/library/jobs
)

