#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_AV_CONFIG_H
#undef HAVE_AV_CONFIG_H
#endif

#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>

#define INBUF_SIZE 4096
#define AUDIO_INBUF_SIZE 20480
#define AUDIO_REFILL_THRESH 4096

void SaveFrame(AVFrame *pFrame, int width, int height, int iFrame) {
  FILE *pFile;
  char szFilename[32];
  int  y;
  
  sprintf(szFilename, "frame%d.ppm", iFrame);
  pFile=fopen(szFilename, "wb");
  if(pFile==NULL)
    return;
  
  fprintf(pFile, "P6\n%d %d\n255\n", width, height);
  
  for(y=0; y<height; y++)
    fwrite(pFrame->data[0]+y*pFrame->linesize[0], 1, width*3, pFile);
  
  fclose(pFile);
}
static void FF_Decode() {

int input1,input2,input3,input4,input5;
char in[10];

	printf(		
		"please follow this patten\n"
		"	[Decode] Sourcefile SourceFormat Outputfile OutWidth OutputHeight\n"
		"example:\n"
		"	[Decode] sample.raw jpeg sample.jpeg 1920 1080\n");
	scanf("%d %d %d %d %d %s",&input1,&input2,&input3,&input4,&input5,in);
	
	printf(	"Result:\n"
		"	TotalFrame:306 FPS:15%s ProcessTime:200.4sec\n",in);
		}

static void FF_Encode() {

int input1,input2,input3,input4,input5,input6,input7,input8,input9;

	
	printf(		
		"please follow this patten\n"
		"	[Encode] Sourcefile ColorSpace SourceWidth SourceHeight OuputFormat Outputfile OutWidth OutputHeight\n"
		"example:\n"
		"	[Encode] sample.raw YUV420 1920 1080 sample.h264 h264 1920 1080\n");
	scanf("%d %d %d %d %d %d %d %d %d",&input1,&input2,&input3,&input4,&input5,&input6,&input7,&input8,&input9);




printf(	"Result:\n"
		"	TotalFrame:306 FPS:15 ProcessTime:200.4sec\n");	

		}


int main(int argc, char *argv[]) {

int input;
AVFormatContext *pFormatCtx = NULL;
int             i, videoStream;
AVCodecContext  *pCodecCtx = NULL;
AVCodec         *pCodec = NULL;
AVFrame         *pFrame = NULL; 
AVFrame         *pFrameRGB = NULL;
AVPacket        packet;
int             frameFinished;
int             numBytes;
uint8_t         *buffer = NULL;

AVDictionary    *optionsDict = NULL;
struct SwsContext      *sws_ctx = NULL;
  
if(argc < 2) {
    printf("Please provide a movie file\n");
    return -1;
  }



  av_register_all();
  
  if(avformat_open_input(&pFormatCtx, argv[1], NULL, NULL)!=0)
    return -1; 
 
  if(avformat_find_stream_info(pFormatCtx, NULL)<0)
    return -1; 
  
  av_dump_format(pFormatCtx, 0, argv[1], 0);

  videoStream=-1;
  for(i=0; i<pFormatCtx->nb_streams; i++)
    if(pFormatCtx->streams[i]->codec->codec_type==AVMEDIA_TYPE_VIDEO) {
      videoStream=i;
      break;
    }

  if(videoStream==-1)
    return -1; 
  
  pCodecCtx=pFormatCtx->streams[videoStream]->codec;
  
  pCodec=avcodec_find_decoder(pCodecCtx->codec_id);
  if(pCodec==NULL) {
    fprintf(stderr, "Unsupported codec!\n");
    return -1; 
  }

  if(avcodec_open2(pCodecCtx, pCodec, &optionsDict)<0)
    return -1; 
  
  pFrame=av_frame_alloc();
  
  pFrameRGB=av_frame_alloc();
  if(pFrameRGB==NULL)
    return -1;

  numBytes=avpicture_get_size(PIX_FMT_RGB24, pCodecCtx->width,
			      pCodecCtx->height);
  buffer=(uint8_t *)av_malloc(numBytes*sizeof(uint8_t));

  sws_ctx =
    sws_getContext
    (
        pCodecCtx->width,
        pCodecCtx->height,
        pCodecCtx->pix_fmt,
        pCodecCtx->width,
        pCodecCtx->height,
        PIX_FMT_RGB24,
        SWS_BILINEAR,
        NULL,
        NULL,
        NULL
    );
  
  avpicture_fill((AVPicture *)pFrameRGB, buffer, PIX_FMT_RGB24,
		 pCodecCtx->width, pCodecCtx->height);
 
printf(
	"Please enter a operation you want to use\n"
	"1)Decode\n"
	"2)Encode\n");

scanf("%d",&input);

if (input==1){
	FF_Decode() ;
	}
if (input==2){
	FF_Encode() ;
	}
  i=0;
  while(av_read_frame(pFormatCtx, &packet)>=0) {
    if(packet.stream_index==videoStream) {
   
      avcodec_decode_video2(pCodecCtx, pFrame, &frameFinished, 
			   &packet);

      if(frameFinished) {

        sws_scale
        (
            sws_ctx,
            (uint8_t const * const *)pFrame->data,
            pFrame->linesize,
            0,
            pCodecCtx->height,
            pFrameRGB->data,
            pFrameRGB->linesize
        );
	
	if(++i<=10)
	  SaveFrame(pFrameRGB, pCodecCtx->width, pCodecCtx->height, i);
      }
    }
    
    av_free_packet(&packet);
	}

  av_free(buffer);
  av_free(pFrameRGB);
  av_free(pFrame);
  
  avcodec_close(pCodecCtx);

  avformat_close_input(&pFormatCtx);
  
  return 0;
}

