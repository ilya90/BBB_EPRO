#include <stdio.h>

#define LED3_BRIGHTNESS_FILE	"//sys/class/leds/beaglebone\:green\:usr3/brightness"
#define LED_ON 					"1"
#define LED_OFF					"0"

int main (){
	int i;
	FILE *f = NULL;
	printf("LED-3 blinking program started\n");
	f = fopen(LED3_BRIGHTNESS_FILE, "w");
	if (f == NULL)
	{
		printf("Error opening file!\n");
		exit(1);
	}

	for (i=0; i<20; i++){
		f = fopen(LED3_BRIGHTNESS_FILE, "w");
		if (f == NULL)
		{
			printf("Error opening file!\n");
			exit(1);
		}
		fprintf(f, LED_ON);
		sleep(1);
		fclose(f);
		f = fopen(LED3_BRIGHTNESS_FILE, "w");
		if (f == NULL)
		{
			printf("Error opening file!\n");
			exit(1);
		}
		fprintf(f, LED_OFF);
		sleep(1);
		fclose(f);
	}
	
	printf("LED-3 program execuution finished\n");

	return 0;
}

/*	echo 1 > /sys/class/leds/beaglebone\:green\:usr3/brightness		*/
/*	arm-linux-gnueabi-gcc led_blink.c -o led_blink					*/
/*	scp led_blink root@192.168.7.2:/home/root/C-programs/			*/
/*	./led_blink 													*/
