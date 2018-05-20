/*
    power_app_stats is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    SophieCompiler is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with power_app_stats.  If not, see <http://www.gnu.org/licenses/>.
    
    @author José Carlos García <hola@josecarlos.me>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#define PAS_SAMPLES 60

int pas_read_charge_now() {
    FILE * fp;
    char charge_now[100];
    int charge_now_integer;

    fp = fopen("/sys/class/power_supply/BAT1/charge_now", "r");
    fgets(charge_now, 100, fp);
    charge_now_integer = atoi(charge_now);

    fclose(fp);

    return charge_now_integer;
}

int pas_is_discharging() {
    FILE * fp;
    char status;
    
    fp = fopen("/sys/class/power_supply/BAT1/status", "r");
    
    fread(&status, 1, 1, fp);
    fclose(fp);

    return status == 68;
}

void * pas_measure_app(void * app_sample_arg) {
    int *app_samples, i;
    app_samples = (int*)app_sample_arg;

    for (i = 0; i < PAS_SAMPLES; i++) {
        app_samples[i] = pas_read_charge_now();
        sleep(1);
    }
}
int main(int argc, char** args) {
    int i, k, pas_samples[PAS_SAMPLES], app_samples[PAS_SAMPLES], arg_len;
    float discharging_speed_mean, discharging_speed_mean_app;
    char * app_command;
    pthread_t monitor;

    if (argc == 1) {
        printf("ERROR! You should specify an application for running!\n");
        return 1;
    }

    if (!pas_is_discharging()) {
        printf("ERROR! Your system should be discharging for using this tool!\n");
        return 1;
    }
    
    app_command = malloc(1);
    app_command[0] = 0;
    
    for (i = 1; i < argc; i++) {
        arg_len = strlen(args[i]);
        app_command = realloc(app_command, k + arg_len + 2);
        strcat(app_command, args[i]);
        strcat(app_command, " ");
        
        k = k + arg_len;
    }
    
    printf("Calculating the average of discharging of your computer\n");
    
    for (i = 0; i < PAS_SAMPLES; i++) {
        pas_samples[i] = pas_read_charge_now();
        app_samples[i] = -1;
        sleep(1);
    }

    for (i = 1; i < PAS_SAMPLES; i++) {
        discharging_speed_mean = (1.0 * discharging_speed_mean * (i - 1) + pas_samples[i] - pas_samples[i - 1]) / (i * 1.0);
    }

    printf("Lauching your application\n");
    pthread_create(&monitor, NULL, pas_measure_app, app_samples);
    system(app_command);
    pthread_cancel(monitor);

    for (i = 1; i < PAS_SAMPLES; i++) {
        if (app_samples[i] == -1) {
            break;
        }
        discharging_speed_mean_app = (1.0 * discharging_speed_mean_app * (i - 1) + app_samples[i] - app_samples[i - 1]) / (i * 1.0);
    }

    printf("Discharging speed without app: %f µA/s\n", discharging_speed_mean);
    printf("Discharging speed with app: %f µA/s\n", discharging_speed_mean_app);
    printf("Difference between two speeds: %f µA/s\n", discharging_speed_mean - discharging_speed_mean_app);

    free(app_command);
}
