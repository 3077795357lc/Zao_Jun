/*
 * 摄像头监控模块
 *
 * 负责 V4L2 采集与像素格式转换，供 UI 层显示：
 *   1) camera_monitor_start() 打开 /dev/video1、配置 YUYV 640x480、
 *      申请并映射 3 个驱动帧缓冲、开流，然后创建采集线程；
 *   2) 采集线程用 poll() 等待出帧，把 YUYV 转成 XRGB8888 后写入双缓冲；
 *   3) UI 线程通过 camera_monitor_take_frame() 取最新一帧，不阻塞、不碰硬件。
 *
 * 这样 LVGL 主线程不会卡在 DQBUF 上，摄像头的等待工作全部在后台线程完成。
 */
#include "hardware/camera_monitor.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <linux/videodev2.h>

#define CAMERA_DMA_BUFFER_COUNT   3
#define CAMERA_PIXEL_FORMAT       V4L2_PIX_FMT_YUYV
#define CAMERA_CAPTURE_FPS        30
#define CAMERA_POLL_TIMEOUT_MS    50
#define CAMERA_CONVERTED_COUNT    2

/* 驱动通过 mmap 暴露给用户态的一帧缓冲 */
typedef struct {
    unsigned char *start;
    unsigned long length;
} camera_dma_buffer_t;

static int camera_device_fd = -1;
static camera_dma_buffer_t camera_dma_buffers[CAMERA_DMA_BUFFER_COUNT];
static int camera_dma_buffer_count;
static int camera_frame_width;
static int camera_frame_height;

/* 转换后的双缓冲：采集线程写其中一块，UI 读另一块 */
static uint32_t *camera_frame_buffers[CAMERA_CONVERTED_COUNT];
static int camera_write_buffer_index;
static int camera_latest_buffer_index = -1;
static bool camera_frame_ready;
static pthread_mutex_t camera_frame_mutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_t camera_capture_thread;
static volatile bool camera_capture_running;
static bool camera_thread_started;
static bool camera_monitoring;

/* ============================ 设备与格式 ============================ */

static int camera_device_open(void)
{
    struct v4l2_capability capability = {0};

    /* 非阻塞打开：配合 poll() 等待帧到达，stop 时也能及时退出线程 */
    camera_device_fd = open(CAMERA_DEVICE_PATH, O_RDWR | O_NONBLOCK);
    if (camera_device_fd < 0) {
        fprintf(stderr, "camera: open %s failed: %s\n",
                CAMERA_DEVICE_PATH, strerror(errno));
        return -1;
    }

    if (ioctl(camera_device_fd, VIDIOC_QUERYCAP, &capability) < 0) {
        perror("camera: VIDIOC_QUERYCAP");
        return -1;
    }

    if (!(capability.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {
        fprintf(stderr, "camera: %s is not a video capture device\n",
                CAMERA_DEVICE_PATH);
        return -1;
    }

    if (!(capability.capabilities & V4L2_CAP_STREAMING)) {
        fprintf(stderr, "camera: %s does not support streaming I/O\n",
                CAMERA_DEVICE_PATH);
        return -1;
    }

    return 0;
}

static int camera_format_configure(void)
{
    struct v4l2_format format = {0};
    struct v4l2_streamparm stream_parameter = {0};

    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.width = CAMERA_FRAME_WIDTH;
    format.fmt.pix.height = CAMERA_FRAME_HEIGHT;
    format.fmt.pix.pixelformat = CAMERA_PIXEL_FORMAT;

    if (ioctl(camera_device_fd, VIDIOC_S_FMT, &format) < 0) {
        perror("camera: VIDIOC_S_FMT");
        return -1;
    }

    if (format.fmt.pix.pixelformat != CAMERA_PIXEL_FORMAT) {
        fprintf(stderr, "camera: device does not support YUYV format\n");
        return -1;
    }

    if (format.fmt.pix.width != CAMERA_FRAME_WIDTH ||
        format.fmt.pix.height != CAMERA_FRAME_HEIGHT) {
        fprintf(stderr, "camera: got %ux%u instead of %dx%d\n",
                format.fmt.pix.width, format.fmt.pix.height,
                CAMERA_FRAME_WIDTH, CAMERA_FRAME_HEIGHT);
        return -1;
    }

    camera_frame_width = format.fmt.pix.width;
    camera_frame_height = format.fmt.pix.height;

    /* 帧率设置不是所有设备都支持，失败不影响采集 */
    stream_parameter.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(camera_device_fd, VIDIOC_G_PARM, &stream_parameter) == 0 &&
        (stream_parameter.parm.capture.capability & V4L2_CAP_TIMEPERFRAME)) {
        printf("可改fps\n");
        stream_parameter.parm.capture.timeperframe.numerator = 1;
        stream_parameter.parm.capture.timeperframe.denominator = CAMERA_CAPTURE_FPS;
        ioctl(camera_device_fd, VIDIOC_S_PARM, &stream_parameter);
    }

    printf("camera: frame size %dx%d\n", camera_frame_width, camera_frame_height);
    return 0;
}

/* ============================ 驱动帧缓冲 ============================ */

static void camera_dma_buffers_unmap(void)
{
    int buffer_index;

    for (buffer_index = 0; buffer_index < camera_dma_buffer_count; buffer_index++) {
        if (camera_dma_buffers[buffer_index].start != NULL) {
            munmap(camera_dma_buffers[buffer_index].start,
                   camera_dma_buffers[buffer_index].length);
            camera_dma_buffers[buffer_index].start = NULL;
            camera_dma_buffers[buffer_index].length = 0;
        }
    }
    camera_dma_buffer_count = 0;
}

static int camera_dma_buffers_map(void)
{
    struct v4l2_requestbuffers request = {0};
    struct v4l2_buffer buffer = {0};
    int buffer_index;

    request.count = CAMERA_DMA_BUFFER_COUNT;
    request.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    request.memory = V4L2_MEMORY_MMAP;

    if (ioctl(camera_device_fd, VIDIOC_REQBUFS, &request) < 0) {
        perror("camera: VIDIOC_REQBUFS");
        return -1;
    }

    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buffer.memory = V4L2_MEMORY_MMAP;

    for (buffer_index = 0; buffer_index < (int)request.count; buffer_index++) {
        void *mapped_address;

        buffer.index = buffer_index;
        if (ioctl(camera_device_fd, VIDIOC_QUERYBUF, &buffer) < 0) {
            perror("camera: VIDIOC_QUERYBUF");
            return -1;
        }

        mapped_address = mmap(NULL, buffer.length, PROT_READ | PROT_WRITE,
                              MAP_SHARED, camera_device_fd, buffer.m.offset);
        if (mapped_address == MAP_FAILED) {
            perror("camera: mmap");
            return -1;
        }

        camera_dma_buffers[buffer_index].start = mapped_address;
        camera_dma_buffers[buffer_index].length = buffer.length;
        camera_dma_buffer_count = buffer_index + 1;
    }

    /* 所有缓冲先入队，驱动才有地方写采集数据 */
    for (buffer_index = 0; buffer_index < camera_dma_buffer_count; buffer_index++) {
        memset(&buffer, 0, sizeof(buffer));
        buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer.memory = V4L2_MEMORY_MMAP;
        buffer.index = buffer_index;

        if (ioctl(camera_device_fd, VIDIOC_QBUF, &buffer) < 0) {
            perror("camera: VIDIOC_QBUF");
            return -1;
        }
    }

    return 0;
}

/* ============================ 像素转换 ============================ */

static int camera_clamp_component(int value)
{
    if (value < 0) {
        return 0;
    }
    if (value > 255) {
        return 255;
    }
    return value;
}

/* 一对 YUYV 像素（Y0 U Y1 V，共 4 字节）转成 2 个 XRGB8888 像素。
 * BT.601 整数公式，系数已乘 256，最后右移 8 位还原。 */
static void camera_convert_pair(const unsigned char *yuyv_source,
                                uint32_t *xrgb_destination)
{
    int luma_values[2];
    int blue_difference = (int)yuyv_source[1] - 128;
    int red_difference = (int)yuyv_source[3] - 128;
    int pixel_offset;

    luma_values[0] = (int)yuyv_source[0] - 16;
    luma_values[1] = (int)yuyv_source[2] - 16;

    for (pixel_offset = 0; pixel_offset < 2; pixel_offset++) {
        int luma = luma_values[pixel_offset];
        int red;
        int green;
        int blue;

        if (luma < 0) {
            luma = 0;
        }

        red = (298 * luma + 409 * red_difference + 128) >> 8;
        green = (298 * luma - 100 * blue_difference - 208 * red_difference + 128) >> 8;
        blue = (298 * luma + 516 * blue_difference + 128) >> 8;

        xrgb_destination[pixel_offset] =
            ((uint32_t)camera_clamp_component(red) << 16) |
            ((uint32_t)camera_clamp_component(green) << 8) |
            (uint32_t)camera_clamp_component(blue);
    }
}

static void camera_convert_frame(const unsigned char *yuyv_frame,
                                 uint32_t *xrgb_frame)
{
    int row;

    for (row = 0; row < camera_frame_height; row++) {
        const unsigned char *source_row =
            yuyv_frame + row * camera_frame_width * 2;
        uint32_t *destination_row =
            xrgb_frame + row * camera_frame_width;
        int column;

        for (column = 0; column < camera_frame_width; column += 2) {
            camera_convert_pair(source_row, destination_row);
            source_row += 4;
            destination_row += 2;
        }
    }
}

/* ============================ 采集线程 ============================ */

static void *camera_capture_thread_main(void *argument)
{
    struct pollfd camera_poll = {0};
    struct v4l2_buffer buffer = {0};

    (void)argument;

    camera_poll.fd = camera_device_fd;
    camera_poll.events = POLLIN;

    while (camera_capture_running) {
        int poll_result = poll(&camera_poll, 1, CAMERA_POLL_TIMEOUT_MS);

        if (poll_result < 0) {
            if (errno == EINTR) {
                continue;
            }
            perror("camera: poll");
            break;
        }

        if (poll_result == 0) {
            continue;   /* 超时：回到循环顶部检查退出标志 */
        }

        if (!(camera_poll.revents & POLLIN)) {
            continue;
        }

        memset(&buffer, 0, sizeof(buffer));
        buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buffer.memory = V4L2_MEMORY_MMAP;

        if (ioctl(camera_device_fd, VIDIOC_DQBUF, &buffer) < 0) {
            if (errno == EAGAIN) {
                continue;
            }
            perror("camera: VIDIOC_DQBUF");
            break;
        }

        camera_convert_frame(camera_dma_buffers[buffer.index].start,
                             camera_frame_buffers[camera_write_buffer_index]);

        if (ioctl(camera_device_fd, VIDIOC_QBUF, &buffer) < 0) {
            perror("camera: VIDIOC_QBUF");
            break;
        }

        pthread_mutex_lock(&camera_frame_mutex);
        camera_latest_buffer_index = camera_write_buffer_index;
        camera_frame_ready = true;
        pthread_mutex_unlock(&camera_frame_mutex);

        camera_write_buffer_index = (camera_write_buffer_index + 1) % CAMERA_CONVERTED_COUNT;
    }

    return NULL;
}

/* ============================ 对外接口 ============================ */

static int camera_frame_buffers_allocate(void)
{
    size_t frame_bytes =
        (size_t)CAMERA_FRAME_WIDTH * CAMERA_FRAME_HEIGHT * sizeof(uint32_t);
    int buffer_index;
    int free_index;

    if (camera_frame_buffers[0] != NULL) {
        return 0;   /* 缓冲区常驻，重复启动时复用 */
    }

    for (buffer_index = 0; buffer_index < CAMERA_CONVERTED_COUNT; buffer_index++) {
        camera_frame_buffers[buffer_index] = malloc(frame_bytes);
        if (camera_frame_buffers[buffer_index] == NULL) {
            fprintf(stderr, "camera: out of memory for frame buffer\n");
            for (free_index = 0; free_index < CAMERA_CONVERTED_COUNT; free_index++) {
                free(camera_frame_buffers[free_index]);
                camera_frame_buffers[free_index] = NULL;
            }
            return -1;
        }
        memset(camera_frame_buffers[buffer_index], 0, frame_bytes);
    }

    return 0;
}

int camera_monitor_start(void)
{
    enum v4l2_buf_type buffer_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (camera_monitoring) {
        return 0;
    }

    if (camera_device_open() < 0) {
        goto fail;
    }

    if (camera_format_configure() < 0) {
        goto fail;
    }

    if (camera_frame_buffers_allocate() < 0) {
        goto fail;
    }

    if (camera_dma_buffers_map() < 0) {
        goto fail;
    }

    if (ioctl(camera_device_fd, VIDIOC_STREAMON, &buffer_type) < 0) {
        perror("camera: VIDIOC_STREAMON");
        goto fail;
    }

    pthread_mutex_lock(&camera_frame_mutex);
    camera_frame_ready = false;
    camera_latest_buffer_index = -1;
    pthread_mutex_unlock(&camera_frame_mutex);
    camera_write_buffer_index = 0;

    camera_capture_running = true;
    if (pthread_create(&camera_capture_thread, NULL,
                       camera_capture_thread_main, NULL) != 0) {
        camera_capture_running = false;
        fprintf(stderr, "camera: create capture thread failed\n");
        ioctl(camera_device_fd, VIDIOC_STREAMOFF, &buffer_type);
        goto fail;
    }

    camera_thread_started = true;
    camera_monitoring = true;
    return 0;

fail:
    camera_dma_buffers_unmap();
    if (camera_device_fd >= 0) {
        close(camera_device_fd);
        camera_device_fd = -1;
    }
    return -1;
}

void camera_monitor_stop(void)
{
    enum v4l2_buf_type buffer_type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (!camera_monitoring && !camera_thread_started) {
        return;
    }

    //回收采集线程
    if (camera_thread_started) {
        camera_capture_running = false;
        pthread_join(camera_capture_thread, NULL);
        camera_thread_started = false;
    }

    if (camera_device_fd >= 0) {
        ioctl(camera_device_fd, VIDIOC_STREAMOFF, &buffer_type);
    }

    camera_dma_buffers_unmap();

    if (camera_device_fd >= 0) {
        close(camera_device_fd);
        camera_device_fd = -1;
    }

    pthread_mutex_lock(&camera_frame_mutex);
    camera_frame_ready = false;
    camera_latest_buffer_index = -1;
    pthread_mutex_unlock(&camera_frame_mutex);

    camera_monitoring = false;
}

bool camera_monitor_is_running(void)
{
    return camera_monitoring;
}

bool camera_monitor_take_frame(const uint32_t **frame_pixels)
{
    bool has_new_frame = false;

    pthread_mutex_lock(&camera_frame_mutex);
    if (camera_frame_ready && camera_latest_buffer_index >= 0) {
        *frame_pixels = camera_frame_buffers[camera_latest_buffer_index];
        camera_frame_ready = false;
        has_new_frame = true;
    }
    pthread_mutex_unlock(&camera_frame_mutex);

    return has_new_frame;
}
