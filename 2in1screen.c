// gcc -O2 -o 2in1screen 2in1screen.c

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define DATA_SIZE 256
#define TOUCHSCREEN "pointer:Goodix Capacitive TouchScreen"
char basedir[DATA_SIZE];
char *basedir_end = NULL;
char content[DATA_SIZE];
char command[DATA_SIZE*4];

char *ROT[]   = {"normal","inverted","left","right"};

double accel_y = 0.0,
	   accel_x = 0.0,
	   accel_g = 7.0;

int rotation_persist;

int current_state = -1;

int next_current_state = -1;

int rotation_changed(){
	int state = current_state;

	if(accel_y < -accel_g) state = 3;
	else if(accel_y > accel_g) state = 2;
	else if(accel_x > accel_g) state = 1;
	else if(accel_x < -accel_g) state = 0;

	if(current_state != state){
		if (next_current_state != state) {
			next_current_state = state;
			rotation_persist = 1;
		} else {
			rotation_persist++;
			if (rotation_persist > 2) {
				current_state = next_current_state;
				next_current_state = -1;
				fprintf(stderr, "Switch to: %i\n", state);
				return 1;
			}
		}
	}

	return 0;
}

FILE* bdopen(char const *fname, char leave_open){
	*basedir_end = '/';
	strcpy(basedir_end+1, fname);
	FILE *fin = fopen(basedir, "r");
	setvbuf(fin, NULL, _IONBF, 0);
	fgets(content, DATA_SIZE, fin);
	*basedir_end = '\0';
	if(leave_open==0){
		fclose(fin);
		return NULL;
	}
	else return fin;
}

void rotate_screen(){
	fprintf(stderr, "Orientation %s\n", ROT[current_state]);
	sprintf(command, "xrandr -o %s", ROT[current_state]);
	fprintf(stderr, "%s\n", command);
	system(command);
	sprintf(command, "xinput --map-to-output \"%s\" eDP1", TOUCHSCREEN);
	fprintf(stderr, "%s\n", command);
	system(command);
}

int main(int argc, char const *argv[]) {
	FILE *pf = popen("ls /sys/bus/iio/devices/iio:device*/in_accel*", "r");
	if(!pf){
		fprintf(stderr, "IO Error.\n");
		return 2;
	}

	if(fgets(basedir, DATA_SIZE , pf)!=NULL){
		basedir_end = strrchr(basedir, '/');
		if(basedir_end) *basedir_end = '\0';
		fprintf(stderr, "Accelerometer: %s\n", basedir);
	}
	else{
		fprintf(stderr, "Unable to find any accelerometer.\n");
		return 1;
	}
	pclose(pf);

	bdopen("in_accel_scale", 0);
	double scale = atof(content);
	fprintf(stderr, "scale = %lf\n", scale);

	FILE *dev_accel_y = bdopen("in_accel_y_raw", 1);
	FILE *dev_accel_x = bdopen("in_accel_x_raw", 1);

	while(1){
		fseek(dev_accel_y, 0, SEEK_SET);
		fgets(content, DATA_SIZE, dev_accel_y);
		accel_y = atof(content) * scale;
		fprintf(stderr, "y = %lf\n", accel_y);
		fseek(dev_accel_x, 0, SEEK_SET);
		fgets(content, DATA_SIZE, dev_accel_x);
		fprintf(stderr, "x = %lf\n", atof(content));
		accel_x = atof(content) * scale;
		fprintf(stderr, "x_scaled = %lf\n", accel_x);
		if(rotation_changed())
			rotate_screen();
		sleep(2);
	}
	
	return 0;
}
