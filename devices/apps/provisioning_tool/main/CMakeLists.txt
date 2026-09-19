idf_component_register(SRCS "main.cpp")
# Supply firmware hash via -D on idf.py build
target_compile_definitions(${COMPONENT_LIB} PRIVATE
	FIRMWARE_SHA256_HEX="\"${FIRMWARE_SHA256}\"")
