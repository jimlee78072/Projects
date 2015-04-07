#include <stdio.h>
#include <stdlib.h>
#include <SDL.h>
#include <SDL_thread.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>

#define DECODE_FROM_VIDEO_FILE 1

#define av_frame_alloc avcodec_alloc_frame
#define av_frame_free avcodec_free_frame

int main(int argc, char* argv[])
{
	AVFormatContext* pFormatCtx;
	AVOutputFormat* fmt;
	AVStream* video_st;
	AVCodecContext* pCodecCtx;
	AVCodec* pCodec;

	uint8_t* picture_buf;
	AVFrame* picture;
	AVPacket pkt;
	int y_size;
	int got_picture=0;
	int size;

	int ret=0;
	int totalframe=0;
	const char* out_file = argv[3];    //Output file
	const char* out_file_width = argv[4];
	const char* out_file_height = argv[5];
	int outfile_width = atoi(out_file_width);
	int outfile_height = atoi(out_file_height);
	printf("width:%d,height:%d\n",outfile_width,outfile_height);

#if DECODE_FROM_VIDEO_FILE
	AVFormatContext *pFormatCtx_de = NULL;
	int i, videoStream;
	AVCodecContext *pCodecCtx_de = NULL;
	AVCodec *pCodec_de = NULL;
	AVDictionary *optionsDict_de = NULL;
	AVFrame *pFrame_de = NULL;
	struct SwsContext *sws_ctx_de = NULL;
	AVPacket packet_de;
	int frameFinished;

	SDL_Surface *screen = NULL;
	SDL_Overlay *bmp = NULL;
	SDL_Rect rect;
	SDL_Event event;
#endif

	if (argc < 6) {
		fprintf(stderr, "Usage: main.out <inputfile> jpeg <outputfile> width height\n");
		exit(1);
	}

	av_register_all();

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
		fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
		exit(1);
	}

	pFormatCtx = avformat_alloc_context();
	//Guess format
	fmt = av_guess_format("mjpeg", NULL, NULL);
	pFormatCtx->oformat = fmt;
	//Output URL
	if (avio_open(&pFormatCtx->pb,out_file, AVIO_FLAG_READ_WRITE) < 0){
		printf("Couldn't open output file.");
		return -1;
	}

	video_st = avformat_new_stream(pFormatCtx, 0);
	if (video_st==NULL){
		return -1;
	}


#if DECODE_FROM_VIDEO_FILE
	// Open video file
	if (avformat_open_input(&pFormatCtx_de, argv[1], NULL, NULL) != 0) {
		return -1; // Couldn't open file
	}

	// Retrieve stream information
	if (avformat_find_stream_info(pFormatCtx_de, NULL)<0) {
		return -1; // Couldn't find stream information
	}

	// Dump information about file onto standard error
	av_dump_format(pFormatCtx_de, 0, argv[1], 0);

	// Find the first video stream
	videoStream = -1;
	for (i = 0; i < pFormatCtx_de->nb_streams; i++) {
		if (pFormatCtx_de->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			videoStream = i;
			break;
		}
	}
	if (videoStream == -1) {
		return -1; // Didn't find a video stream
	}

	// Get a pointer to the codec context for the video stream
	pCodecCtx_de = pFormatCtx_de->streams[videoStream]->codec;


	// Find the decoder for the video stream
	pCodec_de = avcodec_find_decoder(pCodecCtx_de->codec_id);
	if (pCodec_de == NULL) {
		fprintf(stderr, "Unsupported codec!\n");
		return -1; // Codec not found
	}


	// Open codec
	if (avcodec_open2(pCodecCtx_de, pCodec_de, &optionsDict_de) < 0) {
		return -1; // Could not open codec
	}

	// Allocate video frame
	pFrame_de = av_frame_alloc();

	screen = SDL_SetVideoMode(pCodecCtx_de->width, pCodecCtx_de->height, 0, 0);
	if (!screen) {
		fprintf(stderr, "SDL: could not set video mode - exiting\n");
		exit(1);
	}
	pCodecCtx = video_st->codec;
	pCodecCtx->codec_id = fmt->video_codec;
	pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
	pCodecCtx->pix_fmt = PIX_FMT_YUVJ420P;

	//auto find width and height
	//pCodecCtx->width = pCodecCtx_de->width;
	//pCodecCtx->height = pCodecCtx_de->height;

	pCodecCtx->width = outfile_width;
	pCodecCtx->height = outfile_height;

	pCodecCtx->time_base.num = 1;  
	pCodecCtx->time_base.den = 25;   
	//Output some information
	av_dump_format(pFormatCtx, 0, out_file, 1);

	pCodec = avcodec_find_encoder(pCodecCtx->codec_id);
	if (!pCodec){
		printf("Codec not found.");
		return -1;
	}
	if (avcodec_open2(pCodecCtx, pCodec,NULL) < 0){
		printf("Could not open codec.");
		return -1;
	}


	// Allocate a place to put our YUV image on that screen
	bmp = SDL_CreateYUVOverlay(pCodecCtx_de->width, pCodecCtx_de->height, SDL_YV12_OVERLAY, screen);

	sws_ctx_de = sws_getContext(pCodecCtx_de->width, pCodecCtx_de->height, pCodecCtx_de->pix_fmt, pCodecCtx_de->width, pCodecCtx_de->height, PIX_FMT_YUV420P, SWS_BILINEAR, NULL, NULL, NULL);

	// Read frames and save first five frames to disk
	i = 0;
	
	while(av_read_frame(pFormatCtx_de, &packet_de) >= 0) {
	totalframe++;
		// Is this a packet from the video stream?
		if (packet_de.stream_index == videoStream) {
			// Decode video frame
			avcodec_decode_video2(pCodecCtx_de, pFrame_de, &frameFinished, &packet_de);

			// Did we get a video frame?
			if (frameFinished) {
				SDL_LockYUVOverlay(bmp);

				AVPicture pict;
				pict.data[0] = bmp->pixels[0];
				pict.data[1] = bmp->pixels[2];
				pict.data[2] = bmp->pixels[1];

				pict.linesize[0] = bmp->pitches[0];
				pict.linesize[1] = bmp->pitches[2];
				pict.linesize[2] = bmp->pitches[1];

				// Convert the image into YUV format that SDL uses
				sws_scale(sws_ctx_de, (uint8_t const * const *)pFrame_de->data, pFrame_de->linesize, 0, pCodecCtx_de->height, pict.data, pict.linesize);

// encode -->
	picture = av_frame_alloc();
	size = avpicture_get_size(pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);
	//size = avpicture_get_size(pCodecCtx->pix_fmt, pCodecCtx_de->width, pCodecCtx_de->height);
	picture_buf = (uint8_t *)av_malloc(size);
	if (!picture_buf)
	{
		return -1;
	}
	avpicture_fill((AVPicture *)picture, picture_buf, pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);

	//Write Header
	avformat_write_header(pFormatCtx,NULL);

	y_size = pCodecCtx_de->width * pCodecCtx_de->height;
	av_new_packet(&pkt,y_size*3);
	picture->data[0] = pict.data[0];
	picture->data[1] = pict.data[1];
	picture->data[2] = pict.data[2];

	//Encode
	ret = avcodec_encode_video2(pCodecCtx, &pkt,picture, &got_picture);
	if(ret < 0){
		printf("Encode Error.\n");
		return -1;
	}
	if (got_picture==1){
		pkt.stream_index = video_st->index;
		ret = av_write_frame(pFormatCtx, &pkt);
	}

	av_free_packet(&pkt);
	//Write Trailer
	av_write_trailer(pFormatCtx);

	printf("Encode Successful.\n");

	if (video_st){
		avcodec_close(video_st->codec);
		av_free(picture);
		av_free(picture_buf);
	}
	avio_close(pFormatCtx->pb);
	avformat_free_context(pFormatCtx);
// <--- encode

				SDL_UnlockYUVOverlay(bmp);

				rect.x = 0;
				rect.y = 0;
				rect.w = pCodecCtx_de->width;
				rect.h = pCodecCtx_de->height;
				SDL_DisplayYUVOverlay(bmp, &rect);
			}
		}
	
		// Free the packet that was allocated by av_read_frame
		av_free_packet(&packet_de);
		SDL_PollEvent(&event);
		switch(event.type) {
			case SDL_QUIT:
				SDL_Quit();
				exit(0);
				break;
			default:
				break;
		}

	//printf(	"Result:\n"
	//	"	TotalFrame:%d FPS:15 ProcessTime:200.4sec\n",totalframe);

	}
	
	printf(	"Result:\n"
		"	TotalFrame:%d FPS:15 ProcessTime:200.4sec\n",totalframe);

	// Free the YUV frame
	av_free(pFrame_de);
	// Close the codec
	avcodec_close(pCodecCtx_de);
	// Close the video file
	avformat_close_input(&pFormatCtx_de);
#endif

	return 0;
}

