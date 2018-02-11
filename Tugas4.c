#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <math.h>
#include <limits.h>
#include <time.h>

int xResolution;
int yResolution;

int fbfd = 0;
struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;
long int screensize = 0;
char *fbp = 0;
long int location = 0;
int **a;
int **copy_of_a;
int **rotor;
int **copyrotor;
int **true_rotor;
int **true_copyrotor;

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

void drawShape(int n, int c1, int c2, int c3) {
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

void scanLine(int n, int c1, int c2, int c3) {
    int i,j,k,gd,gm,dy,dx;
    int x,y,temp;
    int xi[n];
    float slope[n];

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

void clear(int n){
	int i,j,k,gd,gm,dy,dx;
    int x,y,temp;
    int xi[n];
    float slope[n];

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
            bresenham(xi[i],y,xi[i+1]+1,y,0,0,0);
        }
    }
}

void dilatasi(int n, int c1, int c2, int c3, int factor, int centerX, int centerY, int *stillAvailable) {
    int i;
    for (i = 0; i < n; i++) {
        if (copy_of_a[i][0]*factor - (factor-1)*centerX >= xResolution) {
            *stillAvailable = 0;
        } else {
            a[i][0] = copy_of_a[i][0] * factor - (factor-1)*centerX;    
        }
        if (copy_of_a[i][1]*factor - (factor-1)*centerY >= yResolution) {
            *stillAvailable = 0;
        } else {
            a[i][1] = copy_of_a[i][1] * factor - (factor-1)*centerY;
        }
    }
}

void dilatasi_rotor(int n, int c1, int c2, int c3, int factor, int centerX, int centerY) {
    int i;
    for (i = 0; i < n; i++) {
        rotor[i][0] = true_rotor[i][0] * factor - (factor-1)*centerX;    
        copyrotor[i][0] = true_copyrotor[i][0] * factor - (factor-1)*centerX;
        
        rotor[i][1] = true_rotor[i][1] * factor - (factor-1)*centerY;
        copyrotor[i][1] = true_copyrotor[i][1] * factor - (factor-1)*centerY;
    }
}

void rotasi(int centerX, int centerY) {
    int i;
    for (i = 0; i < 4; i++) {
        rotor[i][0] = (int)((float)(copyrotor[i][0]-centerX)*sqrt(2.0)/2.0 - (float)(copyrotor[i][1]-centerY)*sqrt(2.0)/2.0) + centerX;
     	rotor[i][1] = (int)((float)(copyrotor[i][0]-centerX)*sqrt(2.0)/2.0 + (float)(copyrotor[i][1]-centerY)*sqrt(2.0)/2.0) + centerY;

     	true_rotor[i][0] = (int)((float)(true_copyrotor[i][0]-centerX)*sqrt(2.0)/2.0 - (float)(true_copyrotor[i][1]-centerY)*sqrt(2.0)/2.0) + centerX;
     	true_rotor[i][1] = (int)((float)(true_copyrotor[i][0]-centerX)*sqrt(2.0)/2.0 + (float)(true_copyrotor[i][1]-centerY)*sqrt(2.0)/2.0) + centerY;
    }
}

void initRotor(int centerX, int centerY, int dimension) {
	copyrotor = (int **)malloc(4 * sizeof(int *));
	true_copyrotor = (int **)malloc(4 * sizeof(int *));
    for (int i=0; i<4; i++){
         copyrotor[i] = (int *)malloc(2 * sizeof(int));
         true_copyrotor[i] = (int *)malloc(2 * sizeof(int));
    }

    copyrotor[0][0] = centerX;
    copyrotor[0][1] = centerY-dimension/2;
    copyrotor[1][0] = centerX;
    copyrotor[1][1] = centerY+dimension/2;
    copyrotor[2][0] = centerX-dimension/2;
    copyrotor[2][1] = centerY;
    copyrotor[3][0] = centerX+dimension/2;
    copyrotor[3][1] = centerY;

    true_copyrotor[0][0] = centerX;
    true_copyrotor[0][1] = centerY-dimension/2;
    true_copyrotor[1][0] = centerX;
    true_copyrotor[1][1] = centerY+dimension/2;
    true_copyrotor[2][0] = centerX-dimension/2;
    true_copyrotor[2][1] = centerY;
    true_copyrotor[3][0] = centerX+dimension/2;
    true_copyrotor[3][1] = centerY;

    rotor = (int **)malloc(4 * sizeof(int *));
    true_rotor = (int **)malloc(4 * sizeof(int *));
    for (int i=0; i<4; i++){
         rotor[i] = (int *)malloc(2 * sizeof(int));
         true_rotor[i] = (int *)malloc(2 * sizeof(int));
    }
    rotasi(centerX,centerY);
}

void drawRotor(int c1, int c2, int c3, int count, int d) {
	if (count%2 == 0) {
		bresenham(copyrotor[0][0],copyrotor[0][1],copyrotor[1][0],copyrotor[1][1],c1,c2,c3);
		bresenham(copyrotor[2][0],copyrotor[2][1],copyrotor[3][0],copyrotor[3][1],c1,c2,c3);
		bresenham(copyrotor[0][0]+d,copyrotor[0][1],copyrotor[1][0]+d,copyrotor[1][1],c1,c2,c3);
		bresenham(copyrotor[2][0]+d,copyrotor[2][1],copyrotor[3][0]+d,copyrotor[3][1],c1,c2,c3);
	}
	else {
		bresenham(rotor[0][0],rotor[0][1],rotor[1][0],rotor[1][1],c1,c2,c3);
		bresenham(rotor[2][0],rotor[2][1],rotor[3][0],rotor[3][1],c1,c2,c3);
		bresenham(rotor[0][0]+d,rotor[0][1],rotor[1][0]+d,rotor[1][1],c1,c2,c3);
		bresenham(rotor[2][0]+d,rotor[2][1],rotor[3][0]+d,rotor[3][1],c1,c2,c3);
	}
}

void loadFile(char* filename, int *size){
    FILE *fp;  
     
    fp = fopen(filename, "r");
    fscanf(fp, "%d", size);

    a = (int **)malloc((*size+1) * sizeof(int *));
    copy_of_a = (int **)malloc((*size+1) * sizeof(int *));
    
    for (int i=0; i<(*size+1); i++){
         a[i] = (int *)malloc(2 * sizeof(int));
     	 copy_of_a[i] = (int *)malloc(2 * sizeof(int));
    }

    int i = 0;
    int j = 0;

    int temp;
    while(fscanf(fp, "%d", &temp)!=EOF){  
        if(j%2==0){
            a[i][0] = temp;
            copy_of_a[i][0] = temp;
        }
        else{
            a[i][1] = temp;
            copy_of_a[i][1] = temp;
            i++;
        }
        j++;  
    }
    
    fclose(fp);
}

void findCenter(int size, int *centerX, int *centerY){
	int maxX = INT_MIN;
    int minX = INT_MAX;

    int maxY = INT_MIN;
    int minY = INT_MAX;

    for(int i=0; i<size; i++){
    	if(maxX < a[i][0]){
    		maxX = a[i][0];
    	}
    	if(minX > a[i][0]){
    		minX = a[i][0];
    	}

    	if(maxY < a[i][1]){
    		maxY = a[i][1];
    	}
    	if(minY > a[i][1]){
    		minY = a[i][1];
    	}
    }

    *centerX = minX + (maxX - minX)/2;
    *centerY = minY + (maxY - minY)/2;
}

// Main
int main(int argc, char **argv) {
    // Input resolution
    if (argc != 7) {
        fprintf(stderr, "Please input filename, resolution, red, green, and blue\n");
        fprintf(stderr, "Example: %s input.txt 1366 768 255 255 255\n", argv[0]);
        return -1;
    }

    char *filename = argv[1];
    xResolution = atoi(argv[2]);
    yResolution = atoi(argv[3]);
    int red = atoi(argv[4]);
    int green = atoi(argv[5]);
    int blue = atoi(argv[6]);

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

    
    int size;

    loadFile(filename, &size);

    int stillAvailable = 1;
    
    int centerX;
    int centerY;

    findCenter(size, &centerX, &centerY);

    int i = 1;
    int distance_init = 160;
    int distance = distance_init;

    initRotor(592,351,26);

    for (;;) {
        dilatasi(size,blue,green,red,i,centerX,centerY,&stillAvailable);
        dilatasi_rotor(4,blue,green,red,i,centerX, centerY);
        distance = distance_init*i;

        if (stillAvailable == 1) {
            scanLine(size,blue,green,red);
            
            int count = 0;
    		drawRotor(blue/2,green/2,red/2,count,distance);
    		clock_t begin = clock();
    		for(;;) {
    			usleep(100000);
    			drawRotor(255,255,255,count,distance);
    			count++;
    			drawRotor(blue/2,green/2,red/2,count,distance);
    			clock_t end = clock();
    			double time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
    			if(time_spent >= 0.001){
    				break;
    			}
    		}

        	clear(size);
        	drawRotor(0, 0, 0, 1, distance);
        	drawRotor(0, 0, 0, 2, distance);
        	i++;
        }
        else {
            break;
        }
    }

    
    

    munmap(fbp, screensize);
    sleep(5);
    close(fbfd);
    return 0;
 }