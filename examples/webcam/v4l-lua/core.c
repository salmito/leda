/*
 Copyright (c) 2011 Gabriel Duarte <confusosk8@gmail.com>

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
*/


#include "core.h"

int fd =-1;
int w = 720, h = 480; /* ????? :(  FIXME */
struct v4lconvert_data *v4lconvert_data;
struct v4l2_format src_fmt;    /* raw format */
struct v4l2_buffer buf;
unsigned char *dst_buf;
struct v4l2_format fmt;
const char *dev_name;
struct buffer *buffers;
int n_buffers;


int getwidth()
{
    return w;
}

int getheight()
{
    return h;
}

int xioctl(int fd, int request, void *arg)
{
    int r;

    do {
        r = ioctl(fd, request, arg);
    } while (r < 0 && EINTR == errno);
    return r;
}


 void errno_exit(const char *s)
{
    fprintf(stderr, "%s error %d, %s\n", s, errno, strerror(errno));
    fprintf(stderr, "%s\n", v4lconvert_get_error_message(v4lconvert_data));
    exit(EXIT_FAILURE);
}
                                                  

unsigned char *newframe()
{
    get_frame();
    process_image((unsigned char *)buffers[buf.index].start, buf.bytesused, w, h);
    return dst_buf;
}


void process_image(unsigned char *p, int len, int W, int H)
{

    if(v4lconvert_convert(v4lconvert_data,
                           &src_fmt,
                           &fmt,
                           p, len,
                           dst_buf,
                           fmt.fmt.pix.sizeimage) < 0)
   {
       if(errno != EAGAIN)
       {
           perror("v4l_convert");
       }
        p = dst_buf;
        len = fmt.fmt.pix.sizeimage;
    }
}

int read_frame()
{
    memset(&(buf), 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if(xioctl(fd, VIDIOC_DQBUF, &buf) < 0)
    {
        switch (errno)
        {
            case EAGAIN:
                return 0;
                break;
            case EIO:
                /* Could ignore EIO, see spec. */
                /* fall through */
            default:
                /*errno_exit("VIDIOC_DQBUF");*/
                perror("VIDIOC_DQBUF");
        }
    }
        
    assert((unsigned char)buf.index < n_buffers);
    /*process_image((unsigned char*)buffers[buf.index].start, buf.bytesused, w, h);*/

    if(xioctl(fd, VIDIOC_QBUF, &buf) < 0)
        perror("VIDIOC_QBUF");
        /*errno_exit("VIDIOC_QBUF");*/

    return 0;
}

int get_frame(void)
{
    fd_set fds;
    struct timeval tv;
    int r;

    FD_ZERO(&fds);
    FD_SET(fd, &fds);

    /* Timeout. */
    tv.tv_sec = 2;
    tv.tv_usec = 0;

    r = select(fd + 1, &fds, NULL, NULL, &tv);
    if(r < 0)
    {
        if(EINTR == errno)
            return -1;

        perror("select");
        /*errno_exit("select");*/
    }

    if(0 == r)
    {
        perror("select timeout");
        /*exit(EXIT_FAILURE);*/
        return -1;
    }
    
    read_frame();
    
    return 0;
}

void stop_capturing(void)
{
    printf("Stop Capturing...\n");
}

void start_capturing(void)
{
    int i;
    enum v4l2_buf_type type;
    struct v4l2_buffer buf;

   /*printf("mmap method\n");*/
   
   for (i = 0; i < n_buffers; ++i)
   {
       memset(&(buf), 0, sizeof(buf));
       buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
       buf.memory = V4L2_MEMORY_MMAP;
       buf.index = i;

       if(xioctl(fd, VIDIOC_QBUF, &buf) < 0)
            perror("VIDIOC_QBUF");
   }

   type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

   if(xioctl(fd, VIDIOC_STREAMON, &type) < 0)
       perror("VIDIOC_STREAMON");
}

void uninit_device()
{
    int i;

    for(i = 0; i < n_buffers; ++i)
        if(-1 == munmap(buffers[i].start, buffers[i].length))
            errno_exit("munmap");
    
    if(dst_buf != NULL)
        free(dst_buf);
              
    if(buffers != NULL)
        free(buffers);
}

void init_mmap()
{
    struct v4l2_requestbuffers req;
    struct v4l2_buffer buf;

    memset(&(req), 0, sizeof(req));
    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;


    if(xioctl(fd, VIDIOC_REQBUFS, &req) < 0)
    {
        if(EINVAL == errno)
        {
            fprintf(stderr, "%s does not support memory mapping\n", dev_name);
            exit(EXIT_FAILURE);
        }
        else
        {
            errno_exit("VIDIOC_REQBUFS");
        }
    }

    if(req.count < 2)
    {
        fprintf(stderr, "Insufficient buffer memory on %s\n", dev_name);
        exit(EXIT_FAILURE);
    }


    buffers = (struct buffer*) calloc(req.count, sizeof(struct buffer));

    if(!buffers)
    {
        fprintf(stderr, "Out of memory\n");
        perror("EXIT_FAILURE");
    }

    for(n_buffers = 0; n_buffers < (unsigned char)req.count; ++n_buffers)
    {
        memset(&(buf), 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = n_buffers;

        if(xioctl(fd, VIDIOC_QUERYBUF, &buf) < 0)
            errno_exit("VIDIOC_QUERYBUF");

        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start = mmap(NULL /* start anywhere */ ,
                        buf.length,
                        PROT_READ | PROT_WRITE
                        /* required */ ,
                        MAP_SHARED
                        /* recommended */ ,
                        fd, buf.m.offset);

        if(MAP_FAILED == buffers[n_buffers].start)
            errno_exit("mmap");
    }
}

void init_device()
{
    struct v4l2_capability cap;
    int ret;
    int sizeimage;

    if(xioctl(fd, VIDIOC_QUERYCAP, &cap) < 0)
    {
        if(EINVAL == errno)
        {
            fprintf(stderr, "%s is no V4L2 device\n", dev_name);
            perror("EXIT_FAILURE");
            return;
        } 
        else
        {
            perror("VIDIOC_QUERYCAP");
            return ;
        }
    }

    if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
    {
        fprintf(stderr, "%s is no video capture device\n", dev_name);
        /*exit(EXIT_FAILURE);*/
        perror("EXIT_FAILURE");
        return;
    }

    memset(&(fmt), 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width = w;
    fmt.fmt.pix.height = h;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
    
    v4lconvert_data = v4lconvert_create(fd);

    if(v4lconvert_data == NULL)
    {
        perror("v4lconvert_create");
        return;
    }
        
    if(v4lconvert_try_format(v4lconvert_data, &fmt, &src_fmt) != 0)
    {
        /*errno_exit("v4lconvert_try_format");*/
        perror("v4lconvert_try_format");
        return;
    }
    
    ret = xioctl(fd, VIDIOC_S_FMT, &src_fmt);
    sizeimage = src_fmt.fmt.pix.sizeimage;
    dst_buf = (unsigned char *)malloc(fmt.fmt.pix.sizeimage);

#ifdef DEBUG

    printf("raw pixfmt: %c%c%c%c %dx%d\n",
               src_fmt.fmt.pix.pixelformat & 0xff,
               (src_fmt.fmt.pix.pixelformat >> 8) & 0xff,
               (src_fmt.fmt.pix.pixelformat >> 16) & 0xff,
               (src_fmt.fmt.pix.pixelformat >> 24) & 0xff,
               src_fmt.fmt.pix.width, src_fmt.fmt.pix.height);
#endif    
    
    if(ret < 0)
    {
        perror("VIDIOC_S_FMT");
        return;
    }
    
#ifdef DEBUG
    printf("pixfmt: %c%c%c%c %dx%d\n",
           fmt.fmt.pix.pixelformat & 0xff,
           (fmt.fmt.pix.pixelformat >> 8) & 0xff,
           (fmt.fmt.pix.pixelformat >> 16) & 0xff,
           (fmt.fmt.pix.pixelformat >> 24) & 0xff,
           fmt.fmt.pix.width, fmt.fmt.pix.height);
           fmt.fmt.pix.width, fmt.fmt.pix.height);
    
    /* Note VIDIOC_S_FMT may change width and height. */
#endif

    w = fmt.fmt.pix.width;
    h = fmt.fmt.pix.height;
   
    init_mmap();
}

int close_device(int dev)
{
    return close(dev);
}

int open_device(const char *dev)
{
    struct stat st;

    if(stat(dev, &st) < 0)
    {
#ifdef DEBUG    	    
        fprintf(stderr, "Cannot identify '%s': %d, %s\n", dev, errno, strerror(errno));
#endif
        return -1;
    }

    if(!S_ISCHR(st.st_mode))
    {
#ifdef DEBUG    	    
        fprintf(stderr, "%s is no device\n", dev);
#endif
        return -1;
    }

    fd = open(dev, O_RDWR /* required */  | O_NONBLOCK, 0);
    
    if(fd < 0)
    {
#ifdef DEBUG    	    
        fprintf(stderr, "Cannot open '%s': %d, %s\n", dev, errno, strerror(errno));
#endif
        return -1;
    }
    return fd;
}


