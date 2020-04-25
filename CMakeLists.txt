set(COMPONENT_ADD_INCLUDEDIRS "." "include" "library/ccronexpr" "library/jobs")
set(COMPONENT_SRCDIRS "." "library/ccronexpr" "library/jobs")

register_component()

target_compile_definitions(${COMPONENT_TARGET} PUBLIC -D CRON_USE_LOCAL_TIME)
