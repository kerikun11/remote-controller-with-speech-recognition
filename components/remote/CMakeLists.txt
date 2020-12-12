set(COMPONENT_SRCDIRS
    IRremoteESP8266/src
    )

set(COMPONENT_ADD_INCLUDEDIRS
    IRremoteESP8266/src
    IRremoteESP8266/test
    )

register_component()

target_compile_definitions(
    ${COMPONENT_TARGET}
    PUBLIC UNIT_TEST
)
