set(COMPONENT_SRCS
    app_main.cpp
    )

set(COMPONENT_ADD_INCLUDEDIRS
    .
    )

set(COMPONENT_REQUIRES
    nvs_flash
    protocol_examples_common
    esp_http_client
    esp-sr
    spiffs
    driver
    remote
    )

register_component()
