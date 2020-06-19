#include <stdio.h>
#include <esp/uart.h>
#include <esp8266.h>
#include <FreeRTOS.h>
#include <task.h>

#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#include <wifi_config.h>
#include <pwm.h>

#ifndef WHITE_PIN
#error WHITE_PIN is not specified
#endif

bool led_on = false;
int led_brightness = 0;
int target = 0;

void led_write(bool on) {
    
    if(on){
        printf("target %i",target);
        // lower
        if (led_brightness < target){
            
            while (led_brightness != target){
                led_brightness += 2;

                if (led_brightness > target){
                    led_brightness = target;
                    pwm_set_duty (UINT16_MAX * (led_brightness/100.00));
                    //target = 0;
                    break;
                }

                pwm_set_duty(UINT16_MAX * (led_brightness/100.00));
                vTaskDelay(30 / portTICK_PERIOD_MS);
            }
        }
        // higher 
        else{
            while (led_brightness != target){
                  led_brightness -= 2;

                if (led_brightness < target){
                    led_brightness = target;
                    pwm_set_duty (UINT16_MAX * (led_brightness/100.00));
                   // target = 0;
                    break;
                }

                pwm_set_duty(UINT16_MAX * (led_brightness/100.00));
                vTaskDelay(30 / portTICK_PERIOD_MS);

            }
        }

        //target = 0;

    }else {
        while (led_brightness != 0){

            pwm_set_duty(UINT16_MAX * (led_brightness/100.00));
            led_brightness -=  2;

            if (led_brightness < 0){
                led_brightness = 0;
                pwm_set_duty(0);
                break;
            }

            pwm_set_duty(UINT16_MAX * (led_brightness/100.00));
            vTaskDelay(30 / portTICK_PERIOD_MS);
        }
    }
}

void init_pwm(){
     uint8_t pins[1];

    printf("pwm_init(1, [14])\n");
    pins[0] = WHITE_PIN;
    pwm_init(1, pins, false);

    printf("pwm_set_freq(1000)     # 1 kHz\n");
    pwm_set_freq(1000);

    printf("pwm_start()\n");
    pwm_start();
}

void led_identify_task(void *_args) {
    for (int i=0; i<3; i++) {
        for (int j=0; j<2; j++) {
            pwm_set_duty(UINT16_MAX);
            vTaskDelay(100 / portTICK_PERIOD_MS);
            pwm_set_duty(0);
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }

        vTaskDelay(250 / portTICK_PERIOD_MS);
    }

    target = 100;
    led_write(led_on);

    vTaskDelete(NULL);
}

void led_identify(homekit_value_t _value) {
    printf("LED identify\n");
    xTaskCreate(led_identify_task, "LED identify", 128, NULL, 2, NULL);
}

homekit_value_t led_on_get() {
    return HOMEKIT_BOOL(led_on);
}

void led_on_set(homekit_value_t value) {
    if (value.format != homekit_format_bool) {
        printf("Invalid value format: %d\n", value.format);
        return;
    }

    led_on = value.bool_value;
    led_write(led_on);
}

homekit_value_t led_brightness_get() {
    return HOMEKIT_INT(led_brightness);
}
void led_brightness_set(homekit_value_t value) {
   target = value.int_value;
}


homekit_accessory_t *accessories[] = {
    HOMEKIT_ACCESSORY(.id=1, .category=homekit_accessory_category_lightbulb, .services=(homekit_service_t*[]){
        HOMEKIT_SERVICE(ACCESSORY_INFORMATION, .characteristics=(homekit_characteristic_t*[]){
            HOMEKIT_CHARACTERISTIC(NAME, "Light strip"),
            HOMEKIT_CHARACTERISTIC(MANUFACTURER, "Michael's Software"),
            HOMEKIT_CHARACTERISTIC(SERIAL_NUMBER, "0x10"),
            HOMEKIT_CHARACTERISTIC(MODEL, "Make It Light"),
            HOMEKIT_CHARACTERISTIC(FIRMWARE_REVISION, "1.0"),
            HOMEKIT_CHARACTERISTIC(IDENTIFY, led_identify),
            NULL
        }),
       HOMEKIT_SERVICE(LIGHTBULB, .primary = true, .characteristics = (homekit_characteristic_t*[]) {
            HOMEKIT_CHARACTERISTIC(NAME, "Light strip"),
            HOMEKIT_CHARACTERISTIC(
                ON, true,
                .getter = led_on_get,
                .setter = led_on_set
            ),
            HOMEKIT_CHARACTERISTIC(
                BRIGHTNESS, 100,
                .getter = led_brightness_get,
                .setter = led_brightness_set
            ),
            NULL
        }),
        NULL
    }),
    NULL
};

homekit_server_config_t config = {
    .accessories = accessories,
    .password = "248-12-524",
    .setupId="1A20"
};


void on_wifi_event(wifi_config_event_t event) {
    if (event == WIFI_CONFIG_CONNECTED) {
        printf("Connected to WiFi\n");
    } else if (event == WIFI_CONFIG_DISCONNECTED) {
        printf("Disconnected from WiFi\n");
    }
}

void user_init(void) {
    uart_set_baud(0, 115200);

    wifi_config_init2("Apple Home", NULL, on_wifi_event);
    init_pwm();
    homekit_server_init(&config);
}
