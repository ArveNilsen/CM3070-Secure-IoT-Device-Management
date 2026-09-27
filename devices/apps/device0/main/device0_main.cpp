#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


extern "C" void app_main()
{
	ESP_LOGI("app_main", "reached");
	while (true) {
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}
