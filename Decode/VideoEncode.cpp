#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavutil/time.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
}

using namespace std;

const char* inFileName = "./yuv420p_1280x720.yuv";
const char* outFileName = "./encode_yuv420p_1280x720.h264";


int encode(AVCodecContext* codecContent, AVPacket* packet, AVFrame* frame, FILE* outFile)
{
	//����
	int ret = avcodec_send_frame(codecContent, frame);
	if (ret < 0)
	{
		fprintf(stderr, "Error sending a frame for encoding\n");
		return -1;
	}

	while (ret == 0)
	{

		ret = avcodec_receive_packet(codecContent, packet);

		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
			return 0;
		}
		else if (ret < 0) {
			fprintf(stderr, "Error encoding video frame\n");
			return -1;
		}

		if (ret == 0)
		{
			fwrite(packet->data, 1, packet->size, outFile);
		}

	}
}



int main(int argc, char* argv[])
{

	int ret = 0;

	AVCodec* codec = nullptr;
	AVCodecContext* codecContent = nullptr;
	AVPacket* packet = nullptr;
	AVFrame* frame = nullptr;

	FILE* inFile = nullptr;
	FILE* outFile = nullptr;



	//����ָ��������
	codec = avcodec_find_encoder(AV_CODEC_ID_H264);
	if (codec == nullptr)
	{
		printf("could not find h264 encoder!");
		return -1;
	}

	//���������������
	codecContent = avcodec_alloc_context3(codec);
	if (codecContent == nullptr)
	{
		printf("could not alloc h264 content!");
		return -1;
	}

	//�������
	codecContent->width = 1280;
	codecContent->height = 720;
	codecContent->time_base = AVRational{ 1, 25 };


	codecContent->pix_fmt = AV_PIX_FMT_YUV420P;
	codecContent->gop_size = 60; //�ؼ�֡�����Ĭ��250
	codecContent->framerate = AVRational{ 25, 1 };

	//��ʼ��������������
	ret = avcodec_open2(codecContent, codec, NULL);
	if (ret < 0) {
		fprintf(stderr, "Could not open codec: %d\n", ret);
		exit(1);
	}


	packet = av_packet_alloc();
	if (packet == nullptr)
	{
		printf("alloc packet error");
		return -1;
	}

	frame = av_frame_alloc();
	if (packet == nullptr)
	{
		printf("alloc frame error");
		return -1;
	}

	//�������
	frame->width = codecContent->width;
	frame->height = codecContent->height;
	frame->format = codecContent->pix_fmt;

	//���øò�����������Ƶȫ��I֡������gop_size
	//frame->pict_type = AV_PICTURE_TYPE_I;


	//������Ƶ���ݴ洢�ռ�
	ret = av_frame_get_buffer(frame, 0);
	if (ret)
	{
		printf("alloc frame buffer error!");
		return -1;
	}

	inFile = fopen(inFileName, "rb");
	if (inFile == nullptr)
	{
		printf("error to open file: %s\n", inFileName);
		return -1;
	}

	outFile = fopen(outFileName, "wb");
	if (inFile == nullptr)
	{
		printf("error to open file: %s\n", outFileName);
		return -1;
	}

	//֡����¼
	int framecount = 0;
	
	frame->pts = 0;

	int start_time = av_gettime() / 1000; //���뼶


	while (!feof(inFile))
	{
		ret = av_frame_is_writable(frame);
		if (ret < 0)
		{
			ret = av_frame_make_writable(frame);
		}


		fread(frame->data[0], 1, frame->width * frame->height, inFile); //y
		fread(frame->data[1], 1, frame->width * frame->height / 4, inFile); //u
		fread(frame->data[2], 1, frame->width * frame->height / 4, inFile);  //v

		printf("encode frame num: %d\n", ++framecount);


		frame->pts += 1000 / (codecContent->time_base.den / codecContent->time_base.num);
		encode(codecContent, packet, frame, outFile);

	}

	encode(codecContent, packet, nullptr, outFile);
	printf("encode time cost: %d ms\n ", av_gettime() / 1000 - start_time);

	av_packet_free(&packet);
	av_frame_free(&frame);
	avcodec_free_context(&codecContent);
	fclose(inFile);
	fclose(outFile);


	return 0;
}