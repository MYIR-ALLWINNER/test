/*
 * \file        cameratest.c
 * \brief
 *
 * \version     1.0.0
 * \date        2012年06月26日
 * \author      James Deng <csjamesdeng@allwinnertech.com>
 *
 * Copyright (c) 2012 Allwinner Technology. All Rights Reserved.
 *
 */

#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>

#include "sun8iw6p_display.h"
#include "dragonboard_inc.h"
#include "videodev2_vin.h"

#define ALIGN_4K(x) (((x) + (4095)) & ~(4095))
#define ALIGN_32B(x) (((x) + (31)) & ~(31))
#define ALIGN_16B(x) (((x) + (15)) & ~(15))
/* #define DEBUG_CAMERA */

struct test_layer_info {
	int screen_id;
	int layer_id;
	int mem_id;
	disp_layer_config layer_config;
	int addr_map;
	int width,height;/* screen size */
	int dispfh;/* device node handle */
	int fh;/* picture resource file handle */
	int mem;
	int clear;/* is clear layer */
	char filename[32];
};

struct test_layer_info test_info;

struct buffer {
	void   *start;
	size_t length;
};

static int csi_format;
static int fps;
static int req_frame_num;

static struct buffer *buffers = NULL;
static int nbuffers = 0;

static disp_rectsz input_size;
static disp_rectsz display_size;

static int stop_flag;

static int dev_no;
static int dev_cnt;
static int csi_cnt;
static int camera_layer =0;
static int screen_id = 0;
static pthread_t video_tid;
static int sensor_type = 0;
unsigned int nplanes;

int disp_set_addr(int width, int height, unsigned long *addr);

static int getSensorType(int fd) {
	struct v4l2_control ctrl;
	struct v4l2_queryctrl qc_ctrl;

	if (fd == NULL) {
		return 0xFF000000;
	}

	ctrl.id = V4L2_CID_SENSOR_TYPE;
	qc_ctrl.id = V4L2_CID_SENSOR_TYPE;

	if (-1 == ioctl (fd, VIDIOC_QUERYCTRL, &qc_ctrl)) {
		db_error("query sensor type ctrl failed");
		return -1;
	}
	if (-1 == ioctl(fd, VIDIOC_G_CTRL, &ctrl)) {
		db_error("VIDIOC_G_CTRL failed:value:%d\n", ctrl.value);
	}
	return ctrl.value;
}

static int read_frame(int fd) {
	struct v4l2_buffer buf;
	unsigned long phyaddr;
	struct v4l2_plane planes[VIDEO_MAX_PLANES];
	memset(&buf, 0, sizeof(struct v4l2_buffer));
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	buf.memory = V4L2_MEMORY_MMAP;
	buf.length = nplanes;
	buf.m.planes = planes;

	ioctl(fd, VIDIOC_DQBUF, &buf);

	if(sensor_type == V4L2_SENSOR_TYPE_YUV) {
		phyaddr = buf.m.planes[0].m.mem_offset;
	} else if(sensor_type == V4L2_SENSOR_TYPE_RAW) {
		phyaddr = buf.m.planes[0].m.mem_offset + ALIGN_4K(ALIGN_16B(input_size.width) * input_size.height * 3 >> 1);
	}
	disp_set_addr(display_size.width, display_size.height, &phyaddr);
#ifdef DEBUG_CAMERA
	static int a = 0;
	int ret;
	if(a == 50) {
		FILE *frame_file = NULL;
		frame_file = fopen("/data/frame1.yuv","wb");
		if (frame_file == NULL) db_error("open err\n");
		int frameszie = display_size.width*display_size.height*1.5;
		void *vaddr;
		vaddr = buffers[buf.index].start + phyaddr - buf.m.offset;

		db_debug("buf.length %d, vaddr: %p\n",buf.length,vaddr);
		db_debug("buf.length: %d\n,vaddr: %p\n",frameszie,vaddr);
		ret = fwrite((void*)vaddr,1,frameszie,frame_file);
		db_debug("fwrite ret is %d\n",ret);
		fclose(frame_file);
		a++;
	} else
		a++;
#endif
	ioctl(fd, VIDIOC_QBUF, &buf);
	return 1;
}

static int disp_init(int x,int y,int width,int height) {
	unsigned int arg[6];
	memset(&test_info, 0, sizeof(struct test_layer_info));
	if((test_info.dispfh = open("/dev/disp",O_RDWR)) == -1) {
		db_error("open display device fail!\n");
		return -1;
	}
	/* get current output type */
	disp_output_type output_type;
	for (screen_id = 0;screen_id < 3;screen_id++) {
		arg[0] = screen_id;
		output_type = (disp_output_type)ioctl(test_info.dispfh, DISP_GET_OUTPUT_TYPE, (void*)arg);
		if(output_type != DISP_OUTPUT_TYPE_NONE) {
			db_debug("the output type: %d\n",screen_id);
			break;
		}
	}
	test_info.screen_id = 0; /* 0 for lcd ,1 for hdmi, 2 for edp */
	test_info.layer_config.channel = 0;
	test_info.layer_config.layer_id = camera_layer;
	test_info.layer_config.info.zorder = 1;
	/* test_info.layer_config.info.ck_enable = 0; */
	test_info.layer_config.info.alpha_mode = 1; /* global alpha */
	test_info.layer_config.info.alpha_value = 0xff;
	/* test_info.layer_config.info.pipe = 0; */
	/* test_info.width = ioctl(test_info.dispfh,DISP_CMD_GET_SCN_WIDTH,(void*)arg); 	get screen width and height */
	/* test_info.height = ioctl(test_info.dispfh,DISP_CMD_GET_SCN_HEIGHT,(void*)arg); */

	/* crop setting */
	/* test_info.layer_config.info.fb.crop.x = x; */
	/* test_info.layer_config.info.fb.crop.y = y; */
	/* test_info.layer_config.info.fb.crop.width = (unsigned long long)width << 32; */
	/* layer_config.info.fb.crop.height = (unsigned long long)height << 32; */
	/* display window of the screen */
	test_info.layer_config.info.screen_win.x = x;
	test_info.layer_config.info.screen_win.y = y;
	test_info.layer_config.info.screen_win.width = width;
	test_info.layer_config.info.screen_win.height = height;

	/* mode */
	test_info.layer_config.info.mode = LAYER_MODE_BUFFER;

	/* data format */
	test_info.layer_config.info.fb.format = DISP_FORMAT_YUV420_SP_UVUV;
	return 0;
}
static int disp_quit(void) {
	int ret;
	unsigned int arg[6];
	test_info.layer_config.enable = 0;
	arg[0] = test_info.screen_id;
	arg[1] = (int)&test_info.layer_config;
	arg[2] = 0;
	ret = ioctl(test_info.dispfh, DISP_LAYER_SET_CONFIG, (void*)arg);
	if(0 != ret)
		db_error("fail to set layer info\n");

	close(test_info.dispfh);
	memset(&test_info, 0, sizeof(struct test_layer_info));

	return 0;
}
int disp_set_addr(int width, int height, unsigned long *addr) {
	unsigned int arg[6];
	int ret;
	/* printf("width %d,height %d\n",width,height); */
	/* source frame size */
	test_info.layer_config.info.fb.size[0].width = width;
	test_info.layer_config.info.fb.size[0].height = height;
	test_info.layer_config.info.fb.size[1].width = width/2;
	test_info.layer_config.info.fb.size[1].height = height;
	/* test_info.layer_config.info.fb.size[2].width = width / 2; */
	/* test_info.layer_config.info.fb.size[2].height = height / 2; */

	/* src */
	/* test_info.layer_config.info.fb.crop.x = 0; */
	/* test_info.layer_config.info.fb.crop.y = 0; */
	test_info.layer_config.info.fb.crop.width = (unsigned long long)width << 32;
	test_info.layer_config.info.fb.crop.height = (unsigned long long)height << 32;

	/* test_info.layer_config.info.fb.src_win.width = width; */
	/* test_info.layer_config.info.fb.src_win.height = height; */

	test_info.layer_config.info.fb.addr[0] = (*addr);
	test_info.layer_config.info.fb.addr[1] = (test_info.layer_config.info.fb.addr[0] + width*height);
	/* test_info.layer_config.info.fb.addr[2] = (int)(test_info.layer_config.info.fb.addr[0] + width*height*5/4); */

	test_info.layer_config.enable = 1;
	arg[0] = test_info.screen_id;
	arg[1] = (int)&test_info.layer_config;
	arg[2] = 1;
	ret = ioctl(test_info.dispfh, DISP_LAYER_SET_CONFIG, (void*)arg);
	if(0 != ret)
		db_error("disp_set_addr fail to set layer info\n");

	return 0;
}

static void *video_mainloop(void *args) {
	int fd;
	fd_set fds;
	struct timeval tv;
	int r;
	char dev_name[32];
	struct v4l2_input inp;
	struct v4l2_format fmt;
	struct v4l2_pix_format sub_fmt;
	/* struct rot_channel_cfg rot; */
	struct v4l2_streamparm parms;
	struct v4l2_requestbuffers req;
	int i;
	enum v4l2_buf_type type;
	int try_times = 0;

	if (csi_cnt == 1) {
		snprintf(dev_name, sizeof(dev_name), "/dev/video0");
	} else {
		snprintf(dev_name, sizeof(dev_name), "/dev/video%d", (int)args);
	}

	db_debug("open %s\n", dev_name);

	while (try_times < 10) {
		if ((fd = open(dev_name, O_RDWR,S_IRWXU)) < 0)
			db_error("can't open %s(%s)\n", dev_name, strerror(errno));
		else
			break;
		try_times ++;
		usleep(100000);
	}
	if (try_times == 10)
		goto open_err;

	fcntl(fd, F_SETFD, FD_CLOEXEC);
	inp.index = (int)args;
	inp.type = V4L2_INPUT_TYPE_CAMERA;
	db_debug("inp.index: %d\n", inp.index);

	/* set input input index */
	if (ioctl(fd, VIDIOC_S_INPUT, &inp) == -1) {
		db_error("VIDIOC_S_INPUT error\n");
		goto err;
	}

	parms.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	 /* parms.parm.capture.capturemode = V4L2_MODE_VIDEO; */
	parms.parm.capture.timeperframe.numerator = 1;
	parms.parm.capture.timeperframe.denominator = fps;
	parms.parm.capture.reserved[0] = 0;
	parms.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	parms.parm.capture.capturemode = V4L2_MODE_PREVIEW;

	if (ioctl(fd, VIDIOC_S_PARM, &parms) == -1) {
		db_error("set frequence failed\n");
		 /* goto err; */
	}
	/* set image format */
	memset(&fmt, 0, sizeof(struct v4l2_format));
	memset(&sub_fmt, 0, sizeof(struct v4l2_pix_format));
	//memset(&rot,0,sizeof(struct rot_channel_cfg));
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	fmt.fmt.pix_mp.width = input_size.width;
	fmt.fmt.pix_mp.height = input_size.height;
	fmt.fmt.pix_mp.pixelformat = csi_format;
	fmt.fmt.pix_mp.field = V4L2_FIELD_NONE;
	db_msg("camera resolution set to sensor = %d*%d, num_planes = %d\n",fmt.fmt.pix_mp.width,
			fmt.fmt.pix_mp.height, fmt.fmt.pix_mp.num_planes);
	if (ioctl(fd, VIDIOC_S_FMT, &fmt)<0) {
		db_error("set image format failed\n");
		goto err;
	}
	if (ioctl(fd, VIDIOC_G_FMT, &fmt)<0) {
		db_error("VIDIOC_G_FMT Failed: %s", strerror(errno));
	}
	db_msg("camera resolution got from sensor = %d*%d, num_planes = %d\n",fmt.fmt.pix_mp.width,
			fmt.fmt.pix_mp.height, fmt.fmt.pix_mp.num_planes);
	nplanes = fmt.fmt.pix_mp.num_planes;

	 /* get sensor type */
	sensor_type = getSensorType(fd);
	/*
	if(sensor_type == V4L2_SENSOR_TYPE_RAW) {
		sub_fmt.width = input_size.width;
		sub_fmt.height = input_size.height;
		sub_fmt.pixelformat = csi_format;
		sub_fmt.field = V4L2_FIELD_NONE;
		rot.sel_ch = 1;
		rot.rotation = 0;
		if(ioctl(fd, VIDIOC_SET_SUBCHANNEL, &sub_fmt) < 0) {
		 	    db_error("VIDIOC_SET_SUBCHANNEL error\n");
				goto err;
		}

		if(ioctl(fd, VIDIOC_SET_ROTCHANNEL, &rot) < 0) {
		    db_error("VIDIOC_SET_ROTCHANNEL error\n");
		    goto err;
		}
    }
    */

	db_debug("sensor_type:%d ;image input width #%d height #%d, diplay width #%d height #%d\n",
			sensor_type, input_size.width, input_size.height, display_size.width, display_size.height);
	sensor_type = 0;

	input_size.width = fmt.fmt.pix_mp.width;
	input_size.height = fmt.fmt.pix_mp.height;
	if(sensor_type == V4L2_SENSOR_TYPE_YUV) {
		display_size.width = fmt.fmt.pix_mp.width;
		display_size.height = fmt.fmt.pix_mp.height;
	} else if(sensor_type == V4L2_SENSOR_TYPE_RAW) {
		display_size.width = sub_fmt.width;
		display_size.height = sub_fmt.height;
	}

	db_msg("----------------image input width #%d height #%d, diplay width #%d height #%d\n",
			input_size.width, input_size.height, display_size.width, display_size.height);

	/* request buffer */
	struct v4l2_plane planes[VIDEO_MAX_PLANES];
	memset(&req, 0, sizeof(struct v4l2_requestbuffers));
	memset(planes, 0, VIDEO_MAX_PLANES*sizeof(struct v4l2_plane));
	req.count = req_frame_num;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	req.memory = V4L2_MEMORY_MMAP;
	if(-1 == ioctl(fd, VIDIOC_REQBUFS, &req)){
		db_error("VIDIOC_REQBUFS error!\n");
	}

	buffers = calloc(req.count, sizeof(struct buffer));
	for (nbuffers = 0; nbuffers < req.count; ++nbuffers) {
		struct v4l2_buffer buf;

		memset(&buf, 0, sizeof(struct v4l2_buffer));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = nbuffers;
		buf.length = nplanes;
		buf.m.planes = planes;
		if (NULL == buf.m.planes) {
			db_error("buf.m.planes calloc failed!\n");
		}

		if (ioctl(fd, VIDIOC_QUERYBUF, &buf) == -1) {
			db_error("VIDIOC_QUERYBUF error\n");
			goto buffer_rel;
		}

		buffers[nbuffers].length = buf.m.planes[0].length;
		buffers[nbuffers].start = mmap(NULL, buf.m.planes[0].length,
			PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.planes[0].m.mem_offset);
		if (buffers[nbuffers].start == MAP_FAILED) {
			db_error("mmap failed\n");
			goto buffer_rel;
		}
	}

	db_msg("buffer count:%d\n",nbuffers);
	for (i = 0; i < nbuffers; ++i) {
		struct v4l2_buffer buf;

		memset(&buf, 0, sizeof(struct v4l2_buffer));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index  = i;
		buf.length = nplanes;
		buf.m.planes = (struct v4l2_plane *)calloc(nplanes, sizeof(struct v4l2_plane));
		if (ioctl(fd, VIDIOC_QBUF, &buf) == -1) {
			db_error("VIDIOC_QBUF error\n");
			goto unmap;
		}
	}

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE_MPLANE;
	if (ioctl(fd, VIDIOC_STREAMON, &type) == -1) {
		db_error("VIDIOC_STREAMON error\n");
		goto disp_exit;
	}

	while (1) {
		if (stop_flag)
			break;

		while (1) {
			if (stop_flag)
				break;

			FD_ZERO(&fds);
			FD_SET(fd, &fds);

			/* timeout */
			tv.tv_sec = 2;
			tv.tv_usec = 0;

			r = select(fd + 1, &fds, NULL, NULL, &tv);
			if (r == -1) {
				if (errno == EINTR) {
					continue;
				}

				db_error("select error\n");
			}

			if (r == 0) {
				db_error("select timeout\n");
				goto stream_off;
			}

			if (read_frame(fd)) {
				/* break; */
			}
	}
	FD_ZERO(&fds);
	}

	if(-1 == ioctl(fd, VIDIOC_STREAMOFF, &type)) {
		db_error("vidioc_streamoff error!\n");
		pthread_exit((void *)-1);
	}
	db_msg("VIDIOC_STREAMOFF ok");
	/* disp_stop(); */
	for (i = 0; i < nbuffers; i++) {
		if(-1 == munmap(buffers[i].start, buffers[i].length)) {
			db_error("munmap error!\n");
			pthread_exit((void *)-1);
		}
	}
	free(buffers);
	db_msg("close fd\n");
	if(0 != close(fd)) {
		db_error("close video fd error!\n");
		pthread_exit((void *)-1);
	}
	db_msg("exit camera test thread\n");
	pthread_exit(0);
	db_error("ho my gad!\n");
stream_off:
	ioctl(fd, VIDIOC_STREAMOFF, &type);
disp_exit:
	/* disp_stop(); */
unmap:
	for (i = 0; i < nbuffers; i++) {
		munmap(buffers[i].start, buffers[i].length);
	}
buffer_rel:
	free(buffers);
err:
	close(fd);
open_err:
	pthread_exit((void *)-1);
}

int camera_test_init(int x,int y,int width,int height) {
	db_debug("the window: x: %d,y: %d,width: %d,height: %d\n", x, y, width, height);
	if (script_fetch("camera", "dev_no", &dev_no, 1) ||
			(dev_no != 0 && dev_no != 1)) {
		dev_no = 0;
	}

	if (script_fetch("camera", "dev_cnt", &dev_cnt, 1) ||
			(dev_cnt != 1 && dev_cnt != 2)) {
		db_warn("camera: invalid dev_cnt #%d, use default #1\n", dev_cnt);
		dev_cnt = 1;
	}

	if (script_fetch("camera", "csi_cnt", &csi_cnt, 1) ||
			(csi_cnt != 1 && csi_cnt != 2)) {
		db_warn("camera: invalid csi_cnt #%d, use default #1\n", csi_cnt);
		csi_cnt = 1;
	}

	if (script_fetch("camera", "fps", &fps, 1) ||
			fps < 15) {
		db_warn("camera: invalid fps(must >= 15) #%d, use default #15\n", fps);
		fps = 15;
	}

	db_debug("camera: device count #%d, csi count #%d, fps #%d\n", dev_cnt, csi_cnt, fps);

	csi_format = V4L2_PIX_FMT_NV12;
	req_frame_num = 10;

	/* 480p */
	input_size.width = 640;
	input_size.height = 480;

	if (disp_init(x,y,width,height) < 0) {
		db_error("camera: disp init failed\n");
		return -1;
	}

	if (dev_no >= dev_cnt) {
		dev_no = 0;
	}

	video_tid = 0;
	db_debug("camera: create video mainloop thread\n");
	if (pthread_create(&video_tid, NULL, video_mainloop, (void *)dev_no)) {
		db_error("camera: can't create video mainloop thread(%s)\n", strerror(errno));
		return -1;
	}
	return 0;
}

int camera_test_quit(void) {
	void *retval;

	if (video_tid) {
		stop_flag = 1;
		db_msg("camera: waiting for camera thread finish...\n");
		if (pthread_join(video_tid, &retval)) {
			db_error("cameratester: can't join with camera thread\n");
		}

		db_msg("camera: camera thread exit code #%d\n", (int)retval);
		video_tid = 0;
	}

	disp_quit();

	return 0;
}

int get_camera_cnt(void) {
	return dev_cnt;
}

int switch_camera(void) {
	void *retval;

	db_msg("camera: switch camera... video_tid:%u\n",video_tid);
	if (video_tid) {
		stop_flag = 1;
		db_msg("camera: waiting for camera thread finish...\n");
		if (pthread_join(video_tid, &retval)) {
			db_error("cameratester: can't join with camera thread\n");
		}
		db_msg("camera: camera thread exit code #%d\n", (int)retval);
		video_tid = 0;
	}

	dev_no++;
	if (dev_no >= dev_cnt) {
		dev_no = 0;
	}

	stop_flag = 0;
	db_debug("cameratester: create video mainloop thread\n");
	if (pthread_create(&video_tid, NULL, video_mainloop, (void *)dev_no)) {
		db_error("camera: can't create video mainloop thread(%s)\n", strerror(errno));
		return -1;
	}

	return 0;
}
