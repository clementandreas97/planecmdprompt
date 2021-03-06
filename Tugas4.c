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
#include <pthread.h>

int xResolution;
int yResolution;
int blue;
int green;
int red;

int fbfd = 0;
struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo finfo;
long int screensize = 0;
char *fbp = 0;
long int location = 0;
int **a;
int **copy_of_a;
int **wheel;
int **pilot;
int **rotor;
int **copyrotor;
int **true_rotor;
int **true_copyrotor;

pthread_t threads[6];
int life = 2;
int scale = 1;
int centerX;
int centerY;
int imin, jmin;

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

// Plane Scan Line

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

// Wheel Scan Line

void scanLineWheel(int n, int c1, int c2, int c3) {
    int i,j,k,gd,gm,dy,dx;
    int x,y,temp;
    int xi[n];
    float slope[n];

    // Initiate Variables
    wheel[n][0]=wheel[0][0];
    wheel[n][1]=wheel[0][1];
    
    // Calculate slope
    for(i=0;i<n;i++) {
        dy=wheel[i+1][1]-wheel[i][1];
        dx=wheel[i+1][0]-wheel[i][0];
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
            if (((wheel[i][1]<=y)&&(wheel[i+1][1]>y)) || ((wheel[i][1]>y)&&(wheel[i+1][1]<=y))) {
                xi[k]=(int)(wheel[i][0]+slope[i]*(y-wheel[i][1]));
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

// Pilot Scan Line

void scanLinePilot(int n, int c1, int c2, int c3) {
    int i,j,k,gd,gm,dy,dx;
    int x,y,temp;
    int xi[n];
    float slope[n];

    // Initiate Variables
    pilot[n][0]=pilot[0][0];
    pilot[n][1]=pilot[0][1];
    
    // Calculate slope
    for(i=0;i<n;i++) {
        dy=pilot[i+1][1]-pilot[i][1];
        dx=pilot[i+1][0]-pilot[i][0];
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
            if (((pilot[i][1]<=y)&&(pilot[i+1][1]>y)) || ((pilot[i][1]>y)&&(pilot[i+1][1]<=y))) {
                xi[k]=(int)(pilot[i][0]+slope[i]*(y-pilot[i][1]));
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

// Plane dilatation

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

// Rotor dilatation

void dilatasi_rotor(int n, int c1, int c2, int c3, int factor, int centerX, int centerY) {
    int i;
    for (i = 0; i < n; i++) {
        rotor[i][0] = true_rotor[i][0] * factor - (factor-1)*centerX;    
        copyrotor[i][0] = true_copyrotor[i][0] * factor - (factor-1)*centerX;
        
        rotor[i][1] = true_rotor[i][1] * factor - (factor-1)*centerY;
        copyrotor[i][1] = true_copyrotor[i][1] * factor - (factor-1)*centerY;
    }
}

void dilatasi_wheel(int n){
	for(int i=0; i<n; i++){
		wheel[i][0] = wheel[i][0] * scale - (scale-1)*centerX;
		wheel[i][1] = wheel[i][1] * scale - (scale-1)*centerY;
	}
}

void translasi_pilot(int n){
	for(int i=0; i<n; i++){
		pilot[i][1] = pilot[i][1] - (copy_of_a[jmin][1] - a[jmin][1]);
	}
}

// Rotor rotation

void rotasi(int centerX, int centerY) {
    int i;
    for (i = 0; i < 4; i++) {
        rotor[i][0] = (int)((float)(copyrotor[i][0]-centerX)*sqrt(2.0)/2.0 - (float)(copyrotor[i][1]-centerY)*sqrt(2.0)/2.0) + centerX;
     	rotor[i][1] = (int)((float)(copyrotor[i][0]-centerX)*sqrt(2.0)/2.0 + (float)(copyrotor[i][1]-centerY)*sqrt(2.0)/2.0) + centerY;

     	true_rotor[i][0] = (int)((float)(true_copyrotor[i][0]-centerX)*sqrt(2.0)/2.0 - (float)(true_copyrotor[i][1]-centerY)*sqrt(2.0)/2.0) + centerX;
     	true_rotor[i][1] = (int)((float)(true_copyrotor[i][0]-centerX)*sqrt(2.0)/2.0 + (float)(true_copyrotor[i][1]-centerY)*sqrt(2.0)/2.0) + centerY;
    }
}

// Create a rotor

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

// Draw two rotors

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

// Initiate plane from file

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

// Initiate wheel from file

void loadFileWheel(char* filename, int *size){
    FILE *fp;  
     
    fp = fopen(filename, "r");
    fscanf(fp, "%d", size);

    wheel = (int **)malloc((*size+1) * sizeof(int *));
    
    for (int i=0; i<(*size+1); i++){
         wheel[i] = (int *)malloc(2 * sizeof(int));
    }

    int i = 0;
    int j = 0;

    int temp;
    while(fscanf(fp, "%d", &temp)!=EOF){  
        if(j%2==0){
            wheel[i][0] = temp;
        }
        else{
            wheel[i][1] = temp;
            i++;
        }
        j++;  
    }
    
    fclose(fp);
}

// Initiate pilot from file

void loadFilePilot(char* filename, int *size){
    FILE *fp;  
     
    fp = fopen(filename, "r");
    fscanf(fp, "%d", size);

    pilot = (int **)malloc((*size+1) * sizeof(int *));
    
    for (int i=0; i<(*size+1); i++){
         pilot[i] = (int *)malloc(2 * sizeof(int));
    }

    int i = 0;
    int j = 0;

    int temp;
    while(fscanf(fp, "%d", &temp)!=EOF){  
        if(j%2==0){
            pilot[i][0] = temp;
        }
        else{
            pilot[i][1] = temp;
            i++;
        }
        j++;  
    }
    
    fclose(fp);
}

// Calculate center of plane

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
    		imin = i;
    	}

    	if(maxY < a[i][1]){
    		maxY = a[i][1];
    	}
    	if(minY > a[i][1]){
    		minY = a[i][1];
    		jmin = i;
    	}
    }

    printf("%d %d\n", imin, jmin);

    *centerX = minX + (maxX - minX)/2;
    *centerY = minY + (maxY - minY)/2;
}

// Bounce thread

int findLowestY(int n) {
    int minY = wheel[0][1];
    for (int i=1; i<n; i++) {
        if (wheel[i][1] > minY) {
            minY = wheel[i][1];
        }
    }
    return minY;
}

void bounce (int n, int gravity, int bounceCount, int c1, int c2, int c3) {
    int currentY = findLowestY(n);
    int acc;
    while (bounceCount > 0) {
        acc = gravity;
        while ((findLowestY(n) + acc) < yResolution-20 ) {
            scanLineWheel(n,0,0,0);
            for (int i=0; i<n; i++) {
                wheel[i][1] += acc;
            }
            scanLineWheel(n,c1,c2,c3);
            acc += gravity;
            usleep(50000);
        }
        while (acc > 1) {
            scanLineWheel(n,0,0,0);
            for (int i=0; i<n; i++) {
                wheel[i][1] -= acc;
            }
            scanLineWheel(n,c1,c2,c3);
            usleep(50000);
            acc -= 2 * gravity;
            if (acc < 1) {
                acc = 1;
            }
        }
        bounceCount--;
    }
    acc = gravity;
    while ((findLowestY(n) + acc) < yResolution-20 ) {
        scanLineWheel(n,0,0,0);
        for (int i=0; i<n; i++) {
            wheel[i][1] += acc;
            acc += 1;
        }
        scanLineWheel(n,c1,c2,c3);
        usleep(50000);
    }
}

void *bounceWheel(void *arg) {
	dilatasi_wheel(8);
    bounce(8,10,5,255/2,255/2,255/2);
}

// Eject pilot thread



void *ejectPilot(void *arg) {
	translasi_pilot(22);

    int x = 0;
    int y = 0;
    int vox = -1;
    int voy = 5;
    int a = -2;
    int t = 0;
    while (pilot[11][1] <= yResolution-20 && pilot[11][1] > 20 && pilot[11][0] > 20 && pilot[11][0] < xResolution-20) {
        scanLinePilot(22,0,0,255);
        usleep(100000);
        scanLinePilot(22,0,0,0);
        t++;
        x = vox*t;
        y = 0 - voy*t - a/2*pow(t,2);
        for (int i=0; i < 22; i++) {
        	pilot[i][0] += x;
        	pilot[i][1] += y;
        }
    }
}

// Shoot thread

void drawBullet(int x, int y, int c1, int c2, int c3) {
    bresenham(x,y-5,x,y+5,c1,c2,c3);
    bresenham(x-5,y,x+5,y,c1,c2,c3);
}

void *shoot1(void *arg) {
    int x = xResolution/2;
    int y = yResolution-20;
    int vox = -10;
    int voy = 40;
    int a = -2;
    int t = 0;
    while (y <= yResolution-20 && y > 20 && x > 20 && x < xResolution-20) {
        if (isSameColor(x,y,blue,green,red)) {
            life--;
            if (life == 1) {
                // Wheel bounce thread
                pthread_create(threads+4, NULL, bounceWheel, NULL);
                break;
            }
            else if (life == 0) {
                // Eject pilot thread
                pthread_create(threads+5, NULL, ejectPilot, NULL);
                break;
            }
        }
        drawBullet(x,y,0,0,255);
        usleep(100000);
        drawBullet(x,y,0,0,0);
        t++;
        x = xResolution/2 + vox*t;
        y = yResolution-20 - voy*t - a/2*pow(t,2);
    }
}

void *shoot2(void *arg) {
    int x = xResolution/2;
    int y = yResolution-20;
    int vox = 10;
    int voy = 40;
    int a = -2;
    int t = 0;
    while (y <= yResolution-20 && y > 20 && x > 20 && x < xResolution-20) {
        if (isSameColor(x,y,blue,green,red)) {
            life--;
            if (life == 1) {
                // Wheel bounce thread
                pthread_create(threads+4, NULL, bounceWheel, NULL);
                break;
            }
            else if (life == 0) {
                // Eject pilot thread
                pthread_create(threads+5, NULL, ejectPilot, NULL);
                break;
            }
        }
        drawBullet(x,y,0,0,255);
        usleep(100000);
        drawBullet(x,y,0,0,0);
        t++;
        x = xResolution/2 + vox*t;
        y = yResolution-20 - voy*t - a/2*pow(t,2);
    }
}

void *shoot(void *arg) {
    system ("/bin/stty raw");
    char c;
    while (c = getchar()) {
        if (c == 'd') {
            pthread_create(threads+1, NULL, shoot1, NULL);
        }
        else if (c == 'j') {
            pthread_create(threads+2, NULL, shoot2, NULL);
        }
        else {
            break;
        }
    }
    system ("/bin/stty cooked");
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
    red = atoi(argv[4]);
    green = atoi(argv[5]);
    blue = atoi(argv[6]);

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

    // Shoot thread
    pthread_create(threads, NULL, shoot, NULL);

    // Main thread
    int size;
    int wheel_size;
    int pilot_size;

    loadFile(filename, &size);
    loadFileWheel("wheel.txt", &wheel_size);
    loadFilePilot("pilot.txt", &pilot_size);

    int stillAvailable = 1;

    findCenter(size, &centerX, &centerY);

    // scanLine(size,blue,green,red);
    // scanLinePilot(22, 0, 0, 255);

    int distance_init = 160;
    int distance = distance_init;

    initRotor(592,351,26);

    while (stillAvailable == 1) {
        dilatasi(size,blue,green,red,scale,centerX,centerY,&stillAvailable);
        dilatasi_rotor(4,blue,green,red,scale,centerX, centerY);
        distance = distance_init*scale;

        scanLine(size,blue,green,red);
        
        int count = 0;
		drawRotor(blue/2,green/2,red/2,count,distance);
		//clock_t begin = clock();
		time_t begin, end;
		begin = time(NULL);

		for(;;) {
			usleep(100000);
			drawRotor(255,255,255,count,distance);
			count++;
			scanLine(size,blue,green,red);
			drawRotor(blue/2,green/2,red/2,count,distance);
			end = time(NULL);
			double time_spent = (double)(end - begin);
			if (time_spent >= 0.001){
				break;
			}
		}

    	scanLine(size,0,0,0);
    	drawRotor(0, 0, 0, 1, distance);
    	drawRotor(0, 0, 0, 2, distance);
    	scale++;
    }

    while(1);

    munmap(fbp, screensize);
    sleep(5);
    close(fbfd);
    return 0;
 }