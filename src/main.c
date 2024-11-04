#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>

#include "network.h"
#include "utils.h"
#include "weather.h"

#define WEATHER_API_URL "https://api.open-meteo.com/v1/forecast?latitude=30.4394&longitude=-97.62&hourly=temperature_2m,apparent_temperature,precipitation_probability,precipitation,cloud_cover"
#define WEATHER_API_URL_SHORT "https://api.open-meteo.com/v1/forecast?latitude=30.4394&longitude=-97.62&hourly=temperature_2m"
#define WEATHER_API_URL_SHORT_2 "api.open-meteo.com/v1/forecast?latitude=30.4394&longitude=-97.62&hourly=temperature_2m"
#define WEATHER_API_URL_HOSTNAME "api.open-meteo.com"
#define WEATHER_API_URL_PATH_SHORT "/v1/forecast?latitude=30.4394&longitude=-97.62&hourly=temperature_2m"
#define WEATHER_API_URL_PATH_LONG "/v1/forecast?latitude=30.4394&longitude=-97.62&hourly=temperature_2m,apparent_temperature,precipitation_probability,precipitation,cloud_cover"
#define WEATHER_API_URL_PATH_LONGER "/v1/forecast?latitude=30.4394&longitude=-97.62&hourly=temperature_2m,apparent_temperature,precipitation_probability,precipitation,cloud_cover&temperature_unit=fahrenheit&wind_speed_unit=mph&precipitation_unit=inch&timeformat=unixtime"
#define WEATHER_API_URL_PATH_LONGEST "/v1/forecast?latitude=30.4394&longitude=-97.62&current=temperature_2m,relative_humidity_2m,apparent_temperature,is_day,precipitation,rain,showers,snowfall,weather_code,cloud_cover,pressure_msl,surface_pressure,wind_speed_10m,wind_direction_10m,wind_gusts_10m&hourly=temperature_2m,relative_humidity_2m,dew_point_2m,apparent_temperature,precipitation_probability,precipitation,rain,showers,snowfall,snow_depth,weather_code,pressure_msl,surface_pressure,cloud_cover,cloud_cover_low,cloud_cover_mid,cloud_cover_high,visibility,evapotranspiration,et0_fao_evapotranspiration,vapour_pressure_deficit,wind_speed_10m,wind_speed_80m,wind_speed_120m,wind_speed_180m,wind_direction_10m,wind_direction_80m,wind_direction_120m,wind_direction_180m,wind_gusts_10m,temperature_80m,temperature_120m,temperature_180m,soil_temperature_0cm,soil_temperature_6cm,soil_temperature_18cm,soil_temperature_54cm,soil_moisture_0_to_1cm,soil_moisture_1_to_3cm,soil_moisture_3_to_9cm,soil_moisture_9_to_27cm,soil_moisture_27_to_81cm&daily=weather_code,temperature_2m_max,temperature_2m_min,apparent_temperature_max,apparent_temperature_min,sunrise,sunset,daylight_duration,sunshine_duration,uv_index_max,uv_index_clear_sky_max,precipitation_sum,rain_sum,showers_sum,snowfall_sum,precipitation_hours,precipitation_probability_max,wind_speed_10m_max,wind_gusts_10m_max,wind_direction_10m_dominant,shortwave_radiation_sum,et0_fao_evapotranspiration&temperature_unit=fahrenheit&wind_speed_unit=mph&precipitation_unit=inch&timeformat=unixtime&timezone=auto&forecast_days=14"
#define WEATHER_API_URL_PATH_ACTUAL "/v1/forecast?latitude=30.4394&longitude=-97.62&temperature_unit=fahrenheit&current=temperature_2m"

void disp_t(time_t, weather_data*);

int main(int argc, char *argv[])
{
    /* open files */

    int response_fd = open("response.txt", O_RDWR | O_TRUNC | O_CREAT, S_IWUSR + S_IRUSR);
    int body_fd     = open("body.txt",     O_RDWR | O_TRUNC | O_CREAT, S_IWUSR + S_IRUSR);
    int err;
    
    /* get api response */

    err = network_response(WEATHER_API_URL_HOSTNAME, WEATHER_API_URL_PATH_ACTUAL, response_fd);
    if (err != NETWORK_EXIT_SUCCESS)
    {
        printf("network response error occurred\nerror number: %d\n", err);
        close(response_fd);
        close(body_fd);
        return 1;
    }

    /* change out file descriptors for FILE pointers */
    /* kinda stupid, probably rework this sometime lol */

    close(response_fd);
    close(body_fd);

    FILE *response_file = fopen("response.txt", "r");
    FILE *body_file = fopen("body.txt", "r+");

    /* put response body into a separate file */

    err = response_body(response_file, body_file, 1);
    if (err != NETWORK_EXIT_SUCCESS)
    {
        printf("response body error occurred\nerror number: %d\n", err);
        close(response_fd);
        close(body_fd);
        return 2;
    }

    /* load weather data */

    cJSON *json_root = NULL;

    weather_data *_data = load_file_weather(body_file, json_root);

    weather_data *data = NULL;
    create_weather_data(&data, _data);

    cJSON_Delete(json_root);

    /* cleanup */

    // destroy_weather_data(data);

    fclose(response_file);
    fclose(body_file);

    //return 0; /* :trollface: */

    time_t t_curr;
    time_t t_last;
    time(&t_curr);
    time(&t_last);

    struct termios orig_conf = get_terminal_conf();
    raw_mode_enable();
    cursor_hide();

    while (1)
    {
        time(&t_curr);
        time_t t_delta = t_curr - t_last;
        if (t_delta)
            disp_t(t_curr, data);
        if (was_kbhit())
            if (getchar())
                break;
        t_last = t_curr;
    }

    reset_screen();
    raw_mode_disable();
    set_terminal_conf(orig_conf);
    cursor_show();

    return 0;
}

void disp_t(time_t t_curr, weather_data *weather)
{
    struct tm *t = localtime(&t_curr);
    reset_screen();
    printf("time\n");
    printf("%02d:%02d:%02d\n",
           t->tm_hour,
           t->tm_min,
           t->tm_sec);
    printf("weather\n");
    printf("temperature: %.2f\xc2\xb0""F\n", *weather->current->temperature_2m);
}
