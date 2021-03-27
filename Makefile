all: schedule

schedule: schedule.c
	gcc -Wall -o schedule schedule.c -lpthread -lm
	
clean:
	rm -fr schedule *~ *.o core*
