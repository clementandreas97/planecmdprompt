#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <math.h>

int MAX_VERTEX = 20;

int xResolution;
int yResolution;

int fbfd = 0;
struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;
long int screensize = 0;
char *fbp = 0;
long int location = 0;

// Draw point

void setPixel(int x, int y, int c1, int c2, int c3) {
    location = (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + 
            (y+vinfo.yoffset) * finfo.line_length;
    *(fbp + location) = c1;
    *(fbp + location + 1) = c2;
    *(fbp + location + 2) = c3;
}

// Draw line

void bresenhamLow(int x0, int y0, int x1, int y1, int c1, int c2, int c3) {
    int dx = x1 - x0;
    int dy = y1 - y0;
    int yi = 1;
    if (dy < 0) {
        yi = -1;
        dy = -dy;
    }
    int D = 2*dy - dx;
    int y = y0;
    for (int x = x0; x <= x1; x++) {
        setPixel(x,y,c1,c2,c3);
        if (D > 0) {
            y += yi;
            D -= 2*dx;
        }
        D += 2*dy;
    }
}

void bresenhamHigh(int x0, int y0, int x1, int y1, int c1, int c2, int c3) {
    int dx = x1 - x0;
    int dy = y1 - y0;
    int xi = 1;
    if (dx < 0) {
        xi = -1;
        dx = -dx;
    }
    int D = 2*dx - dy;
    int x = x0;
    for (int y = y0; y <= y1; y++) {
        setPixel(x,y,c1,c2,c3);
        if (D > 0) {
            x += xi;
            D -= 2*dy;
        }
        D += 2*dx;
    }
}

void bresenham(int x0, int y0, int x1, int y1, int c1, int c2, int c3) {
    if (abs(y1 - y0) < abs(x1 - x0)) {
        if (x0 > x1) {
            bresenhamLow(x1,y1,x0,y0,c1,c2,c3);
        }
        else {
            bresenhamLow(x0,y0,x1,y1,c1,c2,c3);
        }
    }
    else {
        if (y0 > y1) {
            bresenhamHigh(x1,y1,x0,y0,c1,c2,c3);
        }
        else {
            bresenhamHigh(x0,y0,x1,y1,c1,c2,c3);
        }
    }
}

// Draw shape

void drawShape(int a[MAX_VERTEX][MAX_VERTEX], int n, int c1, int c2, int c3) {
    a[n][0]=a[0][0];
    a[n][1]=a[0][1];
    for(int i=0;i<n;i++) {
        bresenham(a[i][0],a[i][1],a[i+1][0],a[i+1][1],c1,c2,c3);
    }
}

// Check color similarity

int isSameColor(int x0, int y0, int c11, int c12, int c13) {
    int loc = (x0+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + 
            (y0+vinfo.yoffset) * finfo.line_length;
    int c01 = *(fbp + loc);
    if (c01 < 0) {
        c01 += 256;
    }
    int c02 = *(fbp + loc + 1);
    if (c02 < 0) {
        c02 += 256;
    }
    int c03 = *(fbp + loc + 2);
    if (c03 < 0) {
        c03 += 256;
    }
    return (c01 == c11 && c02 == c12 && c03 == c13);
}

// Scan Line

void scanLine(int a[MAX_VERTEX][MAX_VERTEX], int n, int c1, int c2, int c3) {
    int i,j,k,gd,gm,dy,dx;
    int x,y,temp;
    int xi[MAX_VERTEX];
    float slope[MAX_VERTEX];

    // Initiate Variables
    a[n][0]=a[0][0];
    a[n][1]=a[0][1];
    
    // Calculate slope
    for(i=0;i<n;i++) {
        dy=a[i+1][1]-a[i][1];
        dx=a[i+1][0]-a[i][0];
        if(dy==0) slope[i]=1.0;
        if(dx==0) slope[i]=0.0;
        if((dy!=0)&&(dx!=0)) {
            slope[i]=(float) dx/dy;
        }
    }

    // Fill color
    for(y=0;y<yResolution;y++) {
        k=0;
        // Initiate xi
        for(i=0;i<n;i++) {
            if (((a[i][1]<=y)&&(a[i+1][1]>y)) || ((a[i][1]>y)&&(a[i+1][1]<=y))) {
                xi[k]=(int)(a[i][0]+slope[i]*(y-a[i][1]));
                k++;
            }
        }
        // Sort xi
        for(j=0;j<k-1;j++)
            for(i=0;i<k-1;i++) {
                if(xi[i]>xi[i+1]) {
                    temp=xi[i];
                    xi[i]=xi[i+1];
                    xi[i+1]=temp;
                }
            }
        // Fill
        for(i=0;i<k;i+=2) {
            bresenham(xi[i],y,xi[i+1]+1,y,c1,c2,c3);
        }
    }
}

void dilatasi(int a[MAX_VERTEX][MAX_VERTEX], int n, int c1, int c2, int c3, int factor, int centerX, int centerY, int *stillAvailable) {
    int i;
    for (i = 0; i < n; i++) {
        if (a[i][0]*factor - centerX >= xResolution) {
            *stillAvailable = 0;
        } else {
            a[i][0] = a[i][0] * factor - centerX;    
        }
        if (a[i][1]*factor - centerY >= yResolution) {
            *stillAvailable = 0;
        } else {
            a[i][1] = a[i][1] * factor - centerY;
        }
    }
}

// void rotasi(int a[MAX_VERTEX][MAX_VERTEX], int n, int centerX, int centerY) {
//     int i;
//     for (i = 0; i < n; i++) {
//         a[i][0] = (int)((float)(a[i][0]-centerX)*sqrt(2.0)/2.0 - (float)(a[i][1]-centerY)*sqrt(2.0)/2.0) + centerX;
//         a[i][1] = (int)((float)(a[i][0]-centerX)*sqrt(2.0)/2.0 + (float)(a[i][1]-centerY)*sqrt(2.0)/2.0) + centerY;
//     }
// }

void rotasi(int centerX, int centerY, int dimension, int isX, int c1, int c2, int c3) {
    if (isX%2 == 0) {
        bresenham(centerX-dimension/2,centerY-dimension/2,centerX+dimension/2,centerY+dimension/2,0,0,0);
        bresenham(centerX-dimension/2,centerY+dimension/2,centerX+dimension/2,centerY-dimension/2,0,0,0);
        bresenham(centerX,centerY-dimension/2,centerX,centerY+dimension/2,c1,c2,c3);
        bresenham(centerX-dimension/2,centerY,centerX+dimension/2,centerY,c1,c2,c3);
    }
    else {
        bresenham(centerX,centerY-dimension/2,centerX,centerY+dimension/2,0,0,0);
        bresenham(centerX-dimension/2,centerY,centerX+dimension/2,centerY,0,0,0);
        bresenham(centerX-dimension/2,centerY-dimension/2,centerX+dimension/2,centerY+dimension/2,c1,c2,c3);
        bresenham(centerX-dimension/2,centerY+dimension/2,centerX+dimension/2,centerY-dimension/2,c1,c2,c3);
    }
}

// Main

int main(int argc, char **argv) {
    // Input resolution
    if (argc != 6) {
        fprintf(stderr, "Please input resolution, red, green, and blue\n");
        fprintf(stderr, "Example: %s 1366 768 255 255 255\n", argv[0]);
        return -1;
    }
    xResolution = atoi(argv[1]);
    yResolution = atoi(argv[2]);
    int red = atoi(argv[3]);
    int green = atoi(argv[4]);
    int blue = atoi(argv[5]);

    // Open the file for reading and writing
    fbfd = open("/dev/fb0", O_RDWR);
    if (fbfd == -1) {
        perror("Error: cannot open framebuffer device");
        exit(1);
    }
    printf("The framebuffer device was opened successfully.\n");

    // Get fixed screen information
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1) {
        perror("Error reading fixed information");
        exit(2);
    }

    // Get variable screen information
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
        perror("Error reading variable information");
        exit(3);
    }

    printf("%dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);

    // Figure out the size of the screen in bytes
    screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

    // Map the device to memory
    fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED,
                        fbfd, 0);
    if ((long)fbp == -1) {
        perror("Error: failed to map framebuffer device to memory");
        exit(4);
    }
    printf("The framebuffer device was mapped to memory successfully.\n");

    // Figure out where in memory to put the pixel
    int a[MAX_VERTEX][MAX_VERTEX];
    a[0][0] = 658;
    a[0][1] = 359;
    a[1][0] = 708;
    a[1][1] = 359;
    a[2][0] = 708;
    a[2][1] = 409;
    a[3][0] = 658;
    a[3][1] = 409;
    //drawShape(a,4,255,255,255);
    //scanLine(a,4,blue,green,red);

    // int stillAvailable = 1;
    // int i;

    // usleep(1000000);
    // for (;;) {
    //     scanLine(a,4,0,0,0);
    //     // dilatasi(a,4,blue,green,red,2,683,384,&stillAvailable);
    //     rotasi(a,4,683,384);
    //     if (stillAvailable == 1) {
    //         scanLine(a,4,blue,green,red);
    //     }
    //     else {
    //         break;
    //     }
    //     usleep(1000000);
    // }

    int isX = 0;
    for (;;) {
        rotasi(683,384,100,isX,blue,green,red);
        usleep(150000);
        isX++;
    }

    munmap(fbp, screensize);
    sleep(5);
    close(fbfd);
    return 0;
 }