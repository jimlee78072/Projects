#include <stdio.h>
#include <fcntl.h>	
#include <unistd.h>
#include <string.h>
#include <time.h>

#define FFMPEG_COMMON "ffmpeg -hide_banner -benchmark"

int stopwatch_exec(char* cmd)
{
	int ret;
	struct timeval t_start;
	struct timeval t_ended;
	double delta;
	
	gettimeofday(&t_start, NULL);
	ret = system(cmd);
	gettimeofday(&t_ended, NULL);
	
	delta = 1.0 * (t_ended.tv_sec - t_start.tv_sec)
		+ t_ended.tv_usec / 1000000.0
		- t_start.tv_usec / 1000000.0;
	printf("time elapsed: %.3f sec (include HDD write)\n", delta);
	
	return ret;
}

void get_srcname(char* dst)
{
	char filename[128] = {0};
	int f = -1;
	
	while (f == -1)
	{
		memset(filename, '\0', sizeof(filename));
		printf("\tenter video name: ");
		scanf("%s", dst);
		f = open(dst, O_RDONLY);
	}
	close(f);
}

void get_pix_format(char* format_name)
{
	int pix_format = -1;
	
	while (pix_format < 1 || pix_format > 2)
	{
		printf("\tchoose pix_format   1)rgb24\t2)yuv420p: ");
		scanf("%d", &pix_format);
	}
	switch (pix_format)
	{
		case 1:
			sprintf(format_name, "rgb24");
		break;
		case 2:
			sprintf(format_name, "yuv420p");
		break;
		default:
			printf("pix_format error (should not happen)\n");
		break;
	}
}

void get_frame_size(int* width, int* height)
{
	*width = -1;
	*height = -1;
	
	while (*width < 0)
	{
		printf("\tPlease enter frame width: ");
		scanf("%d", width);
	}
	
	while (*height < 0)
	{
		printf("\tPlease enter frame height: ");
		scanf("%d", height);
	}
}

int FF_Decode() 
{
	char ff_cmd[512] = {0};
	char srcname[128] = {0};
	char dstname[128] = {0};
	char format_name[16] = {0};
	
	printf("FF_Decode uses ffmpeg to decode an source video to a single output raw\n");
	
	printf("setup source video...\n");
	get_srcname(srcname);
	get_pix_format(format_name);
	
	printf("enter output file name (*.rgb/*.yuv): ");
	scanf("%s", dstname);
	
	sprintf(ff_cmd, "%s -i %s -c:v rawvideo -pix_fmt %s %s", FFMPEG_COMMON, srcname, format_name, dstname);
	
	return stopwatch_exec(ff_cmd);
}

int FF_Encode() 
{
	char ff_cmd[512] = {0};
	char srcname[128] = {0};
	char dstname[128] = {0};
	char format_name[16] = {0};
	int width = -1;
	int height = -1;
	
	printf("FF_Encode uses ffmpeg to encode source raw into jpg files frame by frame\n");
	
	printf("setup source raw...(*.rgb/*.yuv) \n");
	get_srcname(srcname);
	get_pix_format(format_name);
	get_frame_size(&width, &height);
	
	printf("enter output name title: ");
	scanf("%s", dstname);
	
	sprintf(ff_cmd, "%s -f rawvideo -s %dx%d -pix_fmt %s -i %s %s_encode_%%07d.jpg", 
		FFMPEG_COMMON, width, height, format_name, srcname, dstname);
	return stopwatch_exec(ff_cmd);
}

int main(int argc, char* argv[])
{
	int choice = -1, ret;
	
	while (choice > 2 || choice < 1)
	{
		printf("Please choose an operation\n1)Decode\n2)Encode\n");
		scanf("%d", &choice);
	}
	switch (choice)
	{
		case 1:
			return FF_Decode();
		break;
		case 2:
			return FF_Encode();
		break;

		default:
			return -1;
		break;
	}
}
	
	
	

